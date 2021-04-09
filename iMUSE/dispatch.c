#include "dispatch.h"

// Almost validated, check if there has to be a separate malloc for dispatch_smallFadeBufs; edit dispatch_free accordingly
int dispatch_moduleInit(iMUSEInitData *initDataptr) {
	dispatch_initDataPtr = initDataptr;
	dispatch_buf = malloc(SMALL_FADES * SMALL_FADE_DIM + LARGE_FADE_DIM * LARGE_FADES);
	
	if (dispatch_buf) {
		dispatch_largeFadeBufs = dispatch_buf;
		dispatch_smallFadeBufs = dispatch_buf + (LARGE_FADE_DIM * LARGE_FADES);

		for (int i = 0; i < LARGE_FADES; i++) {
			dispatch_largeFadeFlags[i] = 0;
		}

		for (int i = 0; i < SMALL_FADES; i++) {
			dispatch_smallFadeFlags[i] = 0;
		}

		for (int i = 0; i < MAX_STREAMZONES; i++) {
			streamZones[i].useFlag = 0;
		}
	} else {
		printf("ERR:in dispatch allocating buffers...\n");
		return -1;
	}
	
	return 0;
}

// Validated
iMUSEDispatch *dispatch_getTrackById(int trackId) {
	return &dispatches[trackId];
}

// Validated
int dispatch_save(int dst, int size) {
	if (size < 8832)
		return -5;
	memcpy((void *)dst, dispatches, 8832);
	return 8832;
}

// Validated
int dispatch_restore(const void *src) {
	memcpy(dispatches, src, 8832);

	for (int i = 0; i < LARGE_FADES; i++) {
		dispatch_largeFadeFlags[i] = 0;
	}

	for (int i = 0; i < SMALL_FADES; i++) {
		dispatch_smallFadeFlags[i] = 0;
	}

	for (int i = 0; i < MAX_STREAMZONES; i++) {
		streamZones[i].useFlag = 0;
	}

	return 8832;
}

// Almost validated, check if the list related code works
int dispatch_allocStreamZones() {
	iMUSEDispatch *curDispatchPtr;
	iMUSEStream *curAllocatedStream;
	iMUSEStreamZone *curStreamZone;
	iMUSEStreamZone *curStreamZoneList;

	curDispatchPtr = dispatches;
	for (int i = 0; i < MAX_TRACKS; i++) {
		curDispatchPtr = &dispatches[i];
		curDispatchPtr->fadeBuf = 0;
		if (curDispatchPtr->trackPtr->soundId && curDispatchPtr->streamPtr) {

			// Try allocating the stream
			curAllocatedStream = streamer_alloc(curDispatchPtr->trackPtr->soundId, curDispatchPtr->streamBufID, 0x2000u);
			curDispatchPtr->streamPtr = curAllocatedStream;

			if (curAllocatedStream) {
				streamer_setSoundToStreamWithCurrentOffset(curAllocatedStream, curDispatchPtr->trackPtr->soundId, curDispatchPtr->currentOffset);
				if (curDispatchPtr->audioRemaining) {

					// Try finding a stream zone which is not in use
					curStreamZone = NULL;
					for (int j = 0; i < MAX_STREAMZONES; i++) {
						if (streamZones[j].useFlag == 0) {
							curStreamZone = &streamZones[j];
						}
					}

					if (!curStreamZone) {
						printf("ERR: out of streamZones...\n");
						curStreamZoneList = (iMUSEStreamZone *)&curDispatchPtr->streamZoneList;
						curDispatchPtr->streamZoneList = 0;
					} else {
						curStreamZoneList = (iMUSEStreamZone *)&curDispatchPtr->streamZoneList;
						curStreamZone->prev = 0;
						curStreamZone->next = 0;
						curStreamZone->useFlag = 1;
						curStreamZone->offset = 0;
						curStreamZone->size = 0;
						curStreamZone->fadeFlag = 0;
						curDispatchPtr->streamZoneList = curStreamZone;
					}

					if (curStreamZoneList->prev) {
						curStreamZoneList->prev->offset = curDispatchPtr->currentOffset;
						curStreamZoneList->prev->size = 0;
						curStreamZoneList->prev->fadeFlag = 0;
					} else {
						printf("ERR: unable to alloc zone during restore...\n");
					}
				}
			} else {
				printf("ERR: unable to start stream during restore...\n");
			}
		}
	}
	return 0;
}

// Almost validated, check if the list removal code works
int dispatch_alloc(iMUSETrack *trackPtr, int priority) {
	iMUSEDispatch *trackDispatch;
	iMUSEDispatch *dispatchToDeallocate;
	iMUSEStreamZone **streamZoneList;
	int getMapResult;
	int *fadeBuf;

	trackDispatch = trackPtr->dispatchPtr;
	trackDispatch->currentOffset = 0;
	trackDispatch->audioRemaining = 0;
	trackDispatch->map[0] = NULL;
	trackDispatch->fadeBuf = 0;
	if (priority) {
		trackDispatch->streamPtr = streamer_alloc(trackPtr->soundId, priority, 0x2000u);
		if (!trackDispatch->streamPtr) {
			printf("ERR: unable to alloc stream...");
			return -1;
		}
		trackDispatch->streamBufID = priority;
		trackDispatch->streamZoneList = 0;
		trackDispatch->streamErrFlag = 0;
	} else {
		trackDispatch->streamPtr = 0;
	}

	getMapResult = dispatch_getNextMapEvent(trackDispatch);
	if (!getMapResult || getMapResult == -3)
		return 0;

	printf("ERR: problem starting sound in dispatch...\n");

	// Remove streamZones from list
	dispatchToDeallocate = trackDispatch->trackPtr->dispatchPtr;
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = &dispatchToDeallocate->streamZoneList;
		streamer_clearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				(*streamZoneList)->useFlag = 0;
				iMUSE_removeItemFromList((iMUSETrack **)&dispatchToDeallocate->streamZoneList, (iMUSETrack *)*streamZoneList);
			} while (*streamZoneList);
		}
	}

	fadeBuf = dispatchToDeallocate->fadeBuf;
	if (!fadeBuf)
		return -1;

	// Mark the fade corresponding to our fadeBuf as unused
	// Check between the large fades 
	int ptrCtr = 0;
	for (int i = 0, ptrCtr = 0; 
		i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM*LARGE_FADES; 
		i++, ptrCtr += LARGE_FADE_DIM) {

		if (dispatch_largeFadeBufs + (LARGE_FADE_DIM*(i+1)) == fadeBuf) { // Found it!
			if (dispatch_largeFadeFlags[i] == 0) {
				printf("ERR: redundant large fade buf de-allocation...\n");
			}
			dispatch_largeFadeFlags[i] = 0;
			return -1;
		}
	}

	// Check between the small fades 
	ptrCtr = 0;
	for (int j = 0, ptrCtr = 0;
		j < SMALL_FADES, ptrCtr < SMALL_FADE_DIM*SMALL_FADES;
		j++, ptrCtr += SMALL_FADE_DIM) {

		if (dispatch_largeFadeBufs + (SMALL_FADE_DIM*(j + 1)) == fadeBuf) { // Found it!
			if (dispatch_smallFadeFlags[j] == 0) {
				printf("ERR: redundant small fade buf de-allocation...\n");
			}
			dispatch_smallFadeFlags[j] = 0;
			return -1;
		}
	}

	printf("ERR: couldn't find fade buf to de-allocate...\n");
	return -1;
}

