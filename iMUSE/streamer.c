#include "streamer.h"

// Validated
int streamer_moduleInit() {
	for (int l = 0; l < MAX_STREAMS; l++) {
		streamer_streams[l].soundId = 0;
	}
	streamer_lastStreamLoaded = NULL;
	return 0;
}

// Validated
iMUSEStream *streamer_alloc(int soundId, int bufId, int maxRead) {
	iMUSESoundBuffer *bufInfoPtr = files_getBufInfo(bufId);
	if (!bufInfoPtr) {
		printf("ERR: streamer couldn't get buf info...");
		return NULL;
	}

	if ((bufInfoPtr->bufSize / 4) <= maxRead) {
		printf("ERR: maxRead too big for buf...");
		return NULL;
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (streamer_streams[l].soundId && streamer_streams[l].bufId == bufId) {
			printf("ERR: stream bufID %lu already in use...", bufId);
			return NULL;
		}
	}

	for (int l = 0; l < MAX_STREAMS; l++) {
		if (!streamer_streams[l].soundId) {
			streamer_streams[l].endOffset = files_seek(soundId, 0, SEEK_END);
			streamer_streams[l].curOffset = 0;
			streamer_streams[l].soundId = soundId;
			streamer_streams[l].bufId = bufId;
			streamer_streams[l].buf = bufInfoPtr->buffer;
			streamer_streams[l].bufFreeSize = bufInfoPtr->bufSize - maxRead - 4;
			streamer_streams[l].loadSize = bufInfoPtr->loadSize;
			streamer_streams[l].criticalSize = bufInfoPtr->criticalSize;
			streamer_streams[l].maxRead = maxRead;
			streamer_streams[l].loadIndex = 0;
			streamer_streams[l].readIndex = 0;
			streamer_streams[l].paused = 0;
			return &streamer_streams[l];
		}
	}
	printf("ERR: no spare streams...");
	return NULL;
}

// Validated
int streamer_clearSoundInStream(iMUSEStream *streamPtr) {
	streamPtr->soundId = 0;
	if (streamer_lastStreamLoaded == streamPtr) {
		streamer_lastStreamLoaded = 0;
	}
	return 0;
}

// Validated
int streamer_processStreams() {
	dispatch_predictFirstStream();
	iMUSEStream *stream1 = NULL;
	iMUSEStream *stream2 = NULL;
	iMUSEStream *stream1_tmp = NULL;
	iMUSEStream *stream2_tmp = NULL;

	// Rewrite this mess in a more readable way
	for (int l = 0; l < MAX_STREAMS; l++) {
		if ((streamer_streams[l].soundId != 0) && 
			(!streamer_streams[l].paused) &&
			(stream1 = &streamer_streams[l], stream1_tmp != NULL) &&
			(stream1 = stream1_tmp, stream2 = &streamer_streams[l], stream2_tmp != NULL)) {
			printf("ERR: three streams in use...");
			stream2 = stream2_tmp;
		}
		stream1_tmp = stream1;
		stream2_tmp = stream2;
	}

	if (!stream1) {
		if (stream2)
			streamer_fetchData(stream2);
		return 0;
	}
	if (!stream2) {
		if (stream1)
			streamer_fetchData(stream1);
		return 0;
	}

	int size1 = stream1->loadIndex - stream1->readIndex;
	if (size1 < 0) {
		size1 += stream1->bufFreeSize;
	}

	int size2 = stream2->loadIndex - stream2->readIndex;
	if (size1 < 0) {
		size2 += stream2->bufFreeSize;
	}

	// Check if we've reached or surpassed criticalSize
	int critical1 = (size1 >= stream1->criticalSize);
	int critical2 = (size2 >= stream2->criticalSize);

	if (!critical1) {
		if (!critical2) {
			if (stream1 == streamer_lastStreamLoaded) {
				streamer_fetchData(stream1);
				streamer_fetchData(stream2);
				return 0;
			} else {
				streamer_fetchData(stream2);
				streamer_fetchData(stream1);
				return 0;
			}
		} else {
			streamer_fetchData(stream1);
			return 0;
		}
	}

	if (!critical2) {
		streamer_fetchData(stream2);
		return 0;
	} else {
		if (stream1 == streamer_lastStreamLoaded)
			streamer_fetchData(stream1);
		else
			streamer_fetchData(stream2);
		return 0;
	}
}

