#ifndef DIGITAL_IMUSE_WAVEDRV
#define DIGITAL_IMUSE_WAVEDRV

#include "imuse.h"

int wvSlicingHalted;

int wavedrv_init(iMUSEInitData *initDataPtr);
int wavedrv_terminate();
int wavedrv_pause();
int wavedrv_resume();
int wavedrv_save(unsigned char *buffer, int sizeLeft);
int wavedrv_restore(unsigned char *buffer);
int wavedrv_setGroupVol();
int wavedrv_startSound(int soundId, int priority);
int wavedrv_stopSound(int soundId);
int wavedrv_stopAllSounds();
int wavedrv_getNextSound(int soundId);
int wavedrv_setParam(int soundId, int opcode, int value);
int wavedrv_getParam(int soundId, int opcode);
int wavedrv_setHook(int soundId, int hookId);
int wavedrv_getHook(int soundId);
int wavedrv_startStream(int oldSoundId, int newSoundId, int param);
int wavedrv_switchStream(int oldSoundId, int soundId, int param3, int fadeSyncFlag2, int fadeSyncFlag1);
int wavedrv_processStreams();
int wavedrv_queryStream(int soundId, int sampleRate, int param3, int param4, int param5);
int wavedrv_feedStream(int soundId, int param2, int param3, int param4);
int wavedrv_lipSync(int soundId, int syncId, int msPos, int width, char height);

#endif 