// Almost validated, check if the list removal code works
int dispatch_release(iMUSETrack *trackPtr) {
	iMUSEDispatch *dispatchToDeallocate;
	iMUSEStreamZone **streamZoneList;
	int *fadeBuf;

	dispatchToDeallocate = trackPtr->dispatchPtr;

	// Remove streamZones from list
	if (dispatchToDeallocate->streamPtr) {
		streamZoneList = &dispatchToDeallocate->streamZoneList;
		streamer_clearSoundInStream(dispatchToDeallocate->streamPtr);
		if (dispatchToDeallocate->streamZoneList) {
			do {
				(*streamZoneList)->useFlag = 0;
				iMUSE_removeItemFromList((iMUSETrack **)&dispatchToDeallocate->streamZoneList, (iMUSETrack *)*streamZoneList);
			} while (*streamZoneList);
		}
	}

	fadeBuf = dispatchToDeallocate->fadeBuf;
	if (!fadeBuf)
		return 0;

	// Mark the fade corresponding to our fadeBuf as unused
	// Check between the large fades 
	int ptrCtr = 0;
	for (int i = 0, ptrCtr = 0;
		i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM*LARGE_FADES;
		i++, ptrCtr += LARGE_FADE_DIM) {

		if (dispatch_largeFadeBufs + (LARGE_FADE_DIM*(i)) == fadeBuf) { // Found it!
			if (dispatch_largeFadeFlags[i] == 0) {
				printf("ERR: redundant large fade buf de-allocation...\n");
			}
			dispatch_largeFadeFlags[i] = 0;
			return 0;
		}
	}

	// Check between the small fades 
	ptrCtr = 0;
	for (int j = 0, ptrCtr = 0;
		j < SMALL_FADES, ptrCtr < SMALL_FADE_DIM*SMALL_FADES;
		j++, ptrCtr += SMALL_FADE_DIM) {

		if (dispatch_smallFadeBufs + (SMALL_FADE_DIM*(j)) == fadeBuf) { // Found it!
			if (dispatch_smallFadeFlags[j] == 0) {
				printf("ERR: redundant small fade buf de-allocation...\n");
			}
			dispatch_smallFadeFlags[j] = 0;
			return 0;
		}
	}

	printf("ERR: couldn't find fade buf to de-allocate...\n");
	return 0;
}

// Almost validated
// list stuff blah blah
int dispatch_switchStream(int oldSoundId, int newSoundId, int fadeLength, int unusedFadeSyncFlag, int offsetFadeSyncFlag) {
	int effFadeLen;
	unsigned int strZnSize;
	int alignmentModDividend;
	iMUSEDispatch *curDispatch;
	int ptrCtr;
	int fadeBuffer;
	unsigned int effFadeSize;
	int getMapResult;

	effFadeLen = fadeLength;

	if (fadeLength > 2000)
		effFadeLen = 2000;
	
	if (tracks_trackCount <= 0) {
		printf("ERR: DpSwitchStream() couldn't find sound...\n");
		return -1;
	}

	for (int i = 0; i <= tracks_trackCount; i++) {
		if (i >= tracks_trackCount) {
			printf("ERR: DpSwitchStream() couldn't find sound...\n");
			return -1;
		}

		if (oldSoundId && curDispatch->trackPtr->soundId == oldSoundId && curDispatch->streamPtr) {
			curDispatch = &dispatches[i];
			break;
		}
	}

	if (curDispatch->streamZoneList) {
		if (!curDispatch->wordSize) {
			printf("ERR: DpSwitchStream() found streamZoneList but null wordSize...\n");
			return -1;
		}

		fadeBuffer = curDispatch->fadeBuf;
		
		if (fadeBuffer) {
			ptrCtr = 0;

			// Mark the fade corresponding to our fadeBuf as unused
			ptrCtr = 0;
			for (int i = 0, ptrCtr = 0;
				i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM*LARGE_FADES;
				i++, ptrCtr += LARGE_FADE_DIM) {

				if (dispatch_largeFadeBufs + (LARGE_FADE_DIM*(i)) == fadeBuffer) {
					if (dispatch_largeFadeFlags[i] == 0) {
						printf("ERR: redundant large fade buf de-allocation...\n");
					}
					dispatch_largeFadeFlags[i] = 0;
					break;
				}
			}

			if (ptrCtr + dispatch_largeFadeBufs != fadeBuffer) {
				for (int j = 0, ptrCtr = 0;
					j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM*SMALL_FADES;
					j++, ptrCtr += SMALL_FADE_DIM) {

					if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
						printf("ERR: couldn't find fade buf to de-allocate...\n");
						break;
					}

					if (dispatch_smallFadeBufs + (SMALL_FADE_DIM*(j)) == fadeBuffer) {
						if (dispatch_smallFadeFlags[j] == 0) {
							printf("ERR: redundant small fade buf de-allocation...\n");
						}
						dispatch_smallFadeFlags[j] = 0;
						break;
					}
				}
			}
		}

		dispatch_fadeSize = (curDispatch->channelCount * curDispatch->wordSize * ((effFadeLen * curDispatch->sampleRate / 1000) & 0xFFFFFFFE)) / 8;
		
		strZnSize = curDispatch->streamZoneList->size;
		if (strZnSize >= dispatch_fadeSize)
			strZnSize = dispatch_fadeSize;
		dispatch_fadeSize = strZnSize;
		
		// Validate and adjust the fade dispatch size further;
		// this should correctly align the dispatch size to avoid starting a fade without
		// inverting the stereo image by mistake
		alignmentModDividend = curDispatch->channelCount * (curDispatch->wordSize == 8 ? 1 : 3);
		if (alignmentModDividend)
			dispatch_fadeSize -= dispatch_fadeSize % alignmentModDividend;
		else
			printf("ERR:ValidateFadeSize() tried mod by 0...\n");
		
		if (dispatch_fadeSize > LARGE_FADE_DIM) {
			printf("WARNING: requested fade too large (%lu)...\n", dispatch_fadeSize);
			dispatch_fadeSize = LARGE_FADE_DIM;
		}

		if (dispatch_fadeSize <= SMALL_FADE_DIM) { // Small fade
			for (int i = 0; i <= SMALL_FADES; i++) {
				if (i == SMALL_FADES) {
					printf("ERR: couldn't allocate small fade buf...\n");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!dispatch_smallFadeFlags[i]) {
					dispatch_smallFadeFlags[i] = 1;
					curDispatch->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
					break;
				}
			}			
		} else { // Large fade
			for (int i = 0; i <= LARGE_FADES; i++) {
				if (i == LARGE_FADES) {
					printf("ERR: couldn't allocate large fade buf...\n");
					curDispatch->fadeBuf = NULL;
					break;
				}

				if (!dispatch_largeFadeFlags[i]) {
					dispatch_largeFadeFlags[i] = 1;
					curDispatch->fadeBuf = dispatch_largeFadeFlags + LARGE_FADE_DIM * i;
					break;
				}
			}

			// Fallback to a small fade if large fades are unavailable
			if (!curDispatch->fadeBuf) {
				for (int i = 0; i <= SMALL_FADES; i++) {
					if (i == SMALL_FADES) {
						printf("ERR: couldn't allocate small fade buf...\n");
						curDispatch->fadeBuf = NULL;
						break;
					}

					if (!dispatch_smallFadeFlags[i]) {
						dispatch_smallFadeFlags[i] = 1;
						curDispatch->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
						break;
					}
				}
			}
		}

		// If we were able to allocate a fade, set up a fade out for the old sound.
		// We'll copy data from the stream buffer to the fade buffer
		// It appears that The DIG (at least the Windows 95 version)
		// doesn't do fade-ins for the new sound!
		if (curDispatch->fadeBuf) {
			curDispatch->fadeOffset = 0;
			curDispatch->fadeRemaining = 0;
			curDispatch->fadeWordSize = curDispatch->wordSize;
			curDispatch->fadeSampleRate = curDispatch->sampleRate;
			curDispatch->fadeChannelCount = curDispatch->channelCount;
			curDispatch->fadeSyncFlag = unusedFadeSyncFlag | offsetFadeSyncFlag;
			curDispatch->fadeSyncDelta = 0;
			curDispatch->fadeVol = MAX_FADE_VOLUME;
			curDispatch->fadeSlope = 0;

			while (curDispatch->fadeRemaining < dispatch_fadeSize) {
				effFadeSize = dispatch_fadeSize - curDispatch->fadeRemaining;
				if ((dispatch_fadeSize - curDispatch->fadeRemaining) >= 0x2000)
					effFadeSize = 0x2000;
					
				memcpy(
					(void *)(curDispatch->fadeBuf + curDispatch->fadeRemaining), 
					(const void *)streamer_reAllocReadBuffer(curDispatch->streamPtr, effFadeSize), 
					effFadeSize);

				curDispatch->fadeRemaining = effFadeSize + curDispatch->fadeRemaining;
			}
		} else {
			printf("WARNING:DpSwitchStream() couldn't alloc fade buf...\n");
		}
	}

	// Clear fades and triggers for the old newSoundId
	fades_clearFadeStatus(curDispatch->trackPtr->soundId, -1);
	triggers_clearTrigger(curDispatch->trackPtr->soundId, -1, -1);

	// Setup the new newSoundId
	curDispatch->trackPtr->soundId = newSoundId;

	streamer_setIndex1(curDispatch->streamPtr, streamer_getFreeBuffer(curDispatch->streamPtr));

	if (offsetFadeSyncFlag && curDispatch->streamZoneList) {
		// Start the newSoundId from an offset
		streamer_setSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, curDispatch->currentOffset);
		while (curDispatch->streamZoneList->next) {
			curDispatch->streamZoneList->next->useFlag = 0;
			iMUSE_removeItemFromList((iMUSETrack **)&curDispatch->streamZoneList->next, (iMUSETrack *)curDispatch->streamZoneList->next);
		}
		curDispatch->streamZoneList->size = 0;
		
		return 0;
	} else {
		// Start the newSoundId from the beginning
		streamer_setSoundToStreamWithCurrentOffset(curDispatch->streamPtr, newSoundId, 0);
		
		while (curDispatch->streamZoneList) {
			curDispatch->streamZoneList->useFlag = 0;
			iMUSE_removeItemFromList((iMUSETrack **)&curDispatch->streamZoneList, (iMUSETrack *)curDispatch->streamZoneList);
		}

		curDispatch->currentOffset = 0;
		curDispatch->audioRemaining = 0;
		curDispatch->map[0] = NULL;

		// Sanity check: make sure we correctly switched 
		// the stream and getNextMapEvent gives an error
		getMapResult = dispatch_getNextMapEvent(curDispatch);
		if (!getMapResult || getMapResult == -3) {
			return 0;
		} else {
			printf("ERR: problem switching stream in dispatch...\n");
			tracks_clear(curDispatch->trackPtr);
			return -1;
		}
	}
}

