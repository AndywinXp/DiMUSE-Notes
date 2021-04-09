#include "imuseutils.h"

// Validated
int iMUSE_addItemToList(iMUSETrack **listPtr, iMUSETrack *listPtr_Item) {
	if (!listPtr_Item || listPtr_Item->prev || listPtr_Item->next) {
		printf("ERR: list arg err when adding...\n");
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

// Validated
int iMUSE_removeItemFromList(iMUSETrack **listPtr, iMUSETrack *itemPtr) {
	iMUSETrack *track = *listPtr;
	if (itemPtr && track) {
		do {
			if (track == itemPtr)
				break;
			track = track->next;
		} while (track);

		if (track) {
			iMUSETrack *next_track = itemPtr->next;
			iMUSETrack *prev_track = itemPtr->prev;

			if (next_track)
				next_track->prev = prev_track;

			if (prev_track) {
				prev_track->next = next_track;
			} else {
				*listPtr = next_track;
			}

			itemPtr->next = NULL;
			itemPtr->prev = NULL;
			return 0;
		} else {
			printf("ERR: item not on list...\n");
			return -3;
		}
	} else {
		printf("ERR: list arg err when removing...\n");
		return -5;
	}
}

// Validated
int iMUSE_clampNumber(int value, int minValue, int maxValue) {
	if (value < minValue)
		return minValue;

	if (value > maxValue)
		return maxValue;

	return value;
}

// Validated
int iMUSE_clampTuning(int value, int minValue, int maxValue) {
	if (minValue > value) {
		value += (12 * ((minValue - value) + 11) / 12);
	}

	if (maxValue < value) {
		value -= (12 * ((value - maxValue) + 11) / 12);
	}

	return value;
}

// Validated
int iMUSE_SWAP32(uint8 *value) {
	return value[3] | ((value[2] | ((value[1] | (*value << 8)) << 8)) << 8);
}

// Validated
int iMUSE_checkHookId(int *trackHookId, int sampleHookId) {
	if (sampleHookId) {
		if (*trackHookId == sampleHookId) {
			*trackHookId = 0;
			return 0;
		} else {
			return -1;
		}
	} else if (*trackHookId == 128) {
		*trackHookId = 0;
		return -1;
	} else {
		return 0;
	}
}