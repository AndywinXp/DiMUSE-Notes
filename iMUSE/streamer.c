#include "streamer.h"
#include "imuse.h"
#include <stdio.h>

int streamer_moduleInit() {
	for (int l = 0; l < 3; l++) {
		streamer_streams[l]->soundId = 0;
	}
	streamer_lastStreamLoaded = 0;
	return 0;
}

void *streamer_alloc(int soundId, int bufID, int32 maxRead) {
	bufInfoPtr = files_bufInfo(bufID);
	if (!bufInfoPtr) {
		printf("streamer couldn't get buf info...");
		return 0;
	}

	if ((bufInfoPtr->size / 4) <= maxRead) {
		printf("maxRead too big for buf...");
	}

	for (int l = 0; l < 3; l++) {
		if (streamer_streams[l].soundId && streamer_streams[l].bufId == bufId) {
			printf("stream bufID %lu already in use...", bufId);
			return 0;
		}
	}
	for (int l = 0; l < 3; l++) {
		if (!streamer_streams[l].soundId) {
			streamer_streams[l].endOffset = files_seek(soundId, 0, SEEK_END);
			streamer_streams[l].curOffset = 0;
			streamer_streams[l].soundId = soundId;
			streamer_streams[l].bufID = bufID;
			streamer_streams[l].buf = bufInfoPtr->ptr; // + 0
			streamer_streams[l].bufFreeSize = bufInfoPtr->size - maxRead - 4; // + 4
			streamer_streams[l].loadSize = bufInfoPtr->loadSize ? ; // + 8
			streamer_streams[l].criticalSize = bufInfoPtr->criticalSize ? ; // + 12
			streamer_streams[l].maxRead = maxRead;
			streamer_streams[l].loadIndex = 0;
			streamer_streams[l].readIndex = 0;
			streamer_streams[l].paused = 0;
			return &streamer_streams[l];
		}
	}
	printf("no spare streams...");
	return 0;

}

int streamer_clearSoundInStream(void *streamPtr) {
	streamPtr->sound = 0;
	if (streamer_lastStreamLoaded == streamPtr) {
		streamer_lastStreamLoaded = 0;
	}
	return 0;
}

int streamer_processStreams() {
	dispatch_predictFirstStream();
	stream1 = NULL;
	stream2 = NULL;
	for (int l = 0; l < 3; l++) {
		if (!streamer_streams[l].soundId)
			continue;
		if (!streamer_streams[l].paused)
			continue;
		if (!stream1) {
			stream1 = streamer_streams[l];
			continue;
		}
		if (!stream2) {
			stream2 = streamer_streams[l];
			continue;
		}
		printf("three streams in use...");
	}
	if (!stream1) {
		if (stream2)
			streamer_FetchData(stream2);
		return 0;
	}
	if (!stream2) {
		if (stream1)
			streamer_FetchData(stream1);
		return 0;
	}

	value = stream1->loadIndex - stream1->readIndex;
	if (value < 0) {
		value += stream1->bufFreeSize;
	}
	critical1 = (value < stream1->criticalSize);

	value2 = stream2->loadIndex - stream2->readIndex;
	if (value < 0) {
		value2 += stream2->bufFreeSize;
		critical1 = true;
	}
	critical2 = (value2 < stream2->criticalSize);

	if (critical1) {
		if (critical2) {
			if (stream1 == streamer_lastStreamLoaded) {
				streamer_FetchData(stream1);
				streamer_FetchData(stream2);
				return 0;
			}
			else {
				streamer_FetchData(stream2);
				streamer_FetchData(stream1);
				return 0;
			}
		}
		else {
			if (stream2 == streamer_lastStreamLoaded)
				streamer_FetchData(stream2);
			else
				streamer_FetchData(stream1);
			return 0;
		}
	}
	if (critical2) {
		streamer_FetchData(stream2);
		return 0;
	}
	else {
		if (stream1 == streamer_lastStreamLoaded)
			streamer_FetchData(stream1);
		else
			streamer_FetchData(stream2);
		return 0;
	}
}

int streamer_reAllocReadBuffer(void *streamPtr, int32 size) {
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (value < size || streamPtr->maxRead < value)
		return 0;
	value = streamPtr->bufFreeSize - streamPtr->readIndex;
	if (value >= size) {
		memcpy(streamPtr->buf + streamPtr->bufFreeSize, streamPtr->buf, size - streamPtr->bufFreeSize + streamPtr->readIndex + 4);
	}

	readIndex = streamPtr->readIndex;
	ptr = streamPtr->buf + streamPtr->readIndex;
	streamPtr->readIndex = readindex + size;
	if (streamPtr->bufFreeSize <= streamPtr->readIndex + size)
		streamPtr->readIndex = readindex + size - streamPtr->bufFreeSize;
	return ptr;
}

int streamer_copyBuffer_Absolute(void *streamPtr, int32 offset, int32 size) {
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (param1 + param2 > value || streamPtr->maxRead < size)
		return 0;
	value = param1 + streamPtr->readIndex;
	if (param1 + streamPtr->readIndex >= streamPtr->bufFreeSize) {
		value -= streamPtr->bufFreeSize;
	}

	value2 = streamPtr->bufFreeSize - value;
	if (value2 < param2) {
		memcpy(streamPtr->buf + streamPtr->bufFreeSize, streamPtr->buf, param2 + value - streamPtr->bufFreeSize);
	}

	return streamPtr->buf + value;
}