// Validated
// make more comments
void dispatch_processDispatches(iMUSETrack *trackPtr, int feedSize, int sampleRate) {
	iMUSEDispatch *dispatchPtr;
	int inFrameCount;
	int effFeedSize;
	int effWordSize;
	int effRemainingAudio;
	int effRemainingFade;
	int getNextMapEventResult;
	int mixVolume;
	int ptrCtr;
	int elapsedFadeDelta;
	uint8 *srcBuf; 
	uint8 *soundAddrData;
	int mixStartingPoint;

	dispatchPtr = trackPtr->dispatchPtr;
	if (dispatchPtr->streamPtr && dispatchPtr->streamZoneList)
		dispatch_predictStream(dispatchPtr);

	// If there's a fade
	if (dispatchPtr->fadeBuf) {
		inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);

		if (inFrameCount >= dispatchPtr->fadeSampleRate * feedSize / sampleRate) {
			inFrameCount = dispatchPtr->fadeSampleRate * feedSize / sampleRate;
			effFeedSize = feedSize;
		} else {
			effFeedSize = sampleRate * inFrameCount / dispatchPtr->fadeSampleRate;
		}

		if (dispatchPtr->fadeWordSize == 12 && dispatchPtr->fadeChannelCount == 1)
			inFrameCount &= 0xFFFFFFFE;

		// If the fade is still going on
		if (inFrameCount) {
			mixVolume = (((dispatchPtr->fadeVol / 65536) + 1) * dispatchPtr->trackPtr->effVol) / 128;

			// Update fadeVol
			effRemainingFade = ((dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) * inFrameCount) / 8;
			dispatchPtr->fadeVol += effRemainingFade * dispatchPtr->fadeSlope;

			// Clamp fadeVol between 0 and MAX_FADE_VOLUME
			if (dispatchPtr->fadeVol + effRemainingFade * dispatchPtr->fadeSlope < 0)
				dispatchPtr->fadeVol = 0;
			if (dispatchPtr->fadeVol > MAX_FADE_VOLUME)
				dispatchPtr->fadeVol = MAX_FADE_VOLUME;

			// Send it all to the mixer
			srcBuf = (uint8 *)(dispatchPtr->fadeBuf + dispatchPtr->fadeOffset);

			mixer_mix(
				srcBuf,
				inFrameCount,
				dispatchPtr->fadeWordSize,
				dispatchPtr->fadeChannelCount,
				effFeedSize,
				0,
				mixVolume,
				trackPtr->pan);

			dispatchPtr->fadeOffset += effRemainingFade;
			dispatchPtr->fadeRemaining -= effRemainingFade;

			// Deallocate fade if it ended
			if (!dispatchPtr->fadeRemaining) {

				ptrCtr = 0;
				for (int i = 0, ptrCtr = 0;
					i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
					i++, ptrCtr += LARGE_FADE_DIM) {

					if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
						if (dispatch_largeFadeFlags[i] == 0) {
							printf("ERR: redundant large fade buf de-allocation...\n");
						}
						dispatch_largeFadeFlags[i] = 0;
						break;
					}
				}

				if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
					for (int j = 0, ptrCtr = 0;
						j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
						j++, ptrCtr += SMALL_FADE_DIM) {

						if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
							printf("ERR: couldn't find fade buf to de-allocate...\n");
							break;
						}

						if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
							if (dispatch_smallFadeFlags[j] == 0) {
								printf("ERR: redundant small fade buf de-allocation...\n");
							}
							dispatch_smallFadeFlags[j] = 0;
							break;
						}
					}
				}
			}
		} else {
			printf("WARNING:fade ends with incomplete frame (or odd 12-bit mono frame)...\n");

			// Fade ended, deallocate it
			ptrCtr = 0;
			for (int i = 0, ptrCtr = 0;
				i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
				i++, ptrCtr += LARGE_FADE_DIM) {

				if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
					if (dispatch_largeFadeFlags[i] == 0) {
						printf("ERR: redundant large fade buf de-allocation...\n");
					}
					dispatch_largeFadeFlags[i] = 0;
					break;
				}
			}

			if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
				for (int j = 0, ptrCtr = 0;
					j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
					j++, ptrCtr += SMALL_FADE_DIM) {

					if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
						printf("ERR: couldn't find fade buf to de-allocate...\n");
						break;
					}

					if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
						if (dispatch_smallFadeFlags[j] == 0) {
							printf("ERR: redundant small fade buf de-allocation...\n");
						}
						dispatch_smallFadeFlags[j] = 0;
						break;
					}
				}
			}
		}
		
		if (!dispatchPtr->fadeRemaining)
			dispatchPtr->fadeBuf = NULL;
	}

	mixStartingPoint = 0;

	while (1) {
		if (!dispatchPtr->audioRemaining) {
			dispatch_fadeStartedFlag = 0;
			getNextMapEventResult = dispatch_getNextMapEvent(dispatchPtr);
			
			if (getNextMapEventResult)
				break;
			
			if (dispatch_fadeStartedFlag) {
				inFrameCount = 8 * dispatchPtr->fadeRemaining / (dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount);
				
				if (inFrameCount >= dispatchPtr->fadeSampleRate * feedSize / sampleRate) {
					inFrameCount = dispatchPtr->fadeSampleRate * feedSize / sampleRate;
					effFeedSize = feedSize;
				} else {
					effFeedSize = sampleRate * inFrameCount / dispatchPtr->fadeSampleRate;
				}
				
				if (dispatchPtr->fadeWordSize == 12 && dispatchPtr->fadeChannelCount == 1)
					inFrameCount &= 0xFFFFFFFE;

				if (!inFrameCount)
					printf("WARNING:fade ends with incomplete frame (or odd 12-bit mono frame)...\n");
				
				effRemainingFade = (inFrameCount * dispatchPtr->fadeWordSize * dispatchPtr->fadeChannelCount) / 8;
				mixVolume = (((dispatchPtr->fadeVol / 65536) + 1) * dispatchPtr->trackPtr->effVol) / 128;

				// Update fadeVol
				dispatchPtr->fadeVol += effRemainingFade * dispatchPtr->fadeSlope;
				
				// Clamp fadeVol between 0 and MAX_FADE_VOLUME
				if (dispatchPtr->fadeVol < 0)
					dispatchPtr->fadeVol = 0;
				if (dispatchPtr->fadeVol > MAX_FADE_VOLUME)
					dispatchPtr->fadeVol = MAX_FADE_VOLUME;
				
				// Send it all to the mixer
				srcBuf = (uint8 *)(dispatchPtr->fadeBuf + dispatchPtr->fadeOffset);

				mixer_mix(
					srcBuf,
					inFrameCount,
					dispatchPtr->fadeWordSize,
					dispatchPtr->fadeChannelCount,
					effFeedSize,
					mixStartingPoint,
					mixVolume,
					trackPtr->pan);

				dispatchPtr->fadeOffset += effRemainingFade;
				dispatchPtr->fadeRemaining -= effRemainingFade;

				if (!dispatchPtr->fadeRemaining) {
					// Fade ended, deallocate it
					ptrCtr = 0;
					for (int i = 0, ptrCtr = 0;
						i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
						i++, ptrCtr += LARGE_FADE_DIM) {

						if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
							if (dispatch_largeFadeFlags[i] == 0) {
								printf("ERR: redundant large fade buf de-allocation...\n");
							}
							dispatch_largeFadeFlags[i] = 0;
							break;
						}
					}

					if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
						for (int j = 0, ptrCtr = 0;
							j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
							j++, ptrCtr += SMALL_FADE_DIM) {

							if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
								printf("ERR: couldn't find fade buf to de-allocate...\n");
								break;
							}

							if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
								if (dispatch_smallFadeFlags[j] == 0) {
									printf("ERR: redundant small fade buf de-allocation...\n");
								}
								dispatch_smallFadeFlags[j] = 0;
								break;
							}
						}
					}
				}
			}
		}

		if (!feedSize)
			return;

		effWordSize = dispatchPtr->channelCount * dispatchPtr->wordSize;
		inFrameCount = dispatchPtr->sampleRate * feedSize / sampleRate;

		if (inFrameCount <= (8 * dispatchPtr->audioRemaining / effWordSize)) {
			effFeedSize = feedSize;
		} else {
			inFrameCount = 8 * dispatchPtr->audioRemaining / effWordSize;
			effFeedSize = sampleRate * (8 * dispatchPtr->audioRemaining / effWordSize) / dispatchPtr->sampleRate;
		}
		
		if (dispatchPtr->wordSize == 12 && dispatchPtr->channelCount == 1)
			inFrameCount &= 0xFFFFFFFE;
		
		if (!inFrameCount) {
			printf("ERR:region ends with incomplete frame (or odd 12-bit mono frame)...\n");
			tracks_clear(trackPtr);
			return;
		}

		effRemainingAudio = (effWordSize * inFrameCount) / 8;
		
		if (dispatchPtr->streamPtr) {
			srcBuf = (uint8 *)streamer_reAllocReadBuffer(dispatchPtr->streamPtr, (effWordSize * inFrameCount) / 8);
			if (!srcBuf) {

				dispatchPtr->streamErrFlag = 1;
				if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
					dispatchPtr->fadeSyncDelta += feedSize;

				streamer_queryStream(
					dispatchPtr->streamPtr,
					&dispatch_curStreamBufSize,
					&dispatch_curStreamCriticalSize,
					&dispatch_curStreamFreeSpace,
					&dispatch_curStreamPaused);

				if (dispatch_curStreamPaused) {
					printf("WARNING: stopping starving paused stream...\n");
					tracks_clear(trackPtr);
				}

				return;
			}
			dispatchPtr->streamZoneList->offset += effRemainingAudio;
			dispatchPtr->streamZoneList->size -= effRemainingAudio;
			dispatchPtr->streamErrFlag = 0;
		} else {
			soundAddrData = files_getSoundAddrData(trackPtr->soundId);
			if (!soundAddrData) {
				printf("ERR: dispatch got NULL file addr...\n");
				return;
			}

			srcBuf = (uint8 *)(soundAddrData + dispatchPtr->currentOffset);
		}

		if (dispatchPtr->fadeBuf) {
			if (dispatchPtr->fadeSyncFlag) {
				if (dispatchPtr->fadeSyncDelta) {
					elapsedFadeDelta = effFeedSize;
					if (effFeedSize >= dispatchPtr->fadeSyncDelta)
						elapsedFadeDelta = dispatchPtr->fadeSyncDelta;

					dispatchPtr->fadeSyncDelta -= elapsedFadeDelta;
					effFeedSize -= elapsedFadeDelta;
					inFrameCount = effFeedSize * dispatchPtr->sampleRate / sampleRate;

					if (dispatchPtr->wordSize == 12 && dispatchPtr->channelCount == 1)
						inFrameCount &= 0xFFFFFFFE;
					
					srcBuf = &srcBuf[effRemainingAudio - ((dispatchPtr->wordSize * inFrameCount * dispatchPtr->channelCount) / 8)];
				}
			}
			
			if (dispatchPtr->fadeBuf) {
				mixVolume = (dispatchPtr->trackPtr->effVol * (128 - (dispatchPtr->fadeVol / 65536))) / 128;

				// Update the fadeSlope
				if (!dispatchPtr->fadeSlope) {
					effRemainingFade = dispatchPtr->fadeRemaining;
					if (effRemainingFade <= 1)
						effRemainingFade = 2;
					dispatchPtr->fadeSlope = - (MAX_FADE_VOLUME / effRemainingFade);
				}
			} else {
				mixVolume = trackPtr->effVol;
			}
		} else {
			mixVolume = trackPtr->effVol;
		}
		
		if (trackPtr->mailbox)
			mixer_setRadioChatter();

		mixer_mix(
			srcBuf, 
			inFrameCount, 
			dispatchPtr->wordSize, 
			dispatchPtr->channelCount, 
			effFeedSize, 
			mixStartingPoint, 
			mixVolume, 
			trackPtr->pan);

		mixer_clearRadioChatter();
		mixStartingPoint += effFeedSize;
		feedSize -= effFeedSize;

		dispatchPtr->currentOffset += effRemainingAudio;
		dispatchPtr->audioRemaining -= effRemainingAudio;
	}

	// Behavior of errors and STOP marker
	if (getNextMapEventResult == -1) 
		tracks_clear(trackPtr);

	if (dispatchPtr->fadeBuf && dispatchPtr->fadeSyncFlag)
		dispatchPtr->fadeSyncDelta += feedSize;
}

