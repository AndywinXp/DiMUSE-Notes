#include "tracks.h"

// Validated
// TODO: another check and proper types
int tracks_moduleInit(iMUSEInitData *initDataPtr) {
	tracks_initDataPtr = initDataPtr;

	if (tracks_initDataPtr->waveMixCount == 0 || tracks_initDataPtr->waveMixCount > 8) {
		warning("TR: waveMixCount NULL or too big, defaulting to 4...");
		tracks_initDataPtr->waveMixCount = 4;
	}

	tracks_trackCount = tracks_initDataPtr->waveMixCount;
	tracks_waveCall = tracks_initDataPtr->waveCall; // This is just a function which returns -1
	tracks_pauseTimer = 0;
	tracks_trackList = NULL;
	tracks_prefSampleRate = 22050;

	if (tracks_waveCall) {
		if (waveapi_moduleInit(initDataPtr->unused, 22050, &waveapi_waveOutParams))
			return -1;
	} else {
		warning("TR: No WAVE driver loaded...");
		waveapi_waveOutParams.mixBuf = 0;
		waveapi_waveOutParams.offsetBeginMixBuf = 0;
		waveapi_waveOutParams.sizeSampleKB = 0;
		waveapi_waveOutParams.bytesPerSample = 8;
		waveapi_waveOutParams.numChannels = 1;
	}

	if (mixer_moduleInit(initDataPtr, &waveapi_waveOutParams) || dispatch_moduleInit(initDataPtr) || streamer_moduleInit())
		return -1;

	for (int l = 0; l < tracks_trackCount; l++) {
		tracks[l].prev = NULL;
		tracks[l].next = NULL;
		tracks[l].dispatchPtr = dispatch_getDispatchByID(l);
		tracks[l].dispatchPtr->trackPtr = &tracks[l];
		tracks[l].soundId = 0;
	}

	return 0;
}

// Validated
void tracks_pause() {
	tracks_pauseTimer = 1;
}

// Validated
void tracks_resume() {
	tracks_pauseTimer = 0;
}

// Validated
int tracks_save(int *buffer, int bufferSize) {
	waveapi_increaseSlice();
	int result = dispatch_save(buffer, bufferSize);
	if (result < 0) {
		waveapi_decreaseSlice();
		return result;
	}
	bufferSize -= result;
	if (bufferSize < 480) {
		waveapi_decreaseSlice();
		return -5;
	}
	memcpy(buffer + result, &tracks, 480);
	waveapi_decreaseSlice();
	return result + 480;
}

// Validated
int tracks_restore(int *buffer) {
	waveapi_increaseSlice();
	tracks_trackList = NULL;
	int result = dispatch_restore(buffer);
	memcpy(&tracks, buffer + result, 480);
	if (tracks_trackCount > 0) {
		for (int l = 0; l < tracks_trackCount; l++) {
			tracks[l].prev = NULL;
			tracks[l].next = NULL;
			tracks[l].dispatchPtr = dispatch_getDispatchByTrackID(l);
			tracks[l].dispatchPtr->trackPtr = &tracks[l];
			if (tracks[l].soundId) {
				iMUSE_addItemToList(&tracks_trackList, &tracks[l]);
			}
		}
	}

	dispatch_allocStreamZones();
	waveapi_decreaseSlice();

	return result + 480;
}

// Validated
void tracks_setGroupVol() {
	iMUSETrack* curTrack = tracks_trackList;
	int tmp;

	while (curTrack) {
		curTrack->effVol = ((curTrack->vol + 1) * groups_getGroupVol(curTrack->group)) / 128;
		curTrack = curTrack->next;
	};
	
	return 0;
}

