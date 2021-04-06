#include <stdio.h>
#include <stdlib.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
int	mixer_outChannelCount;
int	mixer_stereoReverseFlag;
int mixer_outWordSize;
int mixer_radioChatter = 0;
int *mixer_amp8Table;
int *mixer_amp12Table;
int *mixer_softLMID;
int *mixer_softLTable;


int mixer_initModule() {
	int curTableIndex;
	int amplitudeValue;
	int waveMixChannelsCount;
	int softLdenominator;
	int softLnumerator;
	int softLcurValue;
	int zeroCenterOffset;

	mixer_outWordSize = 16;
	mixer_outChannelCount = 2;
	mixer_stereoReverseFlag = 0;

	mixer_amp8Table = (int *)malloc(4352 * sizeof(int));
	mixer_amp12Table = (int *)malloc(69632 * sizeof(int));

	// Equal power volume lookup table, populated from inside-out 
	// (hence the need for softLMID)
	mixer_softLTable = (int *)malloc(16384 * 2 * sizeof(int));
	mixer_softLMID = &mixer_softLTable[16384]; 

	waveMixChannelsCount = 6;

	if (mixer_amp8Table && mixer_amp12Table && mixer_softLTable) {
		zeroCenterOffset = 0;
		curTableIndex = 0;
		for (int i = 0; i < 17; i++) {
			amplitudeValue = -2048 * zeroCenterOffset;
			for (int j = 0; j < 256; j++) {
				mixer_amp8Table[curTableIndex] = amplitudeValue / 127;
				amplitudeValue += 16 * zeroCenterOffset;
				curTableIndex++;
			}

			zeroCenterOffset += 8;
			if (zeroCenterOffset == 8)
				zeroCenterOffset = 7;
		}

		zeroCenterOffset = 0;
		curTableIndex = 0;
		for (int i = 0; i < 17; i++) {
			amplitudeValue = -2048 * zeroCenterOffset;
			for (int j = 0; j < 4096; j++) {
				mixer_amp12Table[curTableIndex] = amplitudeValue / 127;
				amplitudeValue += zeroCenterOffset;
				curTableIndex++;
			}

			zeroCenterOffset += 8;
			if (zeroCenterOffset == 8)
				zeroCenterOffset = 7;
		}

		
		if (mixer_outWordSize == 8) {
			if (waveMixChannelsCount * 2048 > 0) {
				softLnumerator = 0;
				softLdenominator = 2047 * waveMixChannelsCount;
				for (int i = 0; i < waveMixChannelsCount * 2048; i++) {
					softLcurValue = softLnumerator / softLdenominator + 1;
					softLdenominator += waveMixChannelsCount - 1;
					softLnumerator += 254 * waveMixChannelsCount;
					softLcurValue /= 2;

					mixer_softLMID[i] = softLcurValue + 128;
					mixer_softLMID[-i] = 127 - softLcurValue;
				}
			}
		} else if (waveMixChannelsCount * 2048 > 0) {
			softLdenominator = 2047 * waveMixChannelsCount;
			softLnumerator = 0;
			for (int i = 0; i < waveMixChannelsCount * 2048; i++) {
				softLcurValue = softLnumerator / softLdenominator + 1;
				softLcurValue /= 2;
				softLdenominator += waveMixChannelsCount - 1;
				softLnumerator += 65534 * waveMixChannelsCount;

				mixer_softLMID[i] = softLcurValue;
				mixer_softLMID[-i - 1] = -1 - softLcurValue;
			}
		}
		//vh_initModule("iMUSE:source:wave:mixer.c", "17:49:03", "Jul 11 1996", (int)MIXER_FREE, (int)MIXER_DEBUG, 0);
		return 0;
	} else {
		printf("ERR:allocating mixer buffers...\n");
		return -1;
	}
}

int main() {
	mixer_initModule();
	return 0;
}