// Validated
int dispatch_predictFirstStream() {
	waveapi_increaseSlice();

	if (tracks_trackCount > 0) {
		for (int i = 0; i < tracks_trackCount; i++) {
			if (dispatches[i].trackPtr->soundId && dispatches[i].streamPtr && dispatches[i].streamZoneList)
				dispatch_predictStream(&dispatches[i]);
		}
	}

	return waveapi_decreaseSlice();
}

// Almost validated, fade allocation stuff to rename
// add more comments
int dispatch_getNextMapEvent(iMUSEDispatch *dispatchPtr) {
	int *dstMap;
	int *rawMap;
	iMUSEStreamZone *allStrZn;
	uint8 *copiedBuf;
	uint8 *soundAddrData;
	int restartDoubleLoop;
	int size;
	int *mapCurPos;
	int curOffset;
	int blockName;
	unsigned int effFadeSize;
	unsigned int elapsedFadeSize;
	int regionOffset;
	int ptrCtr;


	dstMap = &dispatchPtr->map;

	// If there's no map, fetch it
	if (dispatchPtr->map[0] != 'MAP ') {
		if (dispatchPtr->currentOffset) {
			printf("ERR: GetMap found offset but no map...\n");
			return -1;
		}

		rawMap = files_fetchMap(dispatchPtr->trackPtr->soundId);
		if (rawMap) {
			if (dispatch_convertMap(rawMap, (uint8 *)dstMap)) {
				printf("ERR: ConvertMap() failed...\n");
				return -1;
			}

			if (dispatchPtr->map[2] != 'FRMT') {
				printf("ERR: expected 'FRMT' at start of map...\n");
				return -1;
			}

			dispatchPtr->currentOffset = dispatchPtr->map[4];
			if (dispatchPtr->streamPtr) {
				if (streamer_getFreeBuffer(dispatchPtr->streamPtr)) {
					printf("ERR: GetMap() expected empty stream buf...\n");
					return -1;
				}

				if (dispatchPtr->streamZoneList) {
					printf("ERR: GetMap() expected null streamZoneList...\n");
					return -1;
				}

				allStrZn = dispatch_allocStreamZone();
				dispatchPtr->streamZoneList = allStrZn;
				if (!allStrZn) {
					printf("ERR: GetMap() couldn't alloc zone...\n");
					return -1;
				}

				allStrZn->offset = dispatchPtr->currentOffset;
				dispatchPtr->streamZoneList->size = 0;
				dispatchPtr->streamZoneList->fadeFlag = 0;
				streamer_setSoundToStreamWithCurrentOffset(
					dispatchPtr->streamPtr,
					dispatchPtr->trackPtr->soundId,
					dispatchPtr->currentOffset);
			}

		}

		if (dispatchPtr->streamPtr) {
			copiedBuf = (uint8 *)streamer_copyBufferAbsolute(dispatchPtr->streamPtr, 0, 0x10u);

			if (!copiedBuf) {
				return -3;
			}

			if (iMUSE_SWAP32(copiedBuf) == 'iMUS' && iMUSE_SWAP32(copiedBuf + 8) == 'MAP ') {
				size = iMUSE_SWAP32(copiedBuf + 12) + 24;
				if (!streamer_copyBufferAbsolute(dispatchPtr->streamPtr, 0, size)) {
					return -3;
				}
				rawMap = streamer_reAllocReadBuffer(dispatchPtr->streamPtr, size);
				if (!rawMap) {
					printf("ERR: stream read failed after view succeeded in GetMap()...\n");
					return -1;
				}

				dispatchPtr->currentOffset = size;
				if (dispatch_convertMap(rawMap + 8, (uint8 *)dstMap)) {
					printf("ERR: ConvertMap() failed...\n");
					return -1;
				}

				if (dispatchPtr->map[2] != 'FRMT') {
					printf("ERR: expected 'FRMT' at start of map...\n");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					printf("ERR: GetMap() expected data to follow map...\n");
					return -1;
				} else {
					if (dispatchPtr->streamZoneList) {
						printf("ERR: GetMap() expected null streamZoneList...\n");
						return -1;
					}

					dispatchPtr->streamZoneList = dispatch_allocStreamZone();
					if (!dispatchPtr->streamZoneList) {
						printf("ERR: GetMap() couldn't alloc zone...\n");
						return -1;
					}

					dispatchPtr->streamZoneList->offset = dispatchPtr->currentOffset;
					dispatchPtr->streamZoneList->size = streamer_getFreeBuffer(dispatchPtr->streamPtr);
					dispatchPtr->streamZoneList->fadeFlag = 0;
				}
			} else {
				printf("ERR: unrecognized file format in stream buf...\n");
				return -1;
			}

		} else {
			soundAddrData = files_getSoundAddrData(dispatchPtr->trackPtr->soundId);
			
			if (!soundAddrData) {
				printf("ERR: GetMap() couldn't get sound address...\n");
				return -1;
			}
			
			if (iMUSE_SWAP32(soundAddrData) == 'iMUS' && iMUSE_SWAP32(soundAddrData + 8) == 'MAP ') {
				dispatchPtr->currentOffset = iMUSE_SWAP32(soundAddrData + 12) + 24;
				if (dispatch_convertMap((soundAddrData + 8), (uint8 *)dstMap)) {
					printf("ERR: ConvertMap() failed...\n");
					return -1;
				}

				if (dispatchPtr->map[2] != 'FRMT') {
					printf("ERR: expected 'FRMT' at start of map...\n");
					return -1;
				}

				if (dispatchPtr->map[4] != dispatchPtr->currentOffset) {
					printf("ERR: GetMap() expected data to follow map...\n");
					return -1;
				}
			} else {
				printf("ERR: unrecognized file format in stream buf...\n");
				return -1;
			}
		}
	}

	if (dispatchPtr->audioRemaining
		|| dispatchPtr->streamPtr && dispatchPtr->streamZoneList->offset != dispatchPtr->currentOffset) {
		printf("ERR: navigation error in dispatch...\n");
		return -1;
	}

	mapCurPos = NULL;
	while (1) {
		curOffset = dispatchPtr->currentOffset;

		if (mapCurPos) {
			mapCurPos = (int *)((int8 *)mapCurPos + mapCurPos[1] + 8);
			if ((int8 *)&dispatchPtr->map[2] + dispatchPtr->map[1] > (int8 *)mapCurPos) {
				if (mapCurPos[2] != curOffset) {
					printf("ERR: GetNextMapEvent() no more events at offset %lu...\n", dispatchPtr->currentOffset);
					printf("ERR: no more map events at offset %lx...\n", dispatchPtr->currentOffset);
					return -1;
				}
			} else {
				printf("ERR: GetNextMapEvent() map overrun...\n");
				printf("ERR: no more map events at offset %lx...\n", dispatchPtr->currentOffset);
				return -1;
			}

		} else {
			// Init the current map position starting from the first block
			// (cells 0 and 1 are the string 'MAP ' and the map size respectively)
			mapCurPos = &dispatchPtr->map[2];

			// Search for the block with the same offset as ours
			while (mapCurPos[2] != curOffset) {	
				// Check if we've overrun the offset, to make sure 
				// that there actually is an event at our offset
				mapCurPos = (int *)((int8 *)mapCurPos + mapCurPos[1] + 8);

				if ((int8 *)&dispatchPtr->map[2] + dispatchPtr->map[1] <= (int8 *)mapCurPos) {
					printf("ERR: GetNextMapEvent() couldn't find event at offset %lu...\n", dispatchPtr->currentOffset);
					printf("ERR: no more map events at offset %lx...\n", dispatchPtr->currentOffset);
					return -1;
				}
			}
		}

		if (!mapCurPos) {
			printf("ERR: no more map events at offset %lx...\n", dispatchPtr->currentOffset);
			return -1;
		}

		blockName = mapCurPos[0];

		if (blockName == 'JUMP') {
			if (!iMUSE_checkHookId(&dispatchPtr->trackPtr->jumpHook, mapCurPos[4])) {
				// This is the right hookId, let's jump
				dispatchPtr->currentOffset = mapCurPos[3];
				if (dispatchPtr->streamPtr) {
					if (dispatchPtr->streamZoneList->size || !dispatchPtr->streamZoneList->next) {
						printf("ERR: failed to prepare for jump...\n");
						dispatch_parseJump(dispatchPtr, dispatchPtr->streamZoneList, mapCurPos, 1);
					}

					dispatchPtr->streamZoneList->useFlag = 0;
					iMUSE_removeItemFromList(
						(iMUSETrack **)&dispatchPtr->streamZoneList,
						(iMUSETrack *)dispatchPtr->streamZoneList);

					if (dispatchPtr->streamZoneList->fadeFlag) {
						if (dispatchPtr->fadeBuf) {
							// Mark the fade corresponding to our fadeBuf as unused
							ptrCtr = 0;
							for (int i = 0, ptrCtr = 0;
								i < LARGE_FADES, ptrCtr < LARGE_FADE_DIM * LARGE_FADES;
								i++, ptrCtr += LARGE_FADE_DIM) {

								if (dispatch_largeFadeBufs + (LARGE_FADE_DIM * i) == dispatchPtr->fadeBuf) {
									if (dispatch_largeFadeFlags[i] == 0) {
										printf("ERR: redundant large fade buf de-allocation...\n");
									}
									dispatch_largeFadeFlags[i] = 0;
									break;
								}
							}

							if (ptrCtr + dispatch_largeFadeBufs != dispatchPtr->fadeBuf) {
								for (int j = 0, ptrCtr = 0;
									j < SMALL_FADES, ptrCtr <= SMALL_FADE_DIM * SMALL_FADES;
									j++, ptrCtr += SMALL_FADE_DIM) {

									if (ptrCtr >= SMALL_FADE_DIM * SMALL_FADES) {
										printf("ERR: couldn't find fade buf to de-allocate...\n");
										break;
									}

									if (dispatch_smallFadeBufs + (SMALL_FADE_DIM * j) == dispatchPtr->fadeBuf) {
										if (dispatch_smallFadeFlags[j] == 0) {
											printf("ERR: redundant small fade buf de-allocation...\n");
										}
										dispatch_smallFadeFlags[j] = 0;
										break;
									}
								}
							}
						}

						dispatch_requestedFadeSize = dispatchPtr->streamZoneList->size;
						if (dispatch_requestedFadeSize > LARGE_FADE_DIM) {
							printf("WARNING: requested fade too large (%lu)...\n", dispatch_requestedFadeSize);
							dispatch_requestedFadeSize = LARGE_FADE_DIM;
						}

						if (dispatch_fadeSize <= SMALL_FADE_DIM) { // Small fade
							for (int i = 0; i <= SMALL_FADES; i++) {
								if (i == SMALL_FADES) {
									printf("ERR: couldn't allocate small fade buf...\n");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!dispatch_smallFadeFlags[i]) {
									dispatch_smallFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
									break;
								}
							}
						} else { // Large fade
							for (int i = 0; i <= LARGE_FADES; i++) {
								if (i == LARGE_FADES) {
									printf("ERR: couldn't allocate large fade buf...\n");
									dispatchPtr->fadeBuf = NULL;
									break;
								}

								if (!dispatch_largeFadeFlags[i]) {
									dispatch_largeFadeFlags[i] = 1;
									dispatchPtr->fadeBuf = dispatch_largeFadeFlags + LARGE_FADE_DIM * i;
									break;
								}
							}

							// Fallback to a small fade if large fades are unavailable
							if (!dispatchPtr->fadeBuf) {
								for (int i = 0; i <= SMALL_FADES; i++) {
									if (i == SMALL_FADES) {
										printf("ERR: couldn't allocate small fade buf...\n");
										dispatchPtr->fadeBuf = NULL;
										break;
									}

									if (!dispatch_smallFadeFlags[i]) {
										dispatch_smallFadeFlags[i] = 1;
										dispatchPtr->fadeBuf = dispatch_smallFadeBufs + SMALL_FADE_DIM * i;
										break;
									}
								}
							}
						}

						// If the fade buffer is allocated
						// set up the fade
						if (dispatchPtr->fadeBuf) {
							dispatchPtr->fadeWordSize = dispatchPtr->wordSize;
							dispatchPtr->fadeSampleRate = dispatchPtr->sampleRate;
							dispatchPtr->fadeChannelCount = dispatchPtr->channelCount;
							dispatchPtr->fadeOffset = 0;
							dispatchPtr->fadeRemaining = 0;
							dispatchPtr->fadeSyncFlag = 0;
							dispatchPtr->fadeSyncDelta = 0;
							dispatchPtr->fadeVol = MAX_FADE_VOLUME;
							dispatchPtr->fadeSlope = 0;

							if (dispatch_requestedFadeSize) {
								do {
									effFadeSize = dispatch_requestedFadeSize - dispatchPtr->fadeRemaining;
									if ((unsigned int)(dispatch_requestedFadeSize - dispatchPtr->fadeRemaining) >= 0x2000)
										effFadeSize = 0x2000;
										
									memcpy(
										(void *)(dispatchPtr->fadeBuf + dispatchPtr->fadeRemaining),
										(const void *)streamer_reAllocReadBuffer(dispatchPtr->streamPtr, effFadeSize),
										effFadeSize);

									elapsedFadeSize = effFadeSize + dispatchPtr->fadeRemaining;
									dispatchPtr->fadeRemaining = elapsedFadeSize;
								} while (dispatch_requestedFadeSize > elapsedFadeSize);
							}
								
							dispatch_fadeStartedFlag = 1;
						}

						dispatchPtr->streamZoneList->useFlag = 0;
						iMUSE_removeItemFromList(
							(iMUSETrack **)&dispatchPtr->streamZoneList,
							(iMUSETrack *)dispatchPtr->streamZoneList);
					}
				}

				mapCurPos = NULL;
				continue;
			}

			if (dispatchPtr->audioRemaining)
				return 0;
		} 

		if (blockName == 'FRMT') {
			dispatchPtr->wordSize = mapCurPos[4];
			dispatchPtr->sampleRate = mapCurPos[5];
			dispatchPtr->channelCount = mapCurPos[6];

			continue;
		}		
		
		if (blockName == 'REGN') {
			regionOffset = mapCurPos[2];
			if (regionOffset == dispatchPtr->currentOffset) {
				dispatchPtr->audioRemaining = mapCurPos[3];
				return 0;
			} else {
				printf("ERR: region offset %lu != currentOffset %lu...\n", regionOffset, dispatchPtr->currentOffset);
				return -1;
			}
		}
			
		if (blockName == 'STOP')
			return -1;

		if (blockName == 'TEXT') {
			char *marker = (char *)mapCurPos + 12;
			triggers_processTriggers(dispatchPtr->trackPtr->soundId, marker);
			if (dispatchPtr->audioRemaining)
				return 0;
		}

		printf("ERR: Unrecognized map event at offset %lx...\n", dispatchPtr->currentOffset);
		return -1;
	}

	// You should never be able to reach this return
	return 0;
}