// Validated
// ...I think?
void tracks_callback() {
	if (tracks_pauseTimer) {
		if (++tracks_pauseTimer < 3)
			return;
		tracks_pauseTimer = 3;
	}

	waveapi_increaseSlice();
	dispatch_predictFirstStream();
	if (tracks_waveCall) {
		waveapi_write((int)&iMUSE_audioBuffer, (int)&iMUSE_feedSize, iMUSE_sampleRate);
	} else {
		// 40 Hz frequency for filling the audio buffer, for some reason
		// Anyway it appears we never reach this block since tracks_waveCall is assigned to a (dummy) function
		tracks_running40HzCount += timer_getUsecPerInt(); // Rename to timer_running40HzCount or similar
		if (tracks_running40HzCount >= 25000) {
			tracks_running40HzCount -= 25000;
			iMUSE_feedSize = tracks_prefSampleRate / 40;
			iMUSE_sampleRate = tracks_prefSampleRate;
		} else {
			iMUSE_feedSize = 0;
		}
	}

	if (iMUSE_feedSize != 0) {

		// Since this flag is always 0 this doesn't execute
		if (tracks_initDataPtr->halfSampleRateFlag) {
			iMUSE_sampleRate /= 2;
		}
		mixer_clearMixBuff();
		if (!tracks_pauseTimer) {
			// If SMUSH is active (signaled by the fact that this function is not NULL)
			if (tracks_initDataPtr->SMUSH_related_function) {
				tracks_uSecsToFeed += (iMUSE_feedSize * 1000000) / iMUSE_sampleRate;
				// SMUSH runs at 12 frames per second (12 Hz)
				while (tracks_initDataPtr->running12HzCount <= tracks_uSecsToFeed) {
					tracks_uSecsToFeed -= tracks_initDataPtr->running12HzCount;
					tracks_initDataPtr->SMUSH_related_function(); 
				}
			}
			iMUSETrack *track = tracks_trackList;

			while (track) {
				iMUSETrack *next = track->next;
				dispatch_processDispatches(track, iMUSE_feedSize, iMUSE_sampleRate);
				track = next;
			};
		}

		if (tracks_waveCall) {
			mixer_loop(iMUSE_audioBuffer, iMUSE_feedSize);
			if (tracks_waveCall) {
				waveapi_write((int)&iMUSE_audioBuffer, (int)&iMUSE_feedSize, 0);
			}
		}
	}

	waveapi_decreaseSlice();
}

// Validated
int tracks_startSound(int soundId, int tryPriority, int unused) {
	int priority = iMUSE_clampNumber(tryPriority, 0, 127);
	if (tracks_trackCount > 0) {
		int l = 0;
		while (tracks[l].soundId == 0) {
			l++;
			if (tracks_trackCount <= l)
				break;
		}

		iMUSETrack *foundTrack = &tracks[l];
		if (!foundTrack)
			return -6;

		foundTrack->soundId = soundId;
		foundTrack->marker = NULL;
		foundTrack->group = 0;
		foundTrack->priority = priority;
		foundTrack->vol = 127;
		foundTrack->effVol = groups_getGroupVol(0);
		foundTrack->pan = 64;
		foundTrack->detune = 0;
		foundTrack->transpose = 0;
		foundTrack->pitchShift = 0;
		foundTrack->mailbox = 0;
		foundTrack->jumpHook = 0;

		if (dispatch_alloc(foundTrack, foundTrack->priority)) {
			printf("ERR: dispatch couldn't start sound...\n");
			foundTrack->soundId = 0;
			return -1;
		}
		waveapi_increaseSlice();
		iMUSE_addItemToList(&tracks_trackList, foundTrack);
		waveapi_decreaseSlice();
		return 0;
	}

	printf("ERR: no spare tracks...\n");

	// Let's steal the track with the lowest priority
	iMUSETrack *track = tracks_trackList;
	int bestPriority = 127;
	iMUSETrack *stolenTrack = NULL;

	while (track) {
		int curTrackPriority = track->priority;
		if (curTrackPriority <= bestPriority) {
			bestPriority = curTrackPriority;
			stolenTrack = track;
		}
		track = track->next;
	}

	if (!stolenTrack || priority < bestPriority) {
		return -6;
	} else {
		iMUSE_removeItemFromList(&tracks_trackList, stolenTrack);
		dispatch_release(stolenTrack);
		fades_clearFadeStatus(stolenTrack->soundId, -1);
		triggers_clearTrigger(stolenTrack->soundId, -1, -1);
		stolenTrack->soundId = 0;
	}

	stolenTrack->soundId = soundId;
	stolenTrack->marker = NULL;
	stolenTrack->group = 0;
	stolenTrack->priority = priority;
	stolenTrack->vol = 127;
	stolenTrack->effVol = groups_getGroupVol(0);
	stolenTrack->pan = 64;
	stolenTrack->detune = 0;
	stolenTrack->transpose = 0;
	stolenTrack->pitchShift = 0;
	stolenTrack->mailbox = 0;
	stolenTrack->jumpHook = 0;

	if (dispatch_alloc(&stolenTrack, stolenTrack->priority)) {
		printf("ERR: dispatch couldn't start sound...\n");
		stolenTrack->soundId = 0;
		return -1;
	}

	waveapi_increaseSlice();
	iMUSE_addItemToList(&tracks_trackList, stolenTrack);
	waveapi_decreaseSlice();

	return 0;
}

