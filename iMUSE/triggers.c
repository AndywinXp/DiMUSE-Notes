#include "triggers.h"

// Validated
int triggers_moduleInit(iMUSEInitData *initDataPtr) {
	triggers_initDataPtr = initDataPtr;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

// Validated
int triggers_clear() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

// Validated
int triggers_save(int * buffer, int bufferSize) {
	if (bufferSize < 2848) {
		return -5;
	}
	memcpy(buffer, trigs, 2464);
	memcpy(buffer + 2464, defers, 384);
	return 2848;
}

// Validated
int triggers_restore(int * buffer) {
	memcpy(trigs, buffer, 2464);
	memcpy(defers, buffer + 2464, 384);
	triggers_defersOn = 1;
	return 2848;
}

// Validated
int triggers_setTrigger(int soundId, char *marker, int opcode) {
	if (soundId == 0) {
		return -5;
	}

	if (marker == NULL) {
		marker = &triggers_empty_marker;
	}

	if (triggers_getMarkerLength(marker) >= 256) {
		printf("ERR: attempting to set trig with oversize marker string...");
		return -5;
	}

	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (trigs[l].sound == 0) {
			trigs[l].sound = soundId;
			trigs[l].clearLater = 0;
			trigs[l].opcode = opcode;
			triggers_copyTEXT(trigs[l].text, marker);
			int* ptr = &opcode; // args after opcode
			trigs[l].args_0_ = *(int *)(ptr + 4);
			trigs[l].args_1_ = *(int *)(ptr + 8);
			trigs[l].args_2_ = *(int *)(ptr + 12);
			trigs[l].args_3_ = *(int *)(ptr + 16);
			trigs[l].args_4_ = *(int *)(ptr + 20);
			trigs[l].args_5_ = *(int *)(ptr + 24);
			trigs[l].args_6_ = *(int *)(ptr + 28);
			trigs[l].args_7_ = *(int *)(ptr + 32);
			trigs[l].args_8_ = *(int *)(ptr + 36);
			trigs[l].args_9_ = *(int *)(ptr + 40);
			return 0;
		}
	}
	printf("ERR: tr unable to alloc trigger...");
	return -6;
}

// Validated
int triggers_checkTrigger(int soundId, char *marker, int opcode) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (trigs[l].sound != 0) {
			if (soundId == -1 || trigs[l].sound == soundId) {
				if (marker == -1 || !triggers_compareTEXT(marker, trigs[l].text)) {
					if (opcode == -1 || trigs[l].opcode == opcode)
						r++;
				}
			}
		}
	}

	return r;
}

// Validated
int triggers_clearTrigger(int soundId, char *marker, int opcode) {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if ((trigs[l].sound != 0) && (soundId == -1 || trigs[l].sound == soundId) &&
			(marker || !triggers_compareTEXT(marker, trigs[l].text)) &&
			(opcode == -1 || trigs[l].opcode == opcode)) {
			
			if (triggers_midProcessing) {
				trigs[l].clearLater = 1;
			} else {
				trigs[l].sound = 0;
			}
		}
	}
	return 0;
}

// Validated
void triggers_processTriggers(int soundId, char *marker) {
	char textBuffer[256];
	int r;
	if (triggers_getMarkerLength(marker) >= 256) {
		printf("ERR: TgProcessMarker() passed oversize marker string...");
		return;
	}
	triggers_copyTEXT(triggers_textBuffer, marker);
	triggers_midProcessing++;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if ((trigs[l].sound && trigs[l].sound == soundId) &&
			(trigs[l].text[0] == '\0' || triggers_compareTEXT(triggers_textBuffer, trigs[l].text))) {
			
			// Save the string into our local buffer for later
			r = 0;
			if (triggers_textBuffer[0] != '\0') {
				do {
					textBuffer[r] = triggers_textBuffer[r];
					r++;
				} while (triggers_textBuffer[r] != '\0');
			}
			textBuffer[r] = '\0';

			trigs[l].sound = 0;
			if (trigs[l].opcode == NULL) {
				if (triggers_initDataPtr == NULL || triggers_initDataPtr->scriptCallback == NULL) {
					printf("ERR: null callback in InitData struct...");
				} else {
					// Call the script callback (a function which sets _stoppingSequence to 1)
					triggers_initDataPtr->scriptCallback(triggers_textBuffer, trigs[l].args_0_,
						trigs[l].args_1_, trigs[l].args_2_,
						trigs[l].args_3_, trigs[l].args_4_,
						trigs[l].args_5_, trigs[l].args_6_,
						trigs[l].args_7_, trigs[l].args_8_,
						trigs[l].args_9_);
				}
			} else {
				if ((int) trigs[l].opcode < 30) {
					// Execute a command
					cmds_handleCmds((int) trigs[l].opcode, trigs[l].args_0_,
						trigs[l].args_1_, trigs[l].args_2_,
						trigs[l].args_3_, trigs[l].args_4_,
						trigs[l].args_5_, trigs[l].args_6_,
						trigs[l].args_7_, trigs[l].args_8_,
						trigs[l].args_9_, -1, -1, -1, -1);
				} else {
					// The opcode field is not a command opcode but a pointer to function
					// This is used for speech only
					int(*func)() = trigs[l].opcode;
					func(triggers_textBuffer, trigs[l].args_0_,
						trigs[l].args_1_, trigs[l].args_2_,
						trigs[l].args_3_, trigs[l].args_4_,
						trigs[l].args_5_, trigs[l].args_6_,
						trigs[l].args_7_, trigs[l].args_8_,
						trigs[l].args_9_);
				}
			}

			// Restore the global textBuffer
			r = 0;
			if (textBuffer[0] != '\0') {
				do {
					triggers_textBuffer[r] = textBuffer[r];
					r++;
				} while (textBuffer[r] != '\0');
			}
			triggers_textBuffer[r] = '\0';
		}
	}
	if (--triggers_midProcessing == 0) {
		for (int l = 0; l < MAX_TRIGGERS; l++) {
			if (trigs[l].clearLater) {
				trigs[l].sound = 0;
			}
		}
	}
}

