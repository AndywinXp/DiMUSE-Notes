#ifndef DIGITAL_IMUSE_STREAMER
#define DIGITAL_IMUSE_STREAMER

#include "imuse.h"

#define MAX_STREAMS 3


typedef struct {
	int soundId;
	int curOffset;
	int endOffset;
	int bufId;
	int buf;
	int bufFreeSize;
	int loadSize;
	int criticalSize;
	int maxRead;
	int loadIndex;
	int readIndex;
	int paused;
} iMUSEStreams;

iMUSEInitData * streamer_initDataPtr;
iMUSEStreams streamer_streams[3];
iMUSEStreams *streamer_lastStreamLoaded;
int streamer_bailFlag;

int streamer_moduleInit();
iMUSEStreams *streamer_alloc(int soundId, int bufId, int maxRead);
int streamer_clearSoundInStream(iMUSEStreams *streamPtr);
int streamer_processStreams();
int streamer_reAllocReadBuffer(iMUSEStreams *streamPtr, unsigned int reallocSize);
int streamer_copyBufferAbsolute(iMUSEStreams *streamPtr, int offset, int size);
int streamer_setIndex1(iMUSEStreams *streamPtr, unsigned int offset);
int streamer_setIndex2(iMUSEStreams *streamPtr, unsigned int offset);
int streamer_getFreeBuffer(iMUSEStreams *streamPtr);
int streamer_setSoundToStreamWithCurrentOffset(iMUSEStreams *streamPtr, int soundId, int currentOffset);
int streamer_queryStream(iMUSEStreams *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
int streamer_feedStream(iMUSEStreams *streamPtr, int *srcBuf, unsigned int sizeToFeed, int paused);
int streamer_fetchData(iMUSEStreams *streamPtr);

#endif