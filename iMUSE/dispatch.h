#ifndef DIGITAL_IMUSE_DISPATCH
#define DIGITAL_IMUSE_DISPATCH

#include "imuse.h"

#define LARGE_FADES 1
#define SMALL_FADES 4
#define MAX_DISPATCHES 8
#define MAX_STREAMZONES 50
#define LARGE_FADE_DIM 350000
#define SMALL_FADE_DIM 44100
#define MAX_FADE_VOLUME 8323072

typedef struct {
	iMUSETrack *trackPtr;
	int wordSize;
	int sampleRate;
	int channelCount;
	int currentOffset;
	int audioRemaining;
	int map[256]; // For COMI it's bigger
	iMUSEStream *streamPtr;
	int streamBufID;
	iMUSEStreamZone *streamZoneList;
	int streamErrFlag;
	int *fadeBuf;
	int fadeOffset;
	int fadeRemaining;
	int fadeWordSize;
	int fadeSampleRate;
	int fadeChannelCount;
	int fadeSyncFlag;
	int fadeSyncDelta;
	int fadeVol;
	int fadeSlope;
} iMUSEDispatch;

typedef struct iMUSEStreamZone_node {
	struct iMUSEStreamZone_node *prev;
	struct iMUSEStreamZone_node *next;
	int useFlag;
	int offset;
	int size;
	int fadeFlag;
} iMUSEStreamZone;

iMUSEInitData *dispatch_initDataPtr;

iMUSEDispatch dispatches[MAX_DISPATCHES];
iMUSEStreamZone streamZones[MAX_STREAMZONES];
int *dispatch_buf;
int dispatch_size;
int *dispatch_smallFadeBufs;
int *dispatch_largeFadeBufs;
int dispatch_fadeSize;
int dispatch_largeFadeFlags[LARGE_FADES];
int dispatch_smallFadeFlags[SMALL_FADES];
int dispatch_fadeStartedFlag;
int buff_hookid;
int dispatch_requestedFadeSize;
int dispatch_curStreamBufSize;
int dispatch_curStreamCriticalSize;
int dispatch_curStreamFreeSpace;
int dispatch_curStreamPaused;

int dispatch_moduleInit(iMUSEInitData *initDataptr);
iMUSEDispatch *dispatch_getTrackById(int trackId);
int dispatch_save(int dst, int size);
int dispatch_restore(const void *src);
int dispatch_allocStreamZones();
int dispatch_alloc(iMUSETrack *trackPtr, int groupId);
int dispatch_release(iMUSETrack *trackPtr);
int dispatch_switchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag);
void dispatch_processDispatches(iMUSETrack *trackPtr, int feedSize, int sampleRate);
int dispatch_predictFirstStream();
int dispatch_getNextMapEvent(iMUSEDispatch *dispatchPtr);
int dispatch_convertMap(uint8 *rawMap, uint8 *destMap);
void dispatch_predictStream(iMUSEDispatch *dispatch);
void dispatch_parseJump(iMUSEDispatch *dispatchPtr, iMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetNextMapEvent);
iMUSEStreamZone *dispatch_allocStreamZone();
void dispatch_free();

#endif