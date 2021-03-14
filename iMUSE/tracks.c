#include "tracks.h"
#include "imuse.h"
#include "fades.h"
#include "triggers.h"
#include "dispatch.h"
#include "waveapi.h"
#include <stdio.h> 

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
	}
	else {
		warning("TR: No WAVE driver loaded...");
		waveapi_waveOutParams.mixBuf = 0;
		waveapi_waveOutParams.offsetBeginMixBuf = 0;
		waveapi_waveOutParams.sizeSampleKB = 0;
		waveapi_waveOutParams.bytesPerSample = 8;
		waveapi_waveOutParams.numChannels = 1;
	}
	if (mixer_moduleInit(initDataPtr, &waveapi_waveOutParams))
		return -1;
	if (dispatch_moduleInit(initDataPtr))
		return -1;
	if (streamer_moduleInit())
		return -1;
	for (int l = 0; l < tracks_trackCount; l++) {
		tracks[l].prev = NULL;
		tracks[l].next = NULL;
		tracks[l].dispatchPtr = dispatch_getDispatchByID(l);
		tracks[l].dispatchPtr->trackPtr = tracks[l];
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
// TODO: addItemToList
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
			tracks[l].dispatchPtr->trackPtr = tracks[l];
			if (tracks[l].soundId) {
				IMUSE_AddItemToList(&tracks_trackList, tracks[l]);
			}
		}
	}
	dispatch_allocStreamZones();
	waveapi_decreaseSlice();
	return result + 480;
}

// Validated
// ...I think?
void tracks_setGroupVol() {
	iMUSETracks* curTrack = *(iMUSETracks**) tracks_trackList;
	int tmp;

	if (*(int*) tracks_trackList != NULL) {
		do {
			curTrack->effVol = (groups_getGroupVol(curTrack->group) * (curTrack->vol + 1)) / 128;
			curTrack = (iMUSETracks*) curTrack->next;
		} while (curTrack != NULL);
	}
	
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
		waveapi_write((int)&iMUSE_waveHeader, (int)&iMUSE_feedSize, iMUSE_sampleRate);
	} else {
		// 40 Hz frequency for filling the audio buffer, for some reason
		tracks_running40HzCount += timer_getUsecPerInt();
		if (tracks_running40HzCount >= 25000) {
			tracks_running40HzCount -= 25000;
			iMUSE_feedSize = tracks_prefSampleRate / 40;
			iMUSE_sampleRate = tracks_prefSampleRate;
		} else {
			iMUSE_feedSize = 0;
		}
	}

	if (iMUSE_feedSize != 0) {
		// Since this is always 0 this doesn't execute
		// ...I think
		if (tracks_initDataPtr->use11025HzSampleRate) {
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
			iMUSETracks * track = *(iMUSETracks **) tracks_trackList;
			if (tracks_trackList != NULL) {
				
				do {
					iMUSETracks * next = track->next;
					dispatch_processDispatches(track, iMUSE_feedSize, iMUSE_sampleRate);
					track = next;
				} while (track);
			}
		}
		if (tracks_waveCall) {
			mixer_loop(iMUSE_waveHeader, iMUSE_feedSize);
			if (tracks_waveCall) {
				waveapi_write((int)&iMUSE_waveHeader, (int)&iMUSE_feedSize, 0);
			}
		}
	}
	waveapi_decreaseSlice();
}

// Mess with pointers and stuff
int tracks_startSound(int soundId, int tryPriority, int unused) {
	int priority = tracks_setPriority(tryPriority, 0, 127);
	if (tracks_trackCount > 0) {
		int l = 0;
		while (tracks[l].soundId == 0) {
			l++;
			if (tracks_trackCount <= l)
				break;
		}

		iMUSETracks find_track = tracks[l];
		if (&find_track == NULL)
			return -6;

		find_track.soundId = soundId;
		find_track.marker = NULL;
		find_track.group = 0;
		find_track.priority = priority;
		find_track.vol = 127;
		find_track.effVol = groups_getGroupVol(0);
		find_track.pan = 64;
		find_track.detune = 0;
		find_track.transpose = 0;
		find_track.pitchShift = 0;
		find_track.mailbox = 0;
		find_track.jumpHook = 0;

		if (dispatch_alloc(&find_track, find_track.priority)) {
			printf("dispatch couldn't start sound...");
			find_track.soundId = 0;
			return -1;
		}
		waveapi_increaseSlice();
		iMUSE_addItemToList(tracks_trackList, find_track);
		waveapi_decreaseSlice();
		return 0;
	}

	printf("ERR: no spare tracks...");

	iMUSETracks * track = *(iMUSETracks**) tracks_trackList;
	int best_pri = 127;
	iMUSETracks * find_track = NULL;
	while (track) {
		int pri = track->priority;
		if (best_pri >= pri) {
			best_pri = pri;
			find_track = track;
		}
		track = track->next;
	}

	if (!find_track || priority < best_pri) {
		return -6;
	} else {
		iMUSE_removeItemFromList(tracks_trackList, find_track);
		dispatch_release(find_track);
		fades_clearFadeStatus(find_track->soundId, -1);
		triggers_clearTrigger(find_track->soundId, -1, -1);
		find_track->soundId = 0;
	}

	if (&find_track == NULL)
		return -6;

	find_track->soundId = soundId;
	find_track->marker = NULL;
	find_track->group = 0;
	find_track->priority = priority;
	find_track->vol = 127;
	find_track->effVol = groups_getGroupVol(0);
	find_track->pan = 64;
	find_track->detune = 0;
	find_track->transpose = 0;
	find_track->pitchShift = 0;
	find_track->mailbox = 0;
	find_track->jumpHook = 0;

	if (dispatch_alloc(find_track, find_track->priority)) {
		printf("dispatch couldn't start sound...");
		find_track->soundId = 0;
		return -1;
	}
	waveapi_increaseSlice();
	iMUSE_addItemToList(tracks_trackList, find_track);
	waveapi_decreaseSlice();
	return 0;
}

