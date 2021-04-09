#ifndef DIGITAL_IMUSE_UTILS
#define DIGITAL_IMUSE_UTILS

#include "imuse.h"

int iMUSE_addItemToList(iMUSETrack **listPtr, iMUSETrack *listPtr_Item);
int iMUSE_removeItemFromList(iMUSETrack **listPtr, iMUSETrack *itemPtr);
int iMUSE_SWAP32(uint8 *value);
int iMUSE_clampNumber(int value, int minValue, int maxValue);
int iMUSE_clampTuning(int value, int minValue, int maxValue);
int iMUSE_checkHookId(int *trackHookId, int sampleHookId);

#endif