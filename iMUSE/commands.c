#include "commands.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TO_LE32(x)	(x)
#define TO_LE16(x)	(x)
#else
#define TO_LE32(x)	bswap_32(x)
#define TO_LE16(x)	bswap_16(x)
#endif

// Validated
void iMUSEHeartbeat() {
	// This is what happens:
	// - Usual audio stuff like fetching and playing sound (and everything 
	//   within waveapi_callback()) happens at a base 50Hz rate;
	// - Triggers and fades handling happens at a (somewhat hacky) 60Hz rate;
	// - Music gain reduction happens at a 10Hz rate.

	int soundId, foundGroupId, musicTargetVolume, musicEffVol, musicVol;
	if (!cmd_initDataPtr)
		return;

	int usecPerInt = timer_getUsecPerInt(); // this seems always seems to be 20000 (50 Hz)
	waveapi_callback();
	if (cmd_hostIntHandler) {
		cmd_hostIntUsecCount += usecPerInt;
		while (cmd_runningHostCount >= cmd_hostIntUsecCount) {
			cmd_runningHostCount -= cmd_hostIntUsecCount;
			cmds_hostIntHandler();
		}
	}

	if (cmd_pauseCount)
		return;

	cmd_running60HzCount += usecPerInt;
	while (cmd_running60HzCount >= 16667) {
		cmd_running60HzCount -= 16667;
		cmd_initDataPtr->num60hzIterations++;
		fades_loop();
		triggers_loop();
	}

	cmd_running10HzCount += usecPerInt;
	if (cmd_running10HzCount < 100000)
		return;

	do {
		// SPEECH GAIN REDUCTION 10Hz
		cmd_running10HzCount -= 100000;
		soundId = 0;
		musicTargetVolume = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1);
		while (1) { // Check all tracks to see if there's a speech file playing
			soundId = wave_getNextSound(soundId); 
			if (!soundId)
				break;

			foundGroupId = -1;
			if (files_getNextSound(soundId) == 2) {
				foundGroupId = wave_getParam(soundId, 0x400); // Check the groupId of this sound
			}

			if (foundGroupId == IMUSE_GROUP_SPEECH) {
				// Remember: when a speech file stops playing this block stops 
				// being executed, so musicTargetVolume returns back to its original value
				musicTargetVolume = (musicTargetVolume * 80) / 128;
				break;
			}
		}

		musicEffVol = groups_setGroupVol(IMUSE_GROUP_MUSICEFF, -1); // MUSIC EFFECTIVE VOLUME GROUP (used for gain reduction)
		musicVol = groups_setGroupVol(IMUSE_GROUP_MUSIC, -1); // MUSIC VOLUME SUBGROUP (keeps track of original music volume)

		if (musicEffVol < musicVol) { // If there is gain reduction already going on...
			musicEffVol += 3;
			if (musicEffVol >= musicTargetVolume) {
				if (musicVol <= musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			} else { // Bring up the effective music volume immediately when speech stops playing
				if (musicVol <= musicEffVol) {
					musicVol = musicEffVol;
				}
			}
		} else { // Bring music volume down to target volume with a -18 step if there's speech playing
			     // or else, just cap it to the target if it's out of range
			musicEffVol -= 18;
			if (musicEffVol > musicTargetVolume) {
				if (musicVol >= musicEffVol) {
					musicVol = musicEffVol;
				}
			} else {                                
				if (musicVol >= musicTargetVolume) {
					musicVol = musicTargetVolume;
				}
			}
		}
		groups_setGroupVol(IMUSE_GROUP_MUSICEFF, musicVol);
	} while (cmd_running10HzCount >= 100000);
}

