#include "files.h"
#include "imuse.h"
#include <stdio.h>

int files_moduleInit(iMUSEInitData *initDataPtr) {
	files_initDataPtr = initDataPtr;
	if (!initDataPtr)
		return -1;
	return 0;
}

int files_moduleDeinit() {
	return 0;
}

int files_getSoundAddrData(int soundId) {
	if (soundId > 0 && soundId < 0xFFFFFFF0 && files_initDataPtr) {
		if (files_initDataPtr->getSoundDataAddr) {
			return files_initDataPtr->getSoundDataAddr(soundId);
		} // Dov'è la funzione?
	}
	printf("ERR: soundAddrFunc failure");
	return NULL;
}

int files_some1(soundId) {
	if (soundId > 0 && soundId < 0xFFFFFFF0 && files_initDataPtr) {
		if (files_initDataPtr->func_some1) {
			return files_initDataPtr->func_some1(soundId);
		}
	}
	printf("ERR: soundAddrFunc failure");
	return NULL;
}

int files_getNextSound(int soundId) {
	int foundSoundId = 0;
	do {
		foundSoundId = wavedrv_getNextSound(foundSoundId);
		if (!foundSoundId)
			return -1;
		if (foundSoundId == soundId)
			return 2;
	} while (1); //TODO
	return -1;
}

int files_checkRange(int soundId) {
	return (soundId > 0 && soundId < 0xFFFFFFF0);
}

int files_seek(int soundId, __int32 offset, int mode) {
	if (soundId > 0 && soundId < 0xFFFFFFF0) {
		if (files_initDataPtr->seekFunc) {
			return files_initDataPtr->seekFunc(soundId, offset, mode);
		}
	}
	printf("ERR: seekFunc failure");
	return 0;
}

int files_read(int soundId, unsigned char *buf, int size) {
	if (soundId > 0 && soundId < 0xFFFFFFF0) {
		if (files_initDataPtr->readFunc) {
			return files_initDataPtr->readFunc(soundId, buf, size);
		}
	}
	printf("ERR: readFunc failure");
	return 0;
}

int files_getBufInfo(int bufId) {
	if (bufId && files_initDataPtr->bufInfoFunc) {
		return files_initDataPtr->bufInfoFunc(bufId);
	}
	printf("ERR: bufInfoFunc failure");
	return 0;
} // TODO DOV'è QUEL BUFINFOFUNC