// Validated
int dispatch_convertMap(uint8 *rawMap, uint8 *destMap) {
	int effMapSize;
	uint8 *mapCurPos; 
	int blockName;
	uint8 *blockSizePtr; 
	unsigned int blockSizeMin8;
	uint8 *firstChar;
	uint8 *otherChars;
	unsigned int remainingFieldsNum; 
	int bytesUntilEndOfMap; 
	uint8 *endOfMapPtr;

	if (iMUSE_SWAP32(rawMap) == 'MAP ') {
		bytesUntilEndOfMap = iMUSE_SWAP32(rawMap + 4);
		effMapSize = bytesUntilEndOfMap + 8;
		if (bytesUntilEndOfMap + 8 <= 1024) {
			memcpy(destMap, rawMap, effMapSize);
			
			// Fill (or rather, swap32) the fields:
			// - The 4 bytes string 'MAP '
			// - Size of the map
			*(uint16 *)destMap = iMUSE_SWAP32(destMap);
			*((uint16 *)destMap + 1) = IMUSE_UTILS_SWAP32(destMap + 4);
			
			mapCurPos = destMap + 8;
			endOfMapPtr = &destMap[bytesUntilEndOfMap + 8];

			// Swap32 the rest of the map
			while (mapCurPos < endOfMapPtr) {
				// Swap32 the 4 characters block name
				*(uint16 *)mapCurPos = iMUSE_SWAP32(mapCurPos);
				blockName = iMUSE_SWAP32(mapCurPos);

				// Advance and Swap32 the block size (minus 8) field
				blockSizePtr = mapCurPos + 4;
				blockSizeMin8 = iMUSE_SWAP32(blockSizePtr);
				*(uint16 *)blockSizePtr = blockSizeMin8;
				mapCurPos = blockSizePtr + 4;
					
				// Swapping32 a TEXT block is different:
				// it also contains single characters, so we skip them
				// since they're already good like this
				if (blockName == 'TEXT') {
					// Swap32 the block offset position
					*(uint16 *)mapCurPos = iMUSE_SWAP32(mapCurPos);

					// Skip the single characters
					firstChar = mapCurPos + 4;
					mapCurPos += 5;
					if (*firstChar) {
						do {
							otherChars = mapCurPos++;
						} while (*otherChars);
					}
				} else if ((blockSizeMin8 & 0xFFFFFFFC) != 0) { 
					// Basically divide by 4 to retrieve the number
					// of fields to swap
					remainingFieldsNum = blockSizeMin8 >> 2;

					// ...and swap them of course
					do {
						*(uint16 *)mapCurPos = iMUSE_SWAP32(mapCurPos);
						mapCurPos += 4;
						--remainingFieldsNum;
					} while (remainingFieldsNum);
				}
			} 

			// Just a sanity check to see if we've parsed the whole map,
			// nothing more and nothing less than that
			if (&destMap[bytesUntilEndOfMap - (uint16)mapCurPos] == (uint8 *)-8) {
				return 0;
			} else {
				printf("ERR: ConvertMap() converted wrong number of bytes...\n");
				return -1;
			}

		} else {
			printf("ERR: MAP TOO BIG (%lu)!!!...\n", effMapSize);
			return -1;
		}
	} else {
		printf("ERR: ConvertMap() got bogus map...\n");
		return -1;
	}

	return 0;
}

