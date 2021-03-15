#include "wave.h"

// Validated
int wave_init(iMUSEInitData *initDataPtr) {
	if (tracks_init(initDataPtr))
		return -1;
	wvSlicingHalted = 0;
	return 0;
}

// Validated
int wave_terminate() {
	return 0;
}

// Validated
int wave_pause() {
	wvSlicingHalted++;
	tracks_pause();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return 0;
}

// Validated
int wave_resume() {
	wvSlicingHalted++;
	tracks_resume();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return 0;
}

// Validated
int wave_save(unsigned char *buffer, int bufferSize) {
	wvSlicingHalted++;
	int result = tracks_save(buffer, bufferSize);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}

	return result;
}

// Validated
int wave_restore(unsigned char *buffer) {
	wvSlicingHalted++;
	int result = tracks_restore(buffer);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_setGroupVol() {
	wvSlicingHalted++;
	int result = tracks_setGroupVol();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_startSound(int soundId, int priority) {
	wvSlicingHalted++;
	int result = tracks_startSound(soundId, priority, 0);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_stopSound(int soundId) {
	wvSlicingHalted++;
	int result = tracks_stopSound(soundId);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_stopAllSounds() {
	wvSlicingHalted++;
	int result = tracks_stopAllSounds();
	if (wvSlicingHalted != 0) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_getNextSound(int soundId) {
	wvSlicingHalted++;
	int result = tracks_getNextSound(soundId);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_setParam(int soundId, int opcode, int value) {
	wvSlicingHalted++;
	int result = tracks_setParam(soundId, opcode, value);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_getParam(int soundId, int opcode) {
	wvSlicingHalted++;
	int result = tracks_getParam(soundId, opcode);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_setHook(int soundId, int hookId) {
	return tracks_setHook(soundId, hookId);
}

// Validated
int wave_getHook(int soundId) {
	return tracks_getHook(soundId);
}

// Validated
int wave_startSoundInGroup(int soundId, int priority, int groupId) {
	if (!files_checkRange(soundId))
		return -1;
	wvSlicingHalted++;
	int result = tracks_startSound(soundId, priority, groupId);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_switchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
	wvSlicingHalted++;
	int result = dispatch_switchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_processStreams() {
	wvSlicingHalted++;
	int result = streamer_processStreams();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	wvSlicingHalted++;
	int result = tracks_queryStream(soundId, bufSize, criticalSize, freeSpace, paused);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_feedStream(int soundId, int srcBuf, int sizeToFeed, int paused) {
	wvSlicingHalted++;
	int result = tracks_feedStream(soundId, srcBuf, sizeToFeed, paused);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

// Validated
int wave_lipSync(int soundId, int syncId, int msPos, int *width, int *height) {
	return tracks_lipSync(soundId, syncId, msPos, width, height);
}