// Validated
int cmds_handleCmds(int cmd, int arg_0,  int arg_1,  int arg_2,  int arg_3,  int arg_4,
						int arg_5,  int arg_6,  int arg_7,  int arg_8,  int arg_9,
						int arg_10, int arg_11, int arg_12, int arg_13) {

	if (cmd_initDataPtr == NULL && cmd != 0) {
		printf("ERROR:system not initialized...");
		return -1;
	}

	int(*func)() = iMUSE_FuncList[cmd];
	if (func != NULL && cmd < 30) {
		if (cmd != 17 && cmd != 20)
			func(arg_0,  arg_1,  arg_2,  arg_3,  arg_4,
				 arg_5,  arg_6,  arg_7,  arg_8,  arg_9);
		else
			func(arg_0,  arg_1,  arg_2,  arg_3,  arg_4,
				 arg_5,  arg_6,  arg_7,  arg_8,  arg_9,
				 arg_10, arg_11, arg_12, arg_13);
	}

	printf("ERROR:bogus opcode...");
	return -1;

}

// Validated
int cmds_init(iMUSEInitData * initDataPtr) {
	if (cmd_initDataPtr) {
		printf("ERROR:system already initialized...");
		return -1;
	}

	if (initDataPtr == NULL) {
		printf("ERROR: null initDataPtr...");
		return -1;
	}

	cmd_hostIntHandler = initDataPtr->hostIntHandler;
	cmd_hostIntUsecCount = initDataPtr->hostIntUsecCount;
	cmd_runningHostCount = 0;
	cmd_running60HzCount = 0;
	cmd_running10HzCount = 0;
	initDataPtr->num60hzIterations = 0;

	if (files_moduleInit(initDataPtr) || groups_moduleInit() || fades_moduleInit() ||
		triggers_moduleInit(initDataPtr) || wave_init(initDataPtr) || timer_moduleInit(initDataPtr)) {
		return -1;
	}

	cmd_initDataPtr = initDataPtr;
	cmd_pauseCount = 0;
	//vh_initModule("iMUSE:source:commands.c", IMUSE_CMDS_DEINIT, IMUSE_CMDS_DEBUG, IMUSE_CMDS_MODULE_SPECIAL);
	return 48;
}

// Validated
int cmds_deinit() {
	cmd_initDataPtr = NULL;
	timer_moduleDeinit();
	wave_terminate();
	waveout_free();
	triggers_clear();
	fades_moduleDeinit();
	groups_moduleDeinit();
	files_moduleDeinit();
	cmd_pauseCount = 0;
	cmd_hostIntHandler = 0;
	cmd_hostIntUsecCount = 0;
	cmd_runningHostCount = 0;
	cmd_running60HzCount = 0;
	cmd_running10HzCount = 0;

	return 0;
}

// Validated
int cmds_terminate() {
	//vh_ReleaseModule("iMUSE");
	return 0;
}

// Duplicate save/load stuff, I don't think we need it for the time being
int cmds_persistence(int cmd, void *funcCall) {
	return 0;
}

// Validated (or at least good enough)
int cmds_print(int param1, int param2, int param3, int param4, int param5, int param6, int param7) {
	return printf("%d %d %d %d %d %d %d", param1, param2, param3, param4, param5, param6, param7);
}

// Validated
int cmds_pause() {
	int result = 0;

	if (cmd_pauseCount == 0) {
		result = wave_pause();
	}
	
	if (!result) {
		result = cmd_pauseCount + 1;
	}
	cmd_pauseCount++;

	return result;
}

// Validated
int cmds_resume() {
	int result = 0;

	if (cmd_pauseCount == 1) {
		result = wave_resume();
	}

	if (cmd_pauseCount != 0) {
		cmd_pauseCount--;
	}

	if (!result) {
		result = cmd_pauseCount;
	}
	
	return result;
}