// Validated
void dispatch_predictStream(iMUSEDispatch *dispatch) {
	iMUSEStreamZone *szTmp;
	iMUSEStreamZone *lastStreamInList;
	iMUSEStreamZone *szList;
	int cumulativeStreamOffset; 
	int *curMapPlace;
	int *mapPlaceName;
	int *endOfMap;
	int bytesUntilNextPlace, mapPlaceHookId;
	unsigned int mapPlaceHookPosition;

	if (!dispatch->streamPtr || !dispatch->streamZoneList) {
		printf("ERR: null streamID or zoneList in PredictStream()...\n");
		return;
	}

	szTmp = dispatch->streamZoneList;

	// Get the offset which our stream is currently at
	cumulativeStreamOffset = 0;
	do {
		cumulativeStreamOffset += szTmp->size;
		lastStreamInList = szTmp;
		szTmp = szTmp->next;
	} while (szTmp);

	lastStreamInList->size += streamer_getFreeBuffer(dispatch->streamPtr) - cumulativeStreamOffset;
	szList = dispatch->streamZoneList;

	buff_hookid = dispatch->trackPtr->jumpHook;
	while (szList) {
		if (!szList->fadeFlag) {
			curMapPlace = &dispatch->map[2];
			endOfMap = (int *)((int8 *)&dispatch->map[2] + dispatch->map[1]);

			while (1) {
				// End of getNextMapEventResult, stream
				if (curMapPlace >= endOfMap) {
					curMapPlace = NULL;
					break;
				}

				mapPlaceName = curMapPlace[0];
				bytesUntilNextPlace = curMapPlace[1] + 8;

				if (mapPlaceName == 'JUMP') {
					// We assign these here, to avoid going out of bounds
					// on a place which doesn't have as many fields, like TEXT
					mapPlaceHookPosition = curMapPlace[4];
					mapPlaceHookId = curMapPlace[6];

					if (mapPlaceHookPosition > szList->offset && mapPlaceHookPosition <= szList->size + szList->offset) {
						// Break out of the loop if we have to JUMP
						if (!iMUSE_checkHookId(&buff_hookid, mapPlaceHookId))
							break;
					}
				}
				// Advance the getNextMapEventResult by bytesUntilNextPlace bytes
				curMapPlace = (int *)((int8 *)curMapPlace + bytesUntilNextPlace);
			}

			if (curMapPlace) {
				// This is where we should end up if iMUSE_checkHookId has been successful
				dispatch_parseJump(dispatch, szList, curMapPlace, 0);
			} else {
				// Otherwise this is where we end up when we reached the end of the getNextMapEventResult,
				// which means: if we don't have to jump, just play the next streamZone available
				if (szList->next) {
					cumulativeStreamOffset = szList->size;
					szTmp = dispatch->streamZoneList;
					while (szTmp != szList) {
						cumulativeStreamOffset += szTmp->size;
						szTmp = szTmp->next;
					}
					
					// Continue streaming the newSoundId from where we left off
					streamer_setIndex2(dispatch->streamPtr, cumulativeStreamOffset);

					// Remove che previous streamZone from the list, we don't need it anymore
					while (szList->next->prev) {
						szList->next->prev->useFlag = 0;
						iMUSE_removeItemFromList((iMUSETrack **)&szList->next, (iMUSETrack *)szList->next->prev);
					}

					streamer_setSoundToStreamWithCurrentOffset(
						dispatch->streamPtr,
						dispatch->trackPtr->soundId,
						szList->size + szList->offset);
				}
			}
		}
		szList = szList->next;
	}
}