// Validated
int *streamer_reAllocReadBuffer(iMUSEStream *streamPtr, unsigned int reallocSize) {
	int size = streamPtr->loadIndex - streamPtr->readIndex;

	if (size < 0)
		size += streamPtr->bufFreeSize;
	if (size < reallocSize || streamPtr->maxRead < reallocSize)
		return 0;

	if (streamPtr->bufFreeSize - streamPtr->readIndex < reallocSize) {
		memcpy(streamPtr->buf + streamPtr->bufFreeSize, streamPtr->buf, reallocSize - streamPtr->bufFreeSize + streamPtr->readIndex + 4);
	}

	int readIndex_tmp = streamPtr->readIndex;
	int ptr = streamPtr->buf + streamPtr->readIndex;
	streamPtr->readIndex += reallocSize;
	if (streamPtr->bufFreeSize <= readIndex_tmp + reallocSize)
		streamPtr->readIndex = readIndex_tmp + reallocSize - streamPtr->bufFreeSize;
	
	return ptr;
}

// Validated
int *streamer_copyBufferAbsolute(iMUSEStream *streamPtr, int offset, int size) {
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset + size > value || streamPtr->maxRead < size)
		return 0;
	int offsetReadIndex = offset + streamPtr->readIndex;
	if (offset + streamPtr->readIndex >= streamPtr->bufFreeSize) {
		offsetReadIndex -= streamPtr->bufFreeSize;
	}

	if (streamPtr->bufFreeSize - offsetReadIndex < size) {
		memcpy(streamPtr->buf + streamPtr->bufFreeSize, streamPtr->buf, size + offsetReadIndex - streamPtr->bufFreeSize);
	}

	return streamPtr->buf + offsetReadIndex;
}

