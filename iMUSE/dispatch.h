#ifndef DIGITAL_IMUSE_DISPATCH
#define DIGITAL_IMUSE_DISPATCH

#include "imuse.h"
#include "tracks.h"
#include "streamer.h"

typedef struct {
	iMUSETracks *trackPtr;
	int wordSize;
	int sampleRate;
	int channelCount;
	int currentOffset;
	int audioRemaining;
	int map;
	int field_1C;
	int map_frmt[254];
	iMUSEStreams *streamPtr;
	int streamBufID;
	struct iMUSEStreamZones *streamZoneList;
	int streamErrFlag;
	int fadeBuf;
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

typedef struct iMUSEStreamZones_node {
	struct iMUSEStreamZones_node *prev;
	struct iMUSEStreamZones_node *next;
	int useFlag;
	int offset;
	int size;
	int fadeFlag;
} iMUSEStreamZones;

iMUSEInitData *dispatch_initDataPtr;

#endif