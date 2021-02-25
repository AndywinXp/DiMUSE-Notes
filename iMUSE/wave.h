#ifndef DIGITAL_IMUSE_wave
#define DIGITAL_IMUSE_wave

#include "imuse.h"

int wvSlicingHalted;

int wave_init(iMUSEInitData *initDataPtr);
int wave_terminate();
int wave_pause();
int wave_resume();
int wave_save(unsigned char *buffer, int sizeLeft);
int wave_restore(unsigned char *buffer);
int wave_setGroupVol();
int wave_startSound(int soundId, int priority);
int wave_stopSound(int soundId);
int wave_stopAllSounds();
int wave_getNextSound(int soundId);
int wave_setParam(int soundId, int opcode, int value);
int wave_getParam(int soundId, int opcode);
int wave_setHook(int soundId, int hookId);
int wave_getHook(int soundId);
int wave_startStream(int oldSoundId, int newSoundId, int param);
int wave_switchStream(int oldSoundId, int soundId, int param3, int fadeSyncFlag2, int fadeSyncFlag1);
int wave_processStreams();
int wave_queryStream(int soundId, int sampleRate, int param3, int param4, int param5);
int wave_feedStream(int soundId, int param2, int param3, int param4);
int wave_lipSync(int soundId, int syncId, int msPos, int width, char height);

#endif 