int tracks_stopSound(int soundId) {
	int result = -1;
	if (!tracks_trackList)
		return result;
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
	do {
		iMUSETracks *next_track = track->next;
		if (track->soundId != soundId)
			continue;
		iMUSE_removeItemFromList(tracks_trackList, track);
		dispatch_release(track);
		fades_clearFadeStatus(track->soundId, -1);
		triggers_clearTrigger(track->soundId, -1, -1);
		track->soundId = 0;
		result = 0;
		track = next_track;
	} while (track);
	return result;
}

int tracks_stopAllSounds() {
	if (!tracks_trackList)
		return 0;
	waveapi_increaseSlice();
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
	do {
		iMUSETracks *next_track = track->next;
		iMUSE_removeItemFromList(tracks_trackList, track);
		dispatch_release(track);
		fades_clearFadeStatus(track->soundId, -1);
		triggers_clearTrigger(track->soundId, -1, -1);
		track->soundId = 0;
		track = next_track;
	} while (track);
	waveapi_decreaseSlice();
	return 0;
}

int tracks_getNextSound(int soundId) {
	int found_soundId = 0;
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
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

int tracks_queryStream(int soundId, int sampleRate, int param2, int param3, int paused) {
	if (!tracks_trackList)
		return -1;

	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
	do {
		if (track->soundId) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_queryStream(track->dispatchPtr->streamPtr, sampleRate, param2, param3, paused);
				return 0;
			}
		}
		track = track->next;
	} while (track);
	return -1;
}

int tracks_feedStream(int soundId, int param1, int param2, int param3) {
	if (!tracks_trackList)
		return -1;
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
	do {
		if (track->soundId != 0) {
			if (track->soundId == soundId && track->dispatchPtr->streamPtr) {
				streamer_feedStream(track->dispatchPtr->streamPtr, param1, param2, param3);
				return 0;
			}
		}
		track = track->next;
	} while (track);
	return -1;
}

void tracks_clear(iMUSETracks *trackPtr) {
	iMUSE_removeItemFromList(tracks_trackList, trackPtr);
	dispatch_release(trackPtr);
	fades_clearFadeStatus(trackPtr->soundId, -1);
	triggers_clearTrigger(trackPtr->soundId, -1, -1);
	trackPtr->soundId = 0;
}

int tracks_setParam(int soundId, int opcode, int value) {
	if (!tracks_trackList)
		return -4;
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
	while (track) {
		if (track->soundId == soundId)
		{
			switch (opcode) {
			case 0x400:
				if (value >= 16)
					return -5;
				int group_vol = groups_getGroupVol(value);
				track->group = value;
				track->effVol = ((track->vol + 1) * group_vol) / 128;
				return 0;
			case 0x500:
				if (value > 127)
					return -5;
				track->priority = value;
				return 0;
			case 0x600:
				if (value > 127)
					return -5;
				int group_vol = groups_getGroupVol(track->group);
				track->group = value;
				track->effVol = ((track->vol + 1) * group_vol) / 128;
				return 0;
			case 0x700:
				if (value > 127)
					return -5;
				track->pan = value;
				return 0;
			case 0x800:
				if (value < -9216 || value > 9216)
					return -5;
				track->effVol = value;
				track->pitchShift = value + track->detune * 256;
				return 0;
			case 0x900:
				// The DIG 
				if (value < -12 || value > 12)
					return -5;
				if (value == 0) {
					track->detune = 0;
				} else {
					track->detune = tracks_detune(track->detune + value, -12, 12);
				}
				track->pitchShift = track->effVol + track->detune * 256;
				// end of DIG

				/*
				// COMI
				if (value < 0 || value > 4095)
					return -5;
				track->pitchShift = value;
				// end of COMI*/
				return 0;
			case 0xa00:
				track->mailbox = value;
				return 0;
			default:
				warning("setParam: unknown opcode %lu", opcode);
				return -5;
			}
			track = track->next;
		}
	}
	return -4;

}

