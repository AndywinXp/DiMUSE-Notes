#include "fades.h"

// Validated
int fades_moduleInit() {
	for (int l = 0; l < MAX_FADES; l++) {
		fades[l].status = 0;
	}
	fadesOn = 0;
	return 0;
}

// Validated
int fades_moduleDeinit() {
	return 0;
}

// Validated
int fades_save(unsigned char *buffer, int sizeLeft) {
	// We're saving 640 bytes:
	// which means 10 ints (4 bytes each) for 16 times (number of fades)
	if (sizeLeft < 640)
		return -5;
	memcpy(buffer, fades, 640);
	return 640;
}

// Validated
int fades_restore(unsigned char *buffer) {
	memcpy(fades, buffer, 640);
	fadesOn = 1;
	return 640;
}

// Validated
int fades_fadeParam(int soundId, int opcode, int destinationValue, int fadeLength) {
	if (!soundId || fadeLength < 0)
		return -5;
	if (opcode != 0x500 && opcode != 0x600 && opcode != 0x700 && opcode != 0x800 && opcode != 0xF00 && opcode != 17)
		return -5;
	
	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status && fades[l].sound == soundId && (fades[l].param == opcode || opcode == -1)) {
			fades[l].status = 0;
		}
	}

	if (!fadeLength) {
		if (opcode != 0x600 || destinationValue) {
			// IMUSE_CMDS_SetParam
			cmds_handleCmds(12, soundId, opcode, destinationValue, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			return 0;
		} else {
			// IMUSE_CMDS_StopSound
			cmds_handleCmds(9, soundId, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			return 0;
		}
	}

	for (int l = 0; l < MAX_FADES; l++) {
		if (!fades[l].status) {
			fades[l].sound = soundId;
			fades[l].param = opcode;
			// IMUSE_CMDS_GetParam (probably fetches current volume, with opcode 0x600)
			fades[l].currentVal = cmds_handleCmds(13, soundId, opcode, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			fades[l].length = fadeLength;
			fades[l].counter = fadeLength;
			fades[l].slope = (destinationValue - fades[l].currentVal) / fadeLength;

			if ((destinationValue - fades[l].currentVal) < 0) {
				fades[l].nudge = -1;
				fades[l].slopeMod = (-fades[l].slope % fadeLength);
			} else {
				fades[l].nudge = 1;
				fades[l].slopeMod = fades[l].slope % fadeLength;
			}

			fades[l].modOvfloCounter = 0;
			fades[l].status = 1;
			fadesOn = 1;
		}	
	}

	printf("ERROR: fd unable to alloc fade...\n");
	return -6;
}

// Validated
void fades_clearFadeStatus(int soundId, int opcode) {
	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status == 0 
			&& fades[l].sound == soundId 
			&& (fades[l].param == opcode || opcode == -1)) {
			fades[l].status = 0;
		}
	}
}

// Validated
void fades_loop() {
	if (!fadesOn)
		return;
	fadesOn = 0;

	for (int l = 0; l < MAX_FADES; l++) {
		if (fades[l].status) {
			fadesOn = 1;
			if (--fades[l].counter == 0) {
				fades[l].status = 0;
			}

			int currentVolume = fades[l].currentVal + fades[l].slope;
			int currentSlopeMod = fades[l].modOvfloCounter + fades[l].slopeMod;
			fades[l].modOvfloCounter += fades[l].slopeMod;

			if (fades[l].length <= currentSlopeMod) {
				fades[l].modOvfloCounter = currentSlopeMod - fades[l].length;
				currentVolume += fades[l].nudge;
			}

			if (fades[l].currentVal != currentVolume) {
				fades[l].currentVal = currentVolume;

				if ((fades[l].counter % 6) == 0) {
					if ((fades[l].param != 0x600) || currentVolume != 0) {
						// IMUSE_CMDS_SetParam
						cmds_handleCmds(12, fades[l].sound, fades[l].param, currentVolume, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
						continue;
					} else {
						// IMUSE_CMDS_StopSound
						cmds_handleCmds(9, fades[l].sound, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
					}
				}
			}
		}
	}
}

// Validated
void fades_moduleFree() {
	for (int l = 0; l < MAX_FADES; l++) {
		fades[l].status = 0;
	}
	fadesOn = 0;
}

// Validated
int fades_moduleDebug() {
	printf("fadesOn: %d", fadesOn);

	for (int l = 0; l < MAX_FADES; l++) {
		printf("\n");
		printf("fades[%d]: \n", l);
		printf("status: %d \n", fades[l].status);
		printf("sound: %d \n", fades[l].sound);
		printf("param: %d \n", fades[l].param);
		printf("currentVal: %d \n", fades[l].currentVal);
		printf("counter: %d \n", fades[l].counter);
		printf("length: %d \n", fades[l].length);
		printf("slope: %d \n", fades[l].slope);
		printf("slopeMod: %d \n", fades[l].slopeMod);
		printf("modOvfloCounter: %d \n", fades[l].modOvfloCounter);
		printf("nudge: %d \n", fades[l].nudge);
	}	
	return 0;
}
