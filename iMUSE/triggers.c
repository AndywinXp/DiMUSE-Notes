#include "commands.h"
#include "fades.h"
#include "triggers.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

int triggers_moduleInit(iMUSEInitData * initDataPtr)
{
	triggers_initDataPtr = initDataPtr;
	for (int l = 0; l < 8; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

int triggers_clear()
{
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

int triggers_save(unsigned char * buffer, int sizeLeft)
{
	if (sizeLeft < 2848) {
		return -5;
	}
	memcpy(buffer, trigs, 2464);
	memcpy(buffer + 2464, defers, 384);
	return 2848;
}

int triggers_restore(unsigned char * buffer)
{
	memcpy(trigs, buffer, 2464);
	memcpy(defers, buffer + 2464, 384);
	triggers_defersOn = 1;
	return 2848;
}

int triggers_setTrigger(int soundId, char * marker, int opcode)
{
	if (!soundId)
		return -5;
	if (!marker) {
		marker = triggers_empty_marker;
	}

	if (triggers_getMarkerLength(marker) >= 256) {
		printf("ERR: attempting to set trig with oversize marker string...");
		return -5;
	}
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound) {
			trigs[l].sound = soundId;
			trigs[l].clearLater = 0;
			trigs[l].opcode = opcode;
			triggers_copyTEXT(trigs[l].text, marker);
			__int32* ptr = &opcode; // args after opcode
			trigs[l].args_0_ = *(__int32 *)(ptr + 4);
			trigs[l].args_1_ = *(__int32 *)(ptr + 8);
			trigs[l].args_2_ = *(__int32 *)(ptr + 12);
			trigs[l].args_3_ = *(__int32 *)(ptr + 16);
			trigs[l].args_4_ = *(__int32 *)(ptr + 20);
			trigs[l].args_5_ = *(__int32 *)(ptr + 24);
			trigs[l].args_6_ = *(__int32 *)(ptr + 28);
			trigs[l].args_7_ = *(__int32 *)(ptr + 32);
			trigs[l].args_8_ = *(__int32 *)(ptr + 36);
			trigs[l].args_9_ = *(__int32 *)(ptr + 40);
			return 0;
		}
	}
	printf("ERR: tr unable to alloc trigger...");
	return -6;
}

int triggers_checkTrigger(int soundId, char * marker, int opcode)
{
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;
		if (soundId == -1 || trigs[l].sound == soundId) {
			if (marker|| !triggers_compareTEXT(marker, trigs[l].text)) {
				if (opcode == -1 || trigs[l].opcode == opcode)
					r++;
			}
		}
	}

	return r;
}

int triggers_clearTrigger(int soundId, char * marker, int opcode)
{
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;
		if (soundId != -1 && trigs[l].sound != soundId)
			continue;
		if (marker && triggers_compareTEXT(marker, trigs[l].text))
			continue;
		if (opcode != -1 && trigs[l].opcode != opcode)
			continue;
		if (triggers_midProcessing) {
			trigs[l].clearLater = 1;
		}
		else {
			trigs[l].sound = 1;
		}
		return 0;
	}
}

void triggers_processTriggers(int soundId, char * marker)
{
	char textBuffer[256];
	int r;
	if (triggers_getMarkerLength(marker) >= 256) {
		printf("passed oversize marker string...");
		return;
	}
	triggers_copyTEXT(triggers_textBuffer, marker);
	triggers_midProcessing++;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;
		if (trigs[l].sound != soundId)
			continue;
		if (!trigs[l].text[0] && triggers_compareTEXT(triggers_textBuffer, trigs[l].text))
			continue;
		if (triggers_textBuffer[0]) {
			r = 0;
			do {
				textBuffer[r] = triggers_textBuffer[r];
			} while (triggers_textBuffer[r++]);
		}
		textBuffer[r] = 0;
		trigs[l].sound = 0;
		if (trigs[l].opcode) {
			if (trigs[l].opcode < 30) {
				handleCmds(trigs[l].opcode, trigs[l].args_0_,
					trigs[l].args_1_, trigs[l].args_2_,
					trigs[l].args_3_, trigs[l].args_4_,
					trigs[l].args_5_, trigs[l].args_6_,
					trigs[l].args_7_, trigs[l].args_8_,
					trigs[l].args_9_);
			}
			else { // WTF
				trigs[l].opcode(trigs[l].text, trigs[l].args_0_,
					trigs[l].args_1_, trigs[l].args_2_,
					trigs[l].args_3_, trigs[l].args_4_,
					trigs[l].args_5_, trigs[l].args_6_,
					trigs[l].args_7_, trigs[l].args_8_,
					trigs[l].args_9_);
			}
		}
		else if (!defers[l].opcode) {
			if (!triggers_initDataPtr || !triggers_initDataPtr->scriptCallback) {
				printf("null callback in InitData struct...");
			}
			else {
				triggers_initDataPtr->scriptCallback(trigs[l].text, trigs[l].args_0_,
					trigs[l].args_1_, trigs[l].args_2_,
					trigs[l].args_3_, trigs[l].args_4_,
					trigs[l].args_5_, trigs[l].args_6_,
					trigs[l].args_7_, trigs[l].args_8_,
					trigs[l].args_9_);
			}
		}
		if (textBuffer[0]) {
			r = 0;
			do {
				triggers_textBuffer[r] = textBuffer[r];
			} while (textBuffer[r++]);
		}
		trigs[l].text[0] = 0;
	}
	if (--triggers_midProcessing == 0) {
		for (int l = 0; l < MAX_TRIGGERS; l++) {
			if (trigs[l].clearLater) {
				trigs[l].sound = 0;
			}
		}
	}
}

