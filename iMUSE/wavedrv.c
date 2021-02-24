#include "wavedrv.h"

int wavedrv_init(iMUSEInitData *initDataPtr) {
	if (tracks_init(initDataPtr))
		return -1;
	wvSlicingHalted = 0;
	return 0;
}

int wavedrv_terminate() {
	return 0;
}

int wavedrv_pause() {
	wvSlicingHalted++;
	tracks_pause();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return 0;
}

int wavedrv_resume() {
	wvSlicingHalted++;
	tracks_resume();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return 0;
}

int wavedrv_save(unsigned char *buffer, int sizeLeft) {
	wvSlicingHalted++;
	int result = tracks_save(buffer, sizeLeft);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}

	return result;
}

int wavedrv_restore(unsigned char *buffer) {
	wvSlicingHalted++;
	int result = tracks_restore(buffer);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_setGroupVol() {
	wvSlicingHalted++;
	int result = tracks_setGroupVol();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_startSound(int soundId, int priority) {
	wvSlicingHalted++;
	int result = tracks_startSound(soundId, priority, 0);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_stopSound(int soundId) {
	wvSlicingHalted++;
	int result = tracks_stopSound(soundId);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_stopAllSounds() {
	wvSlicingHalted++;
	int result = tracks_stopAllSounds();
	if (wvSlicingHalted != 0) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_getNextSound(int soundId) {
	wvSlicingHalted++;
	int result = tracks_getNextSound(soundId);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_setParam(int soundId, int opcode, int value) {
	wvSlicingHalted++;
	int result = tracks_setParam(soundId, opcode, value);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_getParam(int soundId, int opcode) {
	wvSlicingHalted++;
	int result = tracks_getParam(soundId, opcode);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_setHook(int soundId, int hookId) {
	return tracks_setHook(soundId, hookId);
}

int wavedrv_getHook(int soundId) {
	return tracks_getHook(soundId);
}

int wavedrv_startStream(int oldSoundId, int newSoundId, int param) {
	if (!files_checkRange(oldSoundId))
		return -1;
	wvSlicingHalted++;
	int result = tracks_startStream(oldSoundId, newSoundId, param);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_switchStream(int oldSoundId, int soundId, int param3, int fadeSyncFlag2, int fadeSyncFlag1) {
	wvSlicingHalted++;
	int result = dispatch_switchStream(oldSoundId, soundId, param3, fadeSyncFlag2, fadeSyncFlag1);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_processStreams() {
	wvSlicingHalted++;
	int result = streamer_processStreams();
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_queryStream(int soundId, int sampleRate, int param3, int param4, int param5) {
	wvSlicingHalted++;
	int result = tracks_queryStream(soundId, sampleRate, param3, param4, param5);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_feedStream(int soundId, int param2, int param3, int param4) {
	wvSlicingHalted++;
	int result = tracks_feedStream(soundId, param2, param3, param4);
	if (wvSlicingHalted) {
		wvSlicingHalted--;
	}
	return result;
}

int wavedrv_lipSync(int soundId, int syncId, int msPos, int width, char height) {
	return tracks_lipSync(soundId, syncId, msPos, width, height);
}