// Validated
int streamer_setIndex1(iMUSEStream *streamPtr, unsigned int offset) {
	streamer_bailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (offset > value)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->readIndex = offsetReadIndex;
	if (streamPtr->bufFreeSize <= offsetReadIndex) {
		streamPtr->readIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

// Validated
int streamer_setIndex2(iMUSEStream *streamPtr, unsigned int offset) {
	streamer_bailFlag = 1;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	if (value < offset)
		return -1;
	int offsetReadIndex = streamPtr->readIndex + offset;
	streamPtr->loadIndex = offsetReadIndex;
	if (offsetReadIndex >= streamPtr->bufFreeSize) {
		streamPtr->loadIndex = offsetReadIndex - streamPtr->bufFreeSize;
	}
	return 0;
}

// Validated
int streamer_getFreeBuffer(iMUSEStream *streamPtr) {
	int freeBufferSize = streamPtr->loadIndex - streamPtr->readIndex;
	if (freeBufferSize < 0)
		freeBufferSize += streamPtr->bufFreeSize;
	return freeBufferSize;
}

// Validated
int streamer_setSoundToStreamWithCurrentOffset(iMUSEStream *streamPtr, int soundId, int currentOffset) {
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

// Validated
int streamer_queryStream(iMUSEStream *streamPtr, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	dispatch_predictFirstStream();
	*bufSize = streamPtr->bufFreeSize;
	*criticalSize = streamPtr->criticalSize;
	int value = streamPtr->loadIndex - streamPtr->readIndex;
	if (value < 0)
		value += streamPtr->bufFreeSize;
	*freeSpace = value;
	*paused = streamPtr->paused;
	return 0;
}

// Validated (appears to be used for IACT blocks)
int streamer_feedStream(iMUSEStream *streamPtr, uint8 *srcBuf, unsigned int sizeToFeed, int paused) {
	int size = streamPtr->loadIndex - streamPtr->readIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;

	if (sizeToFeed > size - 4) {
		printf("ERR: FeedStream() buffer overflow...");
		streamer_bailFlag = 1;

		int newTentativeSize = sizeToFeed - size - 4;
		newTentativeSize -= newTentativeSize % 12 + 12;
		size = streamPtr->loadIndex - streamPtr->readIndex;
		if (size < 0)
			size += streamPtr->bufFreeSize;
		if (size >= newTentativeSize) {
			streamPtr->readIndex = newTentativeSize + streamPtr->bufFreeSize;
			if (streamPtr->bufFreeSize <= streamPtr->readIndex)
				streamPtr->readIndex -= streamPtr->bufFreeSize;
		}
	}
	while (sizeToFeed > 0) {
		int loadIndex = streamPtr->loadIndex;
		size = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (size >= sizeToFeed) {
			size = sizeToFeed;
		}
		sizeToFeed -= size;
		memcpy(&streamPtr->buf[streamPtr->loadIndex], srcBuf, size);
		srcBuf += size;

		int val = size + streamPtr->loadIndex;
		streamPtr->curOffset += size;
		streamPtr->loadIndex += size;
		if (val >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = val - streamPtr->bufFreeSize;
		}
	} while (sizeToFeed > 0);

	streamPtr->paused = paused;
	return 0;
}

// Validated
int streamer_fetchData(iMUSEStream *streamPtr) {
	if (streamPtr->endOffset == 0) {
		streamPtr->endOffset = files_seek(streamPtr->soundId, 0, SEEK_END);
	}

	int size = streamPtr->loadIndex - streamPtr->readIndex;
	if (size <= 0)
		size += streamPtr->bufFreeSize;

	int loadSize = streamPtr->loadSize; // For music that's 11000 bytes
	int remainingSize = streamPtr->endOffset - streamPtr->curOffset;
	
	if (loadSize >= size - 4) {
		loadSize = size - 4;
	}

	if (loadSize >= remainingSize) {
		loadSize = remainingSize;
	}

	if (remainingSize <= 0) {
		streamPtr->paused = 1;

		// Pad the buffer
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
		streamPtr->buf[streamPtr->loadIndex] = 127;
		streamPtr->loadIndex++;
	}

	int actualAmount;
	int requestedAmount;

	do {
		if (loadSize <= 0)
			return 0;

		requestedAmount = streamPtr->bufFreeSize - streamPtr->loadIndex;
		if (requestedAmount >= loadSize) {
			requestedAmount = loadSize;
		}

		if (files_seek(streamPtr->soundId, streamPtr->curOffset, SEEK_SET) != streamPtr->curOffset) {
			warning("ERR: Invalid seek in streamer...");
			streamPtr->paused = 1;
			return 0;
		}

		streamer_bailFlag = 0;
		waveapi_decreaseSlice();
		actualAmount = files_read(streamPtr->soundId, &streamPtr->buf[streamPtr->loadIndex], requestedAmount);
		waveapi_decreaseSlice();
		if (streamer_bailFlag != 0)
			return 0;

		loadSize -= actualAmount;
		streamPtr->curOffset += actualAmount;
		streamer_lastStreamLoaded = streamPtr;

		streamPtr->loadIndex += actualAmount;
		if (streamPtr->loadIndex + actualAmount >= streamPtr->bufFreeSize) {
			streamPtr->loadIndex = streamPtr->loadIndex + actualAmount - streamPtr->bufFreeSize;
		}
	} while (actualAmount == requestedAmount);

	printf("ERR: unable to load correct amount (req=%lu,act=%lu)...", requestedAmount, actualAmount);
	streamer_lastStreamLoaded = NULL;
	return 0;
}
