#include <stdio.h>
#include <stdlib.h>

typedef struct iMUSETracks_node {
	struct iMUSETracks_node *prev;
	struct iMUSETracks_node *next;
	int *dispatchPtr;
	int soundId;
	int *marker;
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

iMUSETracks *tracks_trackList = NULL;
iMUSETracks tracks[8];

int iMUSE_addItemToList(iMUSETracks **listPtr, iMUSETracks *listPtr_Item)
{
	if (!listPtr_Item || listPtr_Item->prev || listPtr_Item->next) {
		printf("ERR: list arg err when adding...");
		return -5;
	} else {
		// Set item's next element to the list
		listPtr_Item->next = *listPtr;

		if (*listPtr == NULL) {
			// If the list is empty, use this item as the list
			*listPtr = listPtr_Item;
		} else {
			// Update the previous element on the tail of the list (not done on the original)
			((*listPtr)->prev) = listPtr_Item;
		}
		// Set the previous element of the item as NULL, 
		// effectively making the item the first element of the list
		listPtr_Item->prev = NULL;

		// Update the list with the new data
		*listPtr = listPtr_Item;

	}
	return 0;
}

int iMUSE_removeItemFromList(iMUSETracks **listPtr, iMUSETracks *itemPtr)
{
	iMUSETracks *track = *listPtr;
	if (itemPtr && track) {
		do {
			if (track == itemPtr)
				break;
			track = track->next;
		} while (track);

		if (track) {
			iMUSETracks *next_track = itemPtr->next;
			iMUSETracks *prev_track = itemPtr->prev;
			if (next_track)
				next_track->prev = prev_track;

			if (prev_track) {
				prev_track->next = next_track;
			} else {
				listPtr = next_track;
			}

			itemPtr->next = NULL;
			itemPtr->prev = NULL;
			return 0;
		} else {
			printf("ERR: item not on list...");
			return -3;
		}
	} else {
		printf("ERR: list arg err when removing...");
		return -5;
	}
}

int main() {
	iMUSETracks find_track = tracks[0];

	find_track.soundId = 1000;
	find_track.marker = NULL;
	find_track.group = 0;
	find_track.priority = 127;
	find_track.vol = 127;
	find_track.effVol = 127;
	find_track.pan = 64;
	find_track.detune = 0;
	find_track.transpose = 0;
	find_track.pitchShift = 0;
	find_track.mailbox = 0;
	find_track.jumpHook = 0;

	iMUSETracks find_track2 = tracks[1];

	find_track2.soundId = 2000;
	find_track2.marker = NULL;
	find_track2.group = 2;
	find_track2.priority = 127;
	find_track2.vol = 127;
	find_track2.effVol = 127;
	find_track2.pan = 64;
	find_track2.detune = 0;
	find_track2.transpose = 0;
	find_track2.pitchShift = 0;
	find_track2.mailbox = 1;
	find_track2.jumpHook = 0;

	iMUSETracks find_track3 = tracks[2];
	find_track3.soundId = 3000;
	find_track3.marker = NULL;
	find_track3.group = 4;
	find_track3.priority = 127;
	find_track3.vol = 127;
	find_track3.effVol = 127;
	find_track3.pan = 64;
	find_track3.detune = 0;
	find_track3.transpose = 0;
	find_track3.pitchShift = 0;
	find_track3.mailbox = 1;
	find_track3.jumpHook = 0;

	iMUSETracks find_track4 = tracks[3];
	find_track4.soundId = 4000;
	find_track4.marker = NULL;
	find_track4.group = 4;
	find_track4.priority = 127;
	find_track4.vol = 127;
	find_track4.effVol = 127;
	find_track4.pan = 64;
	find_track4.detune = 0;
	find_track4.transpose = 0;
	find_track4.pitchShift = 0;
	find_track4.mailbox = 1;
	find_track4.jumpHook = 0;

	iMUSE_addItemToList(&tracks_trackList, &find_track);
	iMUSE_addItemToList(&tracks_trackList, &find_track2);
	iMUSE_addItemToList(&tracks_trackList, &find_track4);
	iMUSE_addItemToList(&tracks_trackList, &find_track3);

	iMUSE_removeItemFromList(&tracks_trackList, &find_track4);

	iMUSETracks *track = tracks_trackList;
	while (track) {
		printf("soundID: %d\n", track->soundId);
		track = track->next;
	}
	

}