// Validated
int cmds_save(int * buffer, int bufferSize) {
	if (bufferSize < 10000) {
		printf("ERR: save buffer too small...");
		return -1;
	}

	buffer[0] = 0x30;
	buffer[1] = *(int*) cmd_initDataPtr->waveMixCount;
	
	int savedSize = fades_save((buffer + 8), bufferSize - 8);
	if (savedSize >= 0) {
		int tmpSize = savedSize + 8;
		savedSize = triggers_save(buffer + tmpSize, bufferSize - tmpSize);
		if (savedSize >= 0) {
			tmpSize += savedSize;
		}
		savedSize = wave_save(buffer + tmpSize, bufferSize - tmpSize);
		if (savedSize >= 0)
			savedSize += tmpSize;
	}

	return savedSize;
}

// Validated
int cmds_restore(int *buffer) {
	fades_moduleDeinit();
	triggers_clear();
	wave_stopAllSounds();

	if (buffer[0] != 0x30) {
		printf("ERR: restore buffer contains bad data...");
		return -1;
	}

	if (buffer[1] != *(int *) cmd_initDataPtr->waveMixCount) {
		printf("ERR: waveMixCount changed between save ");
		return -1;
	}

	int fadesSize = fades_restore(buffer + 2);
	int triggersSize = triggers_restore(buffer + fadesSize + 8);
	int waveSize = wave_restore(buffer + fadesSize + triggersSize + 8);
	return fadesSize + triggersSize + waveSize + 8;
}

// Validated
int cmds_startSound(int soundId, int priority) {
	uint8 *src = files_getSoundAddrData(soundId);
	int stringIndex = 0;

	if (src == NULL) {
		printf("ERR: null sound addr in StartSound()...");
		return -1;
	}

	// Check for the "iMUS" header
	uint8 *iMUSstring = (uint8*)TO_LE32("iMUS");
	do {
		if (src[stringIndex] != iMUSstring[stringIndex])
			break;
		stringIndex += 1;
	} while (stringIndex < 4);

	if (stringIndex == 4) {
		return wave_startSound(soundId, priority);
	}
	
	return -1;
}

// Validated
int cmds_stopSound(int soundId) {
	int result = files_getNextSound(soundId);
	
	if (result != 2)
		return -1;

	return wave_stopSound(soundId);
}

// Validated
int cmds_stopAllSounds() {
	int result = fades_moduleDeinit();
	result |= triggers_clear();
	result |= wave_stopAllSounds();
	
	return result;
}

// Validated
int cmds_getNextSound(int soundId) {
	return wave_getNextSound(soundId);
}

// Validated
int cmds_setParam(int soundId, int subCmd, int value) {
	int result = files_getNextSound(soundId);
	
	if (result != 2)
		return -1;
	
	return wave_setParam(soundId, subCmd, value);
}

// Validated
int cmds_getParam(int soundId, int subCmd) {
	int result = files_getNextSound(soundId);
	
	if (subCmd != 0) {
		if (subCmd == 0x200) {
			return triggers_countPendingSounds(soundId);
		}

		if (result == 2) {
			return wave_getParam(soundId, subCmd);
		}

		result = (subCmd == 0x100) - 1;
	}

	return result;
}

// Validated
int cmds_setHook(int soundId, int hookId) {
	int result = files_getNextSound(soundId);
	
	if (result != 2)
		return -1;
	
	return wave_setHook(soundId, hookId);
}

// Validated
int cmds_getHook(int soundId) {
	int result = files_getNextSound(soundId);
	
	if (result != 2)
		return -1;
	
	return wave_getHook(soundId);
}

// Validated
int cmds_debug() {
	printf("initImuseDataPtr: %p", cmd_initDataPtr);
	printf("pauseCount: %d", cmd_pauseCount);
	printf("hostIntHandler: %p", cmd_hostIntHandler);
	printf("hostIntUsecCount: %d", cmd_hostIntUsecCount);
	printf("runningHostCount: %d", cmd_runningHostCount);
	printf("running60HzCount: %d", cmd_running60HzCount);
	printf("running10HzCount: %d", cmd_running10HzCount);
	
	return 0;
}