int streamer_setIndex1(void *streamPtr, int32 offset) {
	streamer_bailFlag = 1;
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (param1 > value)
		return -1;
	value = streamPtr->readIndex + offset;
	streamPtr->readIndex = value;
	if (streamPtr->bufFreeSize <= value) {
		streamPtr->readIndex = value - streamPtr->bufFreeSize;
	}
	return 0;
}

int streamer_setIndex2(void *streamPtr, int32 offset) {
	streamer_bailFlag = 1;
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufSizeLeft;
	if (value < param1)
		return -1;
	value = streamPtr->readIndex + offset;
	streamPtr->loadIndex = value;
	if (value >= streamPtr->bufFreeSize) {
		streamPtr->loadIndex = value - streamPtr->bufSizeLeft;
	}
	return 0;
}

int streamer_getFreeBuffer(void *streamPtr) {
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	return value;
}

int streamer_setSoundToStreamWithCurrentOffset(void *streamPtr, int soundID, int32 currentOffset) {
	streamer_bailFlag = 1;
	streamPtr->soundId = soundId;
	streamPtr->curOffset = currentOffset;
	streamPtr->endOffset = 0;
	streamPtr->paused = 0;
	if (streamer_lastStreamLoaded == streamPtr) {
		streamer_lastStreamLoaded = NULL;
	}
	return 0;
}

int streamer_queryStream(void *streamPtr, int *bufSize, int *sampleRate, int *freeSpace, bool *paused) {
	dispatch_predictFirstStream();
	*(int *)(bufSize) = streamPtr->bufFreeSize;
	*(int *)(criticalSize) = streamPtr->criticalSize;
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	*(int *)(freeSpace) = freeSpace;
	*(int *)(paused) = streamPtr->paused;
	return 0;
}

int streamer_feedStream(void *streamPtr, void *src, int32 size, int paused) {
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value <= 0)
		value += streamPtr->bufFreeSize;
	value -= 4;
	if (size > value) {
		printf("FeedStream() buffer overflow...");
		streamer_bailFlag = 1;
		value2 = size - value;
		value2 -= value2 % 12 + 12;
		value = streamPtr->loadIndex - streamPtr->readIndex;
		if (value < 0)
			value += streamPtr->bufFreeSize;
		if (value >= value2) {
			streamPtr->readIndex = value2 + streamPtr->bufFreeSize;
			if (streamPtr->bufFreeSize <= streamPtr->readIndex)
				streamPtr->readIndex -= streamPtr->bufFreeSize;
		}
	}
	while (size > 0) {
		loadIndex = streamPtr->loadIndex;
		value = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (value >= size) {
			value = size;
		}
		size -= value;
		memcpy(streamPtr->buf + streamPtr->loadIndex, src, value);
		src += value;
		val = value + streamPtr->loadIndex;
		streamPtr->curOffset += value;
		streamPtr->loadIndex += value;
		if (val >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = val - streamPtr->bufFreeSize;
		}
	} while (size > 0);

	streamPtr->paused = paused;
	return 0;
}

int streamer_fetchData(void *streamPtr) {
	if (!streamPtr->endOffset) {
		streamPtr->endOffset = files_Seek(streamPtr->soundId, 0, SEEK_END);
	}
	value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value <= 0)
		value += streamPtr->bufFreeSize;
	value -= 4;
	value2 = streamPtr->loadSize;
	value3 = streamPtr->endOffset - streamPtr->curOffset;
	if (value2 >= value) {
		value2 = value;
	}
	if (value2 >= value3) {
		value2 = value3;
	}
	if (value3 <= 0) {
		streamPtr->paused = 1;
		*(byte *)(streamPtr->buf + streamPtr->loadIndex) = 127;
		streamPtr->loadIndex++;
		*(byte *)(streamPtr->buf + streamPtr->loadIndex) = 127;
		streamPtr->loadIndex++;
		*(byte *)(streamPtr->buf + streamPtr->loadIndex) = 127;
		streamPtr->loadIndex++;
		*(byte *)(streamPtr->buf + streamPtr->loadIndex) = 127;
		streamPtr->loadIndex++;
	}
	do {
		if (value2 <= 0)
			return 0;
		value4 = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (value4 >= value2) {
			value4 = value2;
		}
		if (files_Seek(streamPtr->soundId, streamPtr->curOffset, SEEK_SET) != streamPtr->curOffset) {
			warning("Invalid seek in streamer...");
			streamPtr->paused = 1;
			return 0;
		}
		streamer_bailFlag = 0;
		waveapi_decreaseSlice();
		result = files_Read(streamPtr->soundId, streamPtr->buf + streamPtr->loadIndex, value4);
		if (streamer_bailFlag)
			return 0;
		value2 -= result;
		streamPtr->curOffset += result;
		streamer_lastStreamLoaded = streamPtr;
		freeSize = streamPtr->bufFreeSize;
		value5 = result + loadIndex;
		streamPtr->loadIndex += result;
		if (value5 >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = value5 - freeSize;
		}
	} while (result == value4);

	printf("unable to load correct amount data");
	streamer_lastStreamLoaded = NULL;
	return 0;
}
