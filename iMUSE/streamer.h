#ifndef DIGITAL_IMUSE_STREAMER
#define DIGITAL_IMUSE_STREAMER

#include "imuse.h"

#define MAX_STREAMS 3

typedef struct {
	int soundId;
	int curOffset;
	int endOffset;
	int bufId;
	uint8 *buf;
	int bufFreeSize;
	int loadSize;
	int criticalSize;
	int maxRead;
	int loadIndex;
	int readIndex;
	int paused;
} iMUSEStream;

iMUSEInitData * streamer_initDataPtr;
iMUSEStream streamer_streams[3];
iMUSEStream *streamer_lastStreamLoaded;
int streamer_bailFlag;

int streamer_moduleInit();
iMUSEStream *streamer_alloc(int soundId, int bufId, int maxRead);
int streamer_clearSoundInStream(iMUSEStream *streamPtr);
int streamer_processStreams();
int *streamer_reAllocReadBuffer(iMUSEStream *streamPtr, unsigned int reallocSize);
int *streamer_copyBufferAbsolute(iMUSEStream *streamPtr, int offset, int size);
int streamer_setIndex1(iMUSEStream *streamPtr, unsigned int offset);
int streamer_setIndex2(iMUSEStream *streamPtr, unsigned int offset);
int streamer_getFreeBuffer(iMUSEStream *streamPtr);
int streamer_setSoundToStreamWithCurrentOffset(iMUSEStream *streamPtr, int soundId, int currentOffset);
int streamer_queryStream(iMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused);
int streamer_feedStream(iMUSEStream *streamPtr, uint8 *srcBuf, unsigned int sizeToFeed, int paused);
int streamer_fetchData(iMUSEStream *streamPtr);

#endif