// Almost validated, list stuff to check
// add more comments
void dispatch_parseJump(iMUSEDispatch *dispatchPtr, iMUSEStreamZone *streamZonePtr, int *jumpParamsFromMap, int calledFromGetMap) {
	int hookPosition, jumpDestination, fadeTime;
	iMUSEStreamZone *nextStreamZone;
	unsigned int alignmentModDividend;
	iMUSEStreamZone *zoneForJump;
	iMUSEStreamZone *zoneAfterJump;
	unsigned int streamOffset;
	iMUSEStreamZone *zoneCycle;

	/* jumpParamsFromMap format:
	   jumpParamsFromMap[0]: four bytes which form the string 'JUMP'
	   jumpParamsFromMap[1]: block size in bytes minus 8 (16 for a JUMP block like this; total == 24 bytes)
	   jumpParamsFromMap[2]: hook position
	   jumpParamsFromMap[3]: jump destination
	   jumpParamsFromMap[4]: hook ID
	   jumpParamsFromMap[5]: fade time in milliseconds
	*/

	hookPosition = jumpParamsFromMap[2];
	jumpDestination = jumpParamsFromMap[3];
	fadeTime = jumpParamsFromMap[5];

	if (streamZonePtr->size + streamZonePtr->offset == hookPosition) {
		nextStreamZone = streamZonePtr->next;
		if (nextStreamZone) {
			// If the next stream zone is already fading
			if (nextStreamZone->fadeFlag) {
				if (nextStreamZone->offset == hookPosition) {
					if (nextStreamZone->next) {
						if (nextStreamZone->next->offset == jumpDestination)
							return;
					}
				}
			} else {
				if (nextStreamZone->offset == jumpDestination)
					return;
			}
		}
	}

	// Maximum size of the dispatch for the fade (in bytes)
	dispatch_size = (dispatchPtr->wordSize
		* dispatchPtr->channelCount
		* ((dispatchPtr->sampleRate * fadeTime / 1000u) & 0xFFFFFFFE)) / 8;
	
	if (!calledFromGetMap) {
		if (dispatch_size + hookPosition - streamZonePtr->offset > streamZonePtr->size)
			return;
	}

	// If the effective fade dispatch size is lower than the maximum one, update it
	if (streamZonePtr->size + streamZonePtr->offset - hookPosition < dispatch_size)
		dispatch_size = streamZonePtr->size + streamZonePtr->offset - hookPosition;

	// Validate and adjust the fade dispatch size further;
	// this should correctly align the dispatch size to avoid starting a fade without
	// inverting the stereo image by mistake
	alignmentModDividend = dispatchPtr->channelCount * (dispatchPtr->wordSize == 8 ? 1 : 3);
	if (alignmentModDividend) {
		dispatch_size -= dispatch_size % alignmentModDividend;
	} else {
		printf("ERR:ValidateFadeSize() tried mod by 0...\n");
	}

	if (hookPosition < jumpDestination)
		dispatch_size = 0;

	zoneForJump = NULL;
	if (dispatch_size) {
		for (int i = 0; i < MAX_STREAMZONES; i++) {
			if (!streamZones[i].useFlag) {
				zoneForJump = &streamZones[i];
			}
		}

		if (!zoneForJump) {
			printf("ERR: out of streamZones...\n");
			printf("ERR: PrepareToJump() couldn't alloc zone...\n");
			return;
		}

		zoneForJump->prev = 0;
		zoneForJump->next = 0;
		zoneForJump->useFlag = 1;
		zoneForJump->offset = 0;
		zoneForJump->size = 0;
		zoneForJump->fadeFlag = 0;
	}

	zoneAfterJump = NULL;
	for (int i = 0; i < MAX_STREAMZONES; i++) {
		if (!streamZones[i].useFlag) {
			zoneAfterJump = &streamZones[i];
		}
	}

	if (!zoneAfterJump) {
		printf("ERR: out of streamZones...\n");
		printf("ERR: PrepareToJump() couldn't alloc zone...\n");
		return;
	}

	zoneAfterJump->prev = 0;
	zoneAfterJump->next = 0;
	zoneAfterJump->useFlag = 1;
	zoneAfterJump->offset = 0;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;

	streamZonePtr->size = hookPosition - streamZonePtr->offset;
	streamOffset = hookPosition - streamZonePtr->offset + dispatch_size;

	zoneCycle = dispatchPtr->streamZoneList;
	while (zoneCycle != streamZonePtr) {
		streamOffset += zoneCycle->size;
		zoneCycle = zoneCycle->next;
	}

	streamer_setIndex2(dispatchPtr->streamPtr, streamOffset);

	while (streamZonePtr->next) {
		streamZonePtr->next->useFlag = 0;
		iMUSE_removeItemFromList((iMUSETrack **)&streamZonePtr->next, (iMUSETrack *)streamZonePtr->next);
	}

	streamer_setSoundToStreamWithCurrentOffset(dispatchPtr->streamPtr,
		dispatchPtr->trackPtr->soundId, jumpDestination);

	if (dispatch_size) {
		streamZonePtr->next = zoneForJump;
		zoneForJump->prev = streamZonePtr;
		streamZonePtr = zoneForJump;
		zoneForJump->next = 0;
		zoneForJump->offset = hookPosition;
		zoneForJump->size = dispatch_size;
		zoneForJump->fadeFlag = 1;
	}

	streamZonePtr->next = zoneAfterJump;
	zoneAfterJump->prev = streamZonePtr;
	zoneAfterJump->next = 0;
	zoneAfterJump->offset = jumpDestination;
	zoneAfterJump->size = 0;
	zoneAfterJump->fadeFlag = 0;

	return;
}

// Validated
iMUSEStreamZone *dispatch_allocStreamZone() {
	for (int i = 0; i < MAX_STREAMZONES; i++) {
		if (streamZones[i].useFlag == 0) {
			streamZones[i].prev = 0;
			streamZones[i].next = 0;
			streamZones[i].useFlag = 1;
			streamZones[i].offset = 0;
			streamZones[i].size = 0;
			streamZones[i].fadeFlag = 0;

			return &streamZones[i];
		}
	}
	printf("ERR: out of streamZones...\n");
	return NULL;
}

// Validated
void dispatch_free() {
	int result;

	free(dispatch_buf);
	dispatch_buf = NULL;
}