int triggers_deferCommand(int count, void * opcode_and_params)
{
	if (!count) {
		return -5;
	}
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter) {
			defers[l].opcode = *(__int32 *)(opcode_and_params);
			__int32 *ptr = (__int32 *) opcode_and_params;
			defers[l].args_0_ = *(__int32 *)(ptr + 4);
			defers[l].args_1_ = *(__int32 *)(ptr + 8);
			defers[l].args_2_ = *(__int32 *)(ptr + 12);
			defers[l].args_3_ = *(__int32 *)(ptr + 16);
			defers[l].args_4_ = *(__int32 *)(ptr + 20);
			defers[l].args_5_ = *(__int32 *)(ptr + 24);
			defers[l].args_6_ = *(__int32 *)(ptr + 28);
			defers[l].args_7_ = *(__int32 *)(ptr + 32);
			defers[l].args_8_ = *(__int32 *)(ptr + 36);
			defers[l].args_9_ = *(__int32 *)(ptr + 40);
			defers[l].counter = count;
			triggers_defersOn = 1;
			return 0;
		}
	}
	printf("ERR: tr unable to alloc deferred cmd...");
	return -6;
}

void triggers_loop()
{
	if (!triggers_defersOn)
		return;

	triggers_defersOn = 0;
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter)
			continue;
		triggers_defersOn = 1;
		if (--defers[l].counter == 0)
			continue;
		if (defers[l].opcode) {
			if (defers[l].opcode < 30) {
				handleCmds(trigs[l].opcode,
					trigs[l].args_0_, trigs[l].args_1_,
					trigs[l].args_2_, trigs[l].args_3_,
					trigs[l].args_4_, trigs[l].args_5_,
					trigs[l].args_6_, trigs[l].args_7_,
					trigs[l].args_8_, trigs[l].args_9_);
			}
			else {
				trigs[l].opcode(trigs[l].text,
					trigs[l].args_0_, trigs[l].args_1_,
					trigs[l].args_2_, trigs[l].args_3_,
					trigs[l].args_4_, trigs[l].args_5_,
					trigs[l].args_6_, trigs[l].args_7_,
					trigs[l].args_8_, trigs[l].args_9_);
			}
		}
		else {
			if (!triggers_initDataPtr || !triggers_initDataPtr->scriptCallback) {
				printf("null callback in InitData struct...");
			}
			else {
				triggers_initDataPtr->scriptCallback(trigs[l].text,
					trigs[l].args_0_, trigs[l].args_1_,
					trigs[l].args_2_, trigs[l].args_3_,
					trigs[l].args_4_, trigs[l].args_5_,
					trigs[l].args_6_, trigs[l].args_7_,
					trigs[l].args_8_, trigs[l].args_9_);
			}
		}
	}
}

int triggers_countPendingSounds(int soundId)
{
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;

		int opcode = trigs[l].opcode;
		if (opcode == 8 && trigs[l].args_0_ == soundId || opcode == 26 && trigs[l].args_1_ == soundId)
			r++;
	}

	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter)
			continue;

		int opcode = defers[l].opcode;
		if (opcode == 8 && defers[l].args_0_ == soundId || opcode == 26 && defers[l].args_1_ == soundId)
			r++;
	}

	return r;
}

int triggers_moduleFree()
{
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

void triggers_copyTEXT(char * dst, char * marker)
{
	if (!dst || !marker)
		return;
	strcpy(dst, marker);
}

int triggers_compareTEXT(char * marker1, char * marker2)
{
	return stricmp(marker1, marker2);
}

int triggers_getMarkerLength(char * marker)
{
	return strlen(marker);
}