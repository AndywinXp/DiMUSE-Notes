#ifndef DIGITAL_IMUSE_UTILS
#define DIGITAL_IMUSE_UTILS

#include "tracks.h"
#include <stdio.h>

int iMUSE_addItemToList(iMUSETracks **listPtr, iMUSETracks *listPtr_Item);
int iMUSE_removeItemFromList(iMUSETracks **listPtr, iMUSETracks *itemPtr);
int iMUSE_SWAP32(unsigned __int8 *value);
int iMUSE_clampNumber(int value, int minValue, int maxValue);
int iMUSE_clampTuning(int value, int minValue, int maxValue);
int iMUSE_checkHookId(int *trackHookId, int sampleHookId);
// TODO
// move trigger_copyTEXT, trigger_compareTEXT, trigger_getMarkerLength 
// tracks_setpriority, tracks_detune here, dispatch_checkHookId here
#endif