// Validated
int tracks_stopSound(int soundId) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId == soundId) {
			iMUSE_removeItemFromList(&tracks_trackList, track);
			dispatch_release(track);
			fades_clearFadeStatus(track->soundId, -1);
			triggers_clearTrigger(track->soundId, -1, -1);
			track->soundId = 0;
		}
		track = track->next;
	} while (track);

	return 0;
}

// Validated
int tracks_stopAllSounds() {
	waveapi_increaseSlice();

	if (tracks_trackList) {
		iMUSETrack *track = tracks_trackList;
		do {
			iMUSE_removeItemFromList(&tracks_trackList, track);
			dispatch_release(track);
			fades_clearFadeStatus(track->soundId, -1);
			triggers_clearTrigger(track->soundId, -1, -1);
			track->soundId = 0;
			track = track->next;
		} while (track);
	}

	waveapi_decreaseSlice();

	return 0;
}

// Validated
int tracks_getNextSound(int soundId) {
	int found_soundId = 0;
	iMUSETrack *track = tracks_trackList;
	while (track) {
		if (track->soundId > soundId) {
			if (!found_soundId || track->soundId < found_soundId) {
				found_soundId = track->soundId;
			}
		}
		track = track->next;
	};

	return found_soundId;
}

// Validated
int tracks_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_queryStream(track->dispatchPtr->streamPtr, bufSize, criticalSize, freeSpace, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);

	return -1;
}

// Validated, but re-check type for srcBuf
int tracks_feedStream(int soundId, int srcBuf, int sizeToFeed, int paused) {
	if (!tracks_trackList)
		return -1;

	iMUSETrack *track = tracks_trackList;
	do {
		if (track->soundId != 0) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_feedStream(track->dispatchPtr->streamPtr, srcBuf, sizeToFeed, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);

	return -1;
}

// Validated
void tracks_clear(iMUSETrack *trackPtr) {
	iMUSE_removeItemFromList(&tracks_trackList, trackPtr);
	dispatch_release(trackPtr);
	fades_clearFadeStatus(trackPtr->soundId, -1);
	triggers_clearTrigger(trackPtr->soundId, -1, -1);
	trackPtr->soundId = 0;
}

// Validated
int tracks_setParam(int soundId, int opcode, int value) {
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track) {
		if (track->soundId == soundId) {
			switch (opcode) {
			case 0x400:
				if (value >= 16)
					return -5;
				track->group = value;
				track->effVol = ((track->vol + 1) * groups_getGroupVol(value)) / 128;
				return 0;
			case 0x500:
				if (value > 127)
					return -5;
				track->priority = value;
				return 0;
			case 0x600:
				if (value > 127)
					return -5;
				track->vol = value;
				track->effVol = ((value + 1) * groups_getGroupVol(track->group)) / 128;
				return 0;
			case 0x700:
				if (value > 127)
					return -5;
				track->pan = value;
				return 0;
			case 0x800:
				if (value < -9216 || value > 9216)
					return -5;
				track->detune = value;
				track->pitchShift = value + track->transpose * 256;
				return 0;
			case 0x900:
				// The DIG 
				if (value < -12 || value > 12)
					return -5;
				if (value == 0) {
					track->transpose = 0;
				} else {
					track->transpose = iMUSE_clampTuning(track->detune + value, -12, 12);
				}
				track->pitchShift = track->detune + (track->transpose * 256);
				// end of DIG

				/*
				// COMI
				if (value < 0 || value > 4095)
					return -5;
				track->pitchShift = value;
				// end of COMI*/
				return 0;
			case 0xA00:
				track->mailbox = value;
				return 0;
			default:
				printf("ERR: setParam: unknown opcode %lu\n", opcode);
				return -5;
			}

			track = track->next;
		}
	}

	return -4;
}

// Validated
int tracks_getParam(int soundId, int opcode) {
	if (!tracks_trackList) {
		if (opcode != 0x100)
			return -4;
		else
			return 0;
	}
	iMUSETrack *track = tracks_trackList;
	int l = 0;
	do {
		if (track->soundId == soundId) {
			switch (opcode) {
			case 0:
				return -1;
			case 0x100:
				return l;
			case 0x200:
				return -1;
			case 0x300:
				return track->marker;
			case 0x400:
				return track->group;
			case 0x500:
				return track->priority;
			case 0x600:
				return track->vol;
			case 0x700:
				return track->pan;
			case 0x800:
				return track->detune;
			case 0x900:
				return track->transpose;
			case 0xA00:
				return track->mailbox;
			case 0x1800:
				return (track->dispatchPtr->streamPtr != 0);
			case 0x1900:
				return track->dispatchPtr->streamBufID;
			// COMI maybe?
			/*
			case 0x1A00:
				if (track->dispatchPtr->wordSize == 0)
					return 0;
				if (track->dispatchPtr->sampleRate == 0)
					return 0;
				if (track->dispatchPtr->channelCount == 0)
					return 0;
				return (track->dispatchPtr->currentOffset * 5) / (((track->dispatchPtr->wordSize / 8) * track->dispatchPtr->sampleRate * track->dispatchPtr->channelCount) / 200);
			*/
			default:
				return -5;
			}
		}

		track = track->next;
		l++;
	} while (track);

	if (opcode != 0x100)
		return -4;
	else
		return 0;
}
/*
int tracks_lipSync(int soundId, int syncId, int msPos, int *width, char *height) {
	int w = 0;
	int h = 0;
	iMUSETrack *track = *(iMUSETrack**)tracks_trackList;
	int result = 0;

	if (msPos < 0) {
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		return result;
	}

	msPos /= 16;
	if (msPos >= 65536) {
		result = -5;
	}
	else {
		if (tracks_trackList) {
			do {
				if (track->soundId == soundId)
					break;
				track = track->next;
			} while (track);
		}
		if (track) {
			switch (syncId) {
			case 0:
				sync_ptr = track->sync_ptr_0;
				sync_size = track->sync_size_0;
				goto break;
			case 1:
				sync_ptr = track->sync_ptr_1
					sync_size = track->sync_size_1;
				goto break;
			case 2:
				sync_ptr = track->sync_ptr_2;
				sync_size = track->sync_size_2;
				break;
			case 3:
				sync_ptr = track->sync_ptr_3;
				sync_size = track->sync_size_3;
				break;
			default:
				break;
			}
			if (sync_size && sync_ptr) {
				sync_size /= 4;
				v12 = sync_ptr + 2;
				v15 = sync_size;
				v13 = sync_size - 1;
				while (sync_size--) {
					if (sync_ptr[2] >= msPos)
						break;
					sync_ptr += 4;
				}
				if (sync_size < 0 || sync_ptr[2] > msPos)
					sync_size -= 4;
				val = *(sync_ptr - 2);
				w = (val >> 8) & 0x7F;
				h = val & 0x7F;
			}
		}
		else {
			result = -4;
		}
	}
	if (width)
		*width = w;
	if (height)
		*height = h;
	return result;
}*/

// Validated
int tracks_setHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	track->jumpHook = hookId;

	return 0;
}

// Validated
int tracks_getHook(int soundId) {
	if (!tracks_trackList)
		return -4;

	iMUSETrack *track = tracks_trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}

	return track->jumpHook;
}

// Validated
void tracks_free() {
	if (!tracks_trackList)
		return;

	waveapi_increaseSlice();

	iMUSETrack *track = tracks_trackList;
	do {
		iMUSE_removeItemFromList(&tracks_trackList, &track);

		dispatch_release(track);
		fades_clearFadeStatus(track->soundId, -1);
		triggers_clearTrigger(track->soundId, -1, -1);

		track->soundId = 0;
		track = track->next;
	} while (track);

	waveapi_decreaseSlice();

	return 0;
}

// Validated
int tracks_debug() {
	printf("trackCount: %d\n", tracks_trackCount);
	printf("waveCall: %p\n", tracks_waveCall);
	printf("pauseTimer: %d\n", tracks_pauseTimer);
	printf("trackList: %p\n", *(int *)tracks_trackList);
	printf("prefSampleRate: %d\n", tracks_prefSampleRate);
	printf("initDataPtr: %p\n", tracks_initDataPtr);

	for (int i = 0; i < MAX_TRACKS; i++) {
		printf("trackId: %d\n", i);
		printf("\tprev: %p\n", tracks[i].prev);
		printf("\tnext: %p\n", tracks[i].next);
		printf("\tdispatchPtr: %p\n", tracks[i].dispatchPtr);
		printf("\tsound: %d\n", tracks[i].soundId);
		printf("\tmarker: %d\n", tracks[i].marker);
		printf("\tgroup: %d\n", tracks[i].group);
		printf("\tpriority: %d\n", tracks[i].priority);
		printf("\tvol: %d\n", tracks[i].vol);
		printf("\teffVol: %d\n", tracks[i].effVol);
		printf("\tpan: %d\n", tracks[i].pan);
		printf("\tdetune: %d\n", tracks[i].detune);
		printf("\ttranspose: %d\n", tracks[i].transpose);
		printf("\tpitchShift: %d\n", tracks[i].pitchShift);
		printf("\tmailbox: %d\n", tracks[i].mailbox);
		printf("\tjumpHook: %d\n", tracks[i].jumpHook);
	}

	return 0;
}