// Validated
int triggers_deferCommand(int count, int opcode) {
	if (!count) {
		return -5;
	}
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter) {
			defers[l].counter = count;
			defers[l].opcode = opcode;
			int* ptr = &opcode;
			defers[l].args_0_ = *(int *)(ptr + 4);
			defers[l].args_1_ = *(int *)(ptr + 8);
			defers[l].args_2_ = *(int *)(ptr + 12);
			defers[l].args_3_ = *(int *)(ptr + 16);
			defers[l].args_4_ = *(int *)(ptr + 20);
			defers[l].args_5_ = *(int *)(ptr + 24);
			defers[l].args_6_ = *(int *)(ptr + 28);
			defers[l].args_7_ = *(int *)(ptr + 32);
			defers[l].args_8_ = *(int *)(ptr + 36);
			defers[l].args_9_ = *(int *)(ptr + 40);
			triggers_defersOn = 1;
			return 0;
		}
	}
	printf("ERR: tr unable to alloc deferred cmd...");
	return -6;
}

// Validated
void triggers_loop() {
	if (!triggers_defersOn)
		return;

	triggers_defersOn = 0;
	for (int l = 0; l < MAX_DEFERS; l++) {
		if (defers[l].counter == 0)
			continue;

		triggers_defersOn = 1;
		defers[l].counter--;

		// For some reason it processes the defer at counter 1 instead of 0...
		// Oh well
		if (defers[l].counter == 1) {
			if (defers[l].opcode != NULL) {
				if ((int) defers[l].opcode < 30) {
					cmds_handleCmds((int) trigs[l].opcode,
						trigs[l].args_0_, trigs[l].args_1_,
						trigs[l].args_2_, trigs[l].args_3_,
						trigs[l].args_4_, trigs[l].args_5_,
						trigs[l].args_6_, trigs[l].args_7_,
						trigs[l].args_8_, trigs[l].args_9_, -1, -1, -1, -1);
				} else {
					int(*func)() = trigs[l].opcode;
					func(trigs[l].text,
						trigs[l].args_0_, trigs[l].args_1_,
						trigs[l].args_2_, trigs[l].args_3_,
						trigs[l].args_4_, trigs[l].args_5_,
						trigs[l].args_6_, trigs[l].args_7_,
						trigs[l].args_8_, trigs[l].args_9_);
				}
			} else {
				if (triggers_initDataPtr->scriptCallback == NULL) {
					printf("ERR: null callback in InitData struct...");
				} else {
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
}

// Validated
int triggers_countPendingSounds(int soundId) {
	int r = 0;
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		if (!trigs[l].sound)
			continue;

		int opcode = trigs[l].opcode;
		if ((opcode == 8 && trigs[l].args_0_ == soundId) || (opcode == 26 && trigs[l].args_1_ == soundId))
			r++;
	}

	for (int l = 0; l < MAX_DEFERS; l++) {
		if (!defers[l].counter)
			continue;

		int opcode = defers[l].opcode;
		if ((opcode == 8 && defers[l].args_0_ == soundId) || (opcode == 26 && defers[l].args_1_ == soundId))
			r++;
	}

	return r;
}

// Validated
int triggers_moduleFree() {
	for (int l = 0; l < MAX_TRIGGERS; l++) {
		trigs[l].sound = 0;
		defers[l].counter = 0;
	}
	triggers_defersOn = 0;
	triggers_midProcessing = 0;
	return 0;
}

// Validated
// Probably just a strcpy, but if there's undefined behavior it's best to match it
void triggers_copyTEXT(char *dst, char *marker) {
	char currentChar;

	if ((dst != NULL) && (marker != NULL)) {
		do {
			currentChar = *marker;
			marker = marker + 1;
			*dst = currentChar;
			dst = dst + 1;
		} while (currentChar != '\0');
	}
	return;
}

// Validated
// Probably just a stricmp, but if there's undefined behavior it's best to match it
int triggers_compareTEXT(char *marker1, char *marker2) {
	if (*marker1 != 0) {
		while ((*marker2 != 0 && (*marker1 == *marker2))) {
			marker1 = marker1 + 1;
			marker2 = marker2 + 1;
			if (*marker1 == 0) {
				return (int)(char)(*marker2 | *marker1);
			}
		}
	}
	return (int)(char)(*marker2 | *marker1);
}

// Validated
// Probably just a strlen, but if there's undefined behavior it's best to match it
int triggers_getMarkerLength(char *marker) {
	int resultingLength;
	char curChar;

	resultingLength = 0;
	curChar = *marker;
	while (curChar != '\0') {
		marker = marker + 1;
		resultingLength += 1;
		curChar = *marker;
	}
	return resultingLength;
}