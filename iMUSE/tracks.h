#ifndef DIGITAL_IMUSE_TRACKS
#define DIGITAL_IMUSE_TRACKS
#include "dispatch.h"
#define MAX_TRACKS 8

typedef struct 
{
	int prev;
	int next;
	iMUSEDispatch *dispatchPtr;
	int soundId;
	int marker;
	int group;
	int priority;
	int vol;
	int effVol;
	int pan;
	int detune;
	int transpose;
	int pitchShift;
	int mailbox;
	int jumpHook;
} iMUSETracks;

iMUSETracks tracks[MAX_TRACKS];
iMUSEInitData * tracks_initDataPtr;

int tracks_trackCount;
int (*tracks_waveCall)();
int tracks_pauseTimer;
char *tracks_trackList;
int tracks_prefSampleRate;
int tracks_running40HzCount;
int tracks_uSecsToFeed;

int tracks_moduleInit(iMUSEInitData *initDataPtr);
void tracks_pause();
void tracks_resume();
int tracks_save(int *buffer, int leftSize);
int tracks_restore(int *buffer);
void tracks_setGroupVol();
void tracks_callback();
int tracks_startSound(int soundId, int tryPriority, int unused);
int tracks_stopSound(int soundId);
int tracks_stopAllSounds();
int tracks_getNextSound(int soundId);
int tracks_queryStream(int soundId, int sampleRate, int param2, int param3, int paused);
int tracks_feedStream(int soundId, int param1, int param2, int param3);
void tracks_clear(void *trackPtr);
int tracks_setParam(int soundId, int opcode, int value);
int tracks_getParam(int soundId, int opcode);
int tracks_lipSync(int soundId, int syncId, int msPos, int *width, char *height);
int tracks_setHook(int soundId, int hookId);
int tracks_getHook(int soundId);
void tracks_free();
int tracks_setPriority(int priorirty, int minPriority, int maxPriority);
int tracks_detune(int detune, int minDetune, int maxDetune);
int tracks_debug();

#endif