int tracks_getParam(int soundId, int opcode) {
	if (!tracks_trackList) {
		if (opcode != 0x100)
			return -4;
		else
			return 0;
	}
	iMUSETracks *track = *(iMUSETracks**) tracks_trackList;
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
				return track->effVol;
			case 0x900:
				return track->detune;
			case 0xA00:
				return track->mailbox;
			case 0x1800:
				return (track->dispatchPtr->streamPtr >= 1);
			case 0x1900:
				return track->dispatchPtr->stramBufID;
			case 0x1A00:
				if (track->dispatchPtr->wordSize == 0)
					return 0;
				if (track->dispatchPtr->sampleRate == 0)
					return 0;
				if (track->dispatchPtr->channelCount == 0)
					return 0;
				return (track->dispatchPtr->currentOffset * 5) / (((track->dispatchPtr->wordSize / 8) * track->dispatchPtr->sampleRate * track->dispatchPtr->channelCount) / 200);
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
	iMUSETracks *track = *(iMUSETracks**)tracks_trackList;
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

int tracks_setHook(int soundId, int hookId) {
	if (hookId > 128)
		return -5;
	if (!tracks_trackList)
		return -4;

	iMUSETracks * track = *(iMUSETracks**)tracks_trackList;
	while (track->soundId != soundId) {
		track = track->next;
		if (!track)
			return -4;
	}
	track->jumpHook = hookId;
	return 0;
}

int tracks_getHook(int soundId) {
	if (!tracks_trackList)
		return -4;

	iMUSETracks * track = *(iMUSETracks**)tracks_trackList;
	while (track->soundId == soundId) {
		track = track->next;
		if (!track)
			return -4;
	}
	return track->jumpHook;
}

void tracks_free() {
	if (!tracks_trackList)
		return;
	waveapi_increaseSlice();
	iMUSETracks * track = *(iMUSETracks**)tracks_trackList;
	do {
		iMUSETracks *next_track = track->next;
		IMUSE_RemoveItemFromList(tracks_trackList, track);
		dispatch_release(track);
		fades_clearFadeStatus(track->soundId, -1);
		triggers_clearTrigger(track->soundId, -1, -1);
		track->soundId = 0;
		track = next_track;
	} while (track);
	waveapi_decreaseSlice();
	return 0;
}

// Validated
int tracks_setPriority(int priority, int minPriority, int maxPriority) {
	if (priority < minPriority)
		return minPriority;
	if (priority > maxPriority)
		return maxPriority;
	return priority;
}

// Validated
int tracks_detune(int detune, int minDetune, int maxDetune) {
	if (minDetune > detune) {
		detune += (12 * ((minDetune - detune) + 11) / 12);
	}
	if (maxDetune < detune) {
		detune -= (12 * ((detune - maxDetune) + 11) / 12);
	}
	return detune;
}

// Validated
int tracks_debug() {
	printf("trackCount: %d", tracks_trackCount);
	printf("waveCall: %p", tracks_waveCall);
	printf("pauseTimer: %d", tracks_pauseTimer);
	printf("trackList: %p", *(int *)tracks_trackList);
	printf("prefSampleRate: %d", tracks_prefSampleRate);
	printf("initDataPtr: %p", tracks_initDataPtr);

	for (int i = 0; i < MAX_TRACKS; i++) {
		printf("trackId: %d", i);
		printf("\tprev: %p", tracks[i].prev);
		printf("\tnext: %p", tracks[i].next);
		printf("\tdispatchPtr: %p", tracks[i].dispatchPtr);
		printf("\tsound: %d", tracks[i].soundId);
		printf("\tmarker: %d", tracks[i].marker);
		printf("\tgroup: %d", tracks[i].group);
		printf("\tpriority: %d", tracks[i].priority);
		printf("\tvol: %d", tracks[i].vol);
		printf("\teffVol: %d", tracks[i].effVol);
		printf("\tpan: %d", tracks[i].pan);
		printf("\tdetune: %d", tracks[i].detune);
		printf("\ttranspose: %d", tracks[i].transpose);
		printf("\tpitchShift: %d", tracks[i].pitchShift);
		printf("\tmailbox: %d", tracks[i].mailbox);
		printf("\tjumpHook: %d", tracks[i].jumpHook);
	}

	return 0;
}
