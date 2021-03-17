#include "imuseutils.h"

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
	}
	else {
		printf("ERR: list arg err when removing...");
		return -5;
	}
}

int iMUSE_SWAP32(unsigned __int8 *value)
{
	return value[3] | ((value[2] | ((value[1] | (*value << 8)) << 8)) << 8);
}