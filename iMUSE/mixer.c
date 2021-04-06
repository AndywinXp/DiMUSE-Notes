#include "mixer.h"
#include "imuse.h"
#include "waveapi.h"

// Validated
// Recheck for type mismatches
int mixer_initModule(iMUSEInitData *initDataPtr, waveOutParams *waveParams) {
	int curTableIndex;
	int amplitudeValue;
	int waveMixChannelsCount;
	int softLdenominator;
	int softLnumerator;
	int softLcurValue;
	int zeroCenterOffset;

	mixer_initDataPtr = initDataPtr;
	mixer_outWordSize = waveParams->bytesPerSample;
	mixer_outChannelCount = waveParams->numChannels;
	mixer_mixBuf = waveParams->mixBuf;
	mixer_mixBufSize = waveParams->offsetBeginMixBuf;
	mixer_stereoReverseFlag = waveParams->sizeSampleKB;

	mixer_amp8Table = (int *)malloc(4352 * sizeof(int));
	mixer_amp12Table = (int *)malloc(69632 * sizeof(int));

	// Equal power volume lookup table, populated from inside-out 
	// (hence the need for softLMID)
	mixer_softLTable = (int *)malloc(16384 * 2 * sizeof(int));
	mixer_softLMID = &mixer_softLTable[16384];

	waveMixChannelsCount = initDataPtr->waveMixCount;

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
		printf("ERR: allocating mixer buffers...\n");
		return -1;
	}
}

// Validated
void mixer_setRadioChatter() {
	mixer_radioChatter = 1;
}

// Validated
void mixer_clearRadioChatter() {
	mixer_radioChatter = 0;
}

// Validated
int mixer_clearMixBuff() {
	if (!mixer_mixBuf)
		return -1;

	memset((void *)mixer_mixBuf, 0, 4 * (mixer_mixBufSize / 4));
	memset((void *)(mixer_mixBuf + 4 * mixer_mixBufSize / 4), 0, mixer_mixBufSize & 3);

	return 0;
}

// Validated
void mixer_mix(uint8 *srcBuf, int inFrameCount, int wordSize, int channelCount, int feedSize, int mixBufStartIndex, int volume, int pan) {
	int *ampTable;
	int rightChannelVolume;
	int leftChannelVolume;
	int channelVolume;
	int channelPan;

	if (mixer_mixBuf) {
		if (srcBuf) {
			if (inFrameCount) {
				if (channelCount == 1 && mixer_outChannelCount == 2) {
					channelVolume = volume / 8;
					if (volume)
						++channelVolume;
					if (channelVolume >= 17)
						channelVolume = 16;
					channelPan = (pan / 8) - 8;
					if (pan > 64)
						++channelPan;
					channelPan = channelVolume == 0 ? 0 : channelPan;

					// Linear volume quantization from the lookup table
					rightChannelVolume = mixer_table[17 * channelVolume + channelPan];
					leftChannelVolume  = mixer_table[17 * channelVolume - channelPan];
					if (wordSize == 8) {
						mixer_mixBits8ConvertToStereo(
							srcBuf,
							inFrameCount,
							feedSize,
							mixBufStartIndex,
							&mixer_amp8Table[leftChannelVolume * 512],
							&mixer_amp8Table[rightChannelVolume * 512]);
					} else {
						if (wordSize == 12) {
							mixer_mixBits12ConvertToStereo(
								srcBuf, 
								inFrameCount, 
								feedSize, 
								mixBufStartIndex, 
								&mixer_amp12Table[leftChannelVolume * 8192], 
								&mixer_amp12Table[rightChannelVolume * 8192]);
						} else {
							mixer_mixBits16ConvertToStereo(
								srcBuf, 
								inFrameCount, 
								feedSize, 
								mixBufStartIndex, 
								&mixer_amp12Table[leftChannelVolume * 8192], 
								&mixer_amp12Table[rightChannelVolume * 8192]);
						}
					}
				} else {
					channelVolume = volume / 8;
					if (volume)
						channelVolume++;
					if (channelVolume >= 17)
						channelVolume = 16;
					if (wordSize == 8)
						ampTable = &mixer_amp8Table[channelVolume * 512];
					else
						ampTable = &mixer_amp12Table[channelVolume * 8192];

					if (mixer_outChannelCount == 1) {
						if (channelCount == 1) {
							if (wordSize == 8) {
								mixer_mixBits8Mono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							} else if (wordSize == 12) {
								mixer_mixBits12Mono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							} else {
								mixer_mixBits16Mono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
							}
						} else if (wordSize == 8) { 
							mixer_mixBits8ConvertToMono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						} else if (wordSize == 12) {
							mixer_mixBits12ConvertToMono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						} else {
							mixer_mixBits16ConvertToMono(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
						}
					} else if (wordSize == 8) {
						mixer_mixBits8Stereo(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					} else if (wordSize == 12) {
						mixer_mixBits12Stereo(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					} else {
						mixer_mixBits16Stereo(srcBuf, inFrameCount, feedSize, mixBufStartIndex, ampTable);
					}
				}
			}
		}
	}
}

// Validated
int mixer_loop(uint8 *destBuffer, unsigned int len) {
	int16 *destBuffer16Bit = (int16 *) destBuffer;
	int16 *mixBuffer = (int16 *) mixer_mixBuf;

	if (!mixer_mixBuf || !destBuffer || !len)
		return -1;

	if (mixer_outChannelCount == 2)
		len *= 2;

	if (!mixer_stereoReverseFlag || mixer_outChannelCount == 1) {
		if (mixer_outWordSize != 16) {
			if (len) {
				for (int i = 0, j = len - 1; j > 0; i++, j--) {
					destBuffer[i] = (uint8) mixer_softLMID[mixBuffer[i]];
				}
			}
			return 0;
		}
		
		if (len) {
			for (int i = 0, j = len - 1; j > 0; i++,j--) {
				destBuffer16Bit[i] = (uint16) mixer_softLMID[2* mixBuffer[i]];
			}
		}
		return 0;
	} else {
		len /= 2;
		if (mixer_outWordSize == 16) {
			if (len) {
				for (int i = 0; i < len; i += 2) {
					destBuffer16Bit[i] = (uint16) mixer_softLMID[2 * mixBuffer[i + 1]];
					destBuffer16Bit[i + 1] = (uint16) mixer_softLMID[2 * mixBuffer[i]];
				}
			}
			return 0;
		}
		
		if (len) {
			for (int i = 0; i < len; i += 2) {
				destBuffer[i] = (uint8) mixer_softLMID[mixBuffer[i + 1]];
				destBuffer[i + 1] = (uint8) mixer_softLMID[mixBuffer[i]];
			}
		}
		return 0;
	}
}

// Validated
void mixer_mixBits8Mono(uint8 * srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	uint8 *ptr;
	int residualLength;
	int value;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[2 * mixBufStartIndex];
	if (inFrameCount == feedSize) {
		if (mixer_radioChatter) {
			ptr = srcBuf + 4;
			value = srcBuf[0] - 128 + srcBuf[1] - 128 + srcBuf[2] - 128 + srcBuf[3] - 128;
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += 4 * (uint16) ampTable[2 * (srcBuf_ptr[i] - value / 4)];
					value += ptr[i] - srcBuf_ptr[i];
				}
			}
		} else {
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16) ampTable[2 * srcBuf_ptr[i]];
				}
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		if (inFrameCount - 1 != 0) {
			for (int i = 0, j = 0; i < inFrameCount - 1; i++, j += 2) {
				mixBufCurCell[j] += (uint16) ampTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[j + 1] += ((int16) ampTable[2 * srcBuf_ptr[i]] + (int16)ampTable[2 * srcBuf_ptr[i + 1]]) / 2;
			}
		}
		mixBufCurCell[inFrameCount] += (uint16) ampTable[2 * srcBuf_ptr[inFrameCount]];
		mixBufCurCell[inFrameCount + 1] += (uint16) ampTable[2 * srcBuf_ptr[inFrameCount]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 2) {
				mixBufCurCell[i] += (uint16) ampTable[2 * srcBuf_ptr[j]];
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += (uint16) ampTable[2 * srcBuf_ptr[0]];
				// Seek the next srcBuf element until there's excess length
				for (residualLength += inFrameCount; residualLength >= 0; ++srcBuf_ptr)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits12Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int value;
	int xorFlag;
	int residualLength;
	int term_1;
	int term_2;

	if ((inFrameCount & 1) != 0) {
		inFrameCount &= 0xFFFFFFFE;
		printf("WARNING: odd inFrameCount with 12-bit data...\n");
	}

	mixBufCurCell = (uint16 *) &mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (inFrameCount / 2) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < inFrameCount / 2; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256)];
				mixBufCurCell[1] += (uint16) ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if ((inFrameCount / 2) - 1) {
			for (int i = 0; i < (inFrameCount / 2) - 1; i++) {
				value = (uint16) ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];

				mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256)];
				mixBufCurCell[1] += ((int16) value + (int16) ampTable[2 * value]) / 2;
				mixBufCurCell[2] += value;
				mixBufCurCell[3] += ((int16) value + (int16) ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF) * 256))]) / 2;
				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] +=  (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
		mixBufCurCell[1] += ((uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
			             +   (uint16) ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))]) / 2;
		mixBufCurCell[2] +=  (uint16) ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
		mixBufCurCell[3] +=  (uint16) ampTable[2 * (srcBuf_ptr[2] | (16 * (srcBuf_ptr[1] & 0xF0)))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {		
				mixBufCurCell[i] += (uint16) ampTable[2 * (srcBuf_ptr[0] | (srcBuf_ptr[1] & 0xF) * 256)];
				srcBuf_ptr += 3;
			}
		}
	} else {
		xorFlag = 0;
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				if (xorFlag) {
					term_1 = (srcBuf_ptr[1] & 0xF0) * 16;
					term_2 = srcBuf_ptr[2];
				} else {
					term_1 = (srcBuf_ptr[1] & 0xF) * 256;
					term_2 = srcBuf_ptr[0];
				}

				mixBufCurCell[i] += (uint16) ampTable[2 * (term_2 | term_1)];
				residualLength += inFrameCount;
				while (residualLength >= 0) {
					residualLength -= feedSize;
					xorFlag ^= 1u;
					if (!xorFlag)
						srcBuf_ptr += 3;
				}
			}
		}
	}
}

// Validated
void mixer_mixBits16Mono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += (uint16) ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		int i;
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += ((int16) ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096]
								   + (int16) ampTable[(int16)(srcBuf_ptr[i + 1] & 0xFFF7) / 8 + 4096]) / 2;
				mixBufCurCell += 2;
			}
		}
		mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[i] & 0xFFF7) / 8 + 4096];
	} else {
		if (2 * feedSize == inFrameCount) {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
					srcBuf_ptr += 2;
				}
			}
		} else {
			residualLength = -inFrameCount;

			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[i] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];

					for (residualLength += inFrameCount; residualLength >= 0; ++srcBuf_ptr)
						residualLength -= feedSize;
				}
			}
		}
	}
}

// Validated
void mixer_mixBits8ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint8 *srcBuf_ptr;
	int residualLength;
	uint16 *mixBufCurCell;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[2 * mixBufStartIndex];
	if (inFrameCount == feedSize) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 2) {
				mixBufCurCell[i] += (2 * (int16)ampTable[2 * srcBuf_ptr[j]]) / 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		if (inFrameCount != 1) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
				int term_1 = ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[3]]) / 2;
				int term_2 = (int16)ampTable[2 * srcBuf_ptr[0]] + (int16)ampTable[2 * srcBuf_ptr[2]];
				mixBufCurCell[1] += ((term_2 / 2) + term_1) / 2;

				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
		mixBufCurCell[0] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
		mixBufCurCell[1] += ((int16)ampTable[2 * srcBuf_ptr[1]] + (int16)ampTable[2 * srcBuf_ptr[0]]) / 2;
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			for (int i = 0, j = 0; i < feedSize; i++, j += 4) {
				mixBufCurCell[i] += (2 * (int16)ampTable[2 * srcBuf_ptr[j]]) / 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16)ampTable[2 * srcBuf_ptr[0]] + (int16)ampTable[2 * srcBuf_ptr[1]]) / 2;
				// Skip srcBuf elements until there's excess length
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits12ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint8 *srcBuf_ptr; 
	int residualLength;
	uint16 *mixBufCurCell;
	int term_1;
	int term_2;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				term_1 = srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256);
				term_2 = (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
				
				mixBufCurCell[i] += ((int16) ampTable[2 * term_1] + term_2) / 2;
				srcBuf_ptr += 3;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				term_1 = (int16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF0) * 256))];
				term_2 = (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF)  * 16))];

				mixBufCurCell[0] += (term_1 + term_2) / 2;
				mixBufCurCell[1] += ((term_1 + (int16) ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF)  * 256))]) / 2)
				                  + ((term_2 + (int16) ampTable[2 * (srcBuf_ptr[5] | ((srcBuf_ptr[4] & 0xF0) * 16))]) / 2) / 2;
				
				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}

		mixBufCurCell[0] += ((int16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
					       + (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;
		mixBufCurCell[1] += ((int16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
			               + (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;


	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				
				mixBufCurCell[i] += ((int16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))] 
								   + (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;

				srcBuf_ptr += 6;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
								   + (int16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;

				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 3)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits16ConvertToMono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *)&mixer_mixBuf[2 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16) ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]
					               + (int16) ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]) / 2;
				srcBuf_ptr += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] +=   ((int16) ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]
								   +   (int16) ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]) / 2;

				mixBufCurCell[1] += ((((int16) ampTable[((int16)(srcBuf_ptr[0] & 0xFFF7) / 8) + 4096]
					               +   (int16) ampTable[((int16)(srcBuf_ptr[2] & 0xFFF7) / 8) + 4096]) / 2)
					               + (((int16) ampTable[((int16)(srcBuf_ptr[1] & 0xFFF7) / 8) + 4096]
						           +   (int16) ampTable[((int16)(srcBuf_ptr[3] & 0xFFF7) / 8) + 4096]) / 2)) / 2;
				mixBufCurCell += 2;
				srcBuf_ptr += 2;
			}
		}

		mixBufCurCell[0] += ((int16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]
						   + (int16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]) / 2;
		mixBufCurCell[1] += ((int16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096] 
			               + (int16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]) / 2;
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
								   + (int16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]) / 2;
				srcBuf_ptr += 4;
			}
		}
	} else {
		residualLength = -inFrameCount;
		
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[i] += ((int16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
								   + (int16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096]) / 2;
				
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits8ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	uint8 *ptr;
	int value;
	int residualLength;
	
	mixBufCurCell = (uint16 *) &mixer_mixBuf[4 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (mixer_radioChatter) {
			srcBuf_ptr = srcBuf;
			ptr = srcBuf + 4;
			value = srcBuf[0] - 128 + srcBuf[1] - 128 + srcBuf[2] - 128 + srcBuf[3] - 128;
			if (feedSize) {
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += 4 * (uint16)  leftAmpTable[2 * (srcBuf_ptr[i] - (value / 4))];
					mixBufCurCell[1] += 4 * (uint16) rightAmpTable[2 * (srcBuf_ptr[i] - (value / 4))];
					value += ptr[i] - srcBuf_ptr[i];
					mixBufCurCell += 2;
				}
			}
		} else {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += (uint16)  leftAmpTable[2 * srcBuf_ptr[i]];
					mixBufCurCell[1] += (uint16) rightAmpTable[2 * srcBuf_ptr[i]];
					mixBufCurCell += 2;
				}
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		int i;
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[1] += (uint16) rightAmpTable[2 * srcBuf_ptr[i]];
				mixBufCurCell[2] += ((int16)  leftAmpTable[2 * srcBuf_ptr[i]] + (int16)  leftAmpTable[2 * srcBuf_ptr[i]]) / 2;
				mixBufCurCell[3] += ((int16) rightAmpTable[2 * srcBuf_ptr[i]] + (int16) rightAmpTable[2 * srcBuf_ptr[i]]) / 2;
			}
		}
		mixBufCurCell[0] += (uint16)  leftAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[1] += (uint16) rightAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[2] += (uint16)  leftAmpTable[2 * srcBuf_ptr[i]];
		mixBufCurCell[3] += (uint16) rightAmpTable[2 * srcBuf_ptr[i]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[2 * srcBuf_ptr[1]];
				mixBufCurCell[1] += (uint16) rightAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell += 2;
				srcBuf_ptr += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)leftAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)rightAmpTable[2 * srcBuf_ptr[0]];
				mixBufCurCell += 2;

				for (residualLength += inFrameCount; residualLength > 0; ++srcBuf_ptr)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits12ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;

	int xorFlag;
	int residualLength;

	int term_1;
	int term_2;
	int term_3;
	int term_4;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[4 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (inFrameCount / 2) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < (inFrameCount / 2); i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] += (uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[2] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
				mixBufCurCell[3] += (uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if ((inFrameCount / 2) - 1 != 0) {
			for (int i = 0; i < (inFrameCount / 2) - 1; i++) {
				mixBufCurCell[0] +=  (uint16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] +=  (uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];

				mixBufCurCell[2] += ((uint16)  leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]
								   + (uint16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]) / 2;

				mixBufCurCell[3] += ((uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]
								   + (uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]) / 2;

				mixBufCurCell[4] += (uint16)   leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
				mixBufCurCell[5] += (uint16)  rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];

				mixBufCurCell[6] += ((uint16)  leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))] +
									  (int16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[4] & 0xF) * 256))]) / 2;

				mixBufCurCell[7] += ((uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))] +
									  (int16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]) / 2;

				srcBuf_ptr += 3;
				mixBufCurCell += 8;
			}
		}

		mixBufCurCell[0] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
		mixBufCurCell[1] += (uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];

		mixBufCurCell[2] += ((uint16) leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
						   + (uint16) leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;

		mixBufCurCell[3] += ((uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
						   + (uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]) / 2;

		mixBufCurCell[4] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
		mixBufCurCell[5] += (uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
		mixBufCurCell[6] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
		mixBufCurCell[7] += (uint16) rightAmpTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] += (uint16) rightAmpTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else {
		xorFlag = 0;
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				if (xorFlag) {
					term_2 = (srcBuf_ptr[1] & 0xF0) * 16;
					term_1 = srcBuf_ptr[2];
				} else {
					term_2 = (srcBuf_ptr[1] & 0xF) * 256;
					term_1 = srcBuf_ptr[0];
				}
				mixBufCurCell[0] += (uint16) leftAmpTable[2 * (term_1 | term_2)];

				if (xorFlag) {
					term_4 = (srcBuf_ptr[1] & 0xF0) * 16;
					term_3 = srcBuf_ptr[2];
				} else {
					term_4 = (srcBuf_ptr[1] & 0xF) * 256;
					term_3 = srcBuf_ptr[0];
				}
				mixBufCurCell[1] += (uint16) rightAmpTable[2 * (term_3 | term_4)];

				residualLength += inFrameCount;
				while (residualLength > 0) {
					residualLength -= feedSize;
					xorFlag ^= 1u;
					if (!xorFlag)
						srcBuf_ptr += 3;
				}

				mixBufCurCell += 2;
			}
		}
	}
}

// Validated
void mixer_mixBits16ConvertToStereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable) {
	uint16* mixBufCurCell;
	uint16 *srcBuf_tmp;
	int residualLength;

	mixBufCurCell = (uint16*) &mixer_mixBuf[4 * mixBufStartIndex];

	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16) rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_tmp = srcBuf;
		int i;
		if (inFrameCount - 1 != 0) {
			for (i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] +=  (uint16)  leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] +=  (uint16) rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];

				mixBufCurCell[2] += ((int16) leftAmpTable[(int16)(srcBuf_tmp[i]     & 0xFFF7) / 8 + 4096]
				                 +   (int16) leftAmpTable[(int16)(srcBuf_tmp[i + 1] & 0xFFF7) / 8 + 4096]) / 2;
				
				mixBufCurCell[3] += ((int16) rightAmpTable[((int16)(srcBuf_tmp[i]     & 0xFFF7) / 8) + 4096]
					             +   (int16) rightAmpTable[((int16)(srcBuf_tmp[i + 1] & 0xFFF7) / 8) + 4096]) / 2;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] += (uint16)  leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16) rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[2] += (uint16)  leftAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[3] += (uint16) rightAmpTable[(int16)(srcBuf_tmp[i] & 0xFFF7) / 8 + 4096];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16) rightAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				srcBuf_tmp += 2;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_tmp = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)  leftAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16) rightAmpTable[(int16)(srcBuf_tmp[0] & 0xFFF7) / 8 + 4096];
				
				for (residualLength += inFrameCount; residualLength > 0; ++srcBuf_tmp)
					residualLength -= feedSize;
				
				mixBufCurCell += 2;
			}
		}
	}
}

// Validated
void mixer_mixBits8Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16*) &mixer_mixBuf[4 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16)ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16)ampTable[2 * srcBuf_ptr[1]];
				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16) ampTable[2 * srcBuf_ptr[1]];
				mixBufCurCell[2] += ((int16) ampTable[2 * srcBuf_ptr[0]] + (int16) ampTable[2 * srcBuf_ptr[2]]) / 2;
				mixBufCurCell[3] += ((int16) ampTable[2 * srcBuf_ptr[1]] + (int16) ampTable[2 * srcBuf_ptr[3]]) / 2;
				mixBufCurCell += 4;			 
				srcBuf_ptr += 2;
			}
		}
		mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0]];
		mixBufCurCell[1] += (uint16) ampTable[2 * srcBuf_ptr[1]];
		mixBufCurCell[2] += (uint16) ampTable[2 * srcBuf_ptr[0]];
		mixBufCurCell[3] += (uint16) ampTable[2 * srcBuf_ptr[1]];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16) ampTable[2 * srcBuf_ptr[1]];
				srcBuf_ptr += 4;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;
			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * srcBuf_ptr[0]];
				mixBufCurCell[1] += (uint16) ampTable[2 * srcBuf_ptr[1]];
				mixBufCurCell += 2;
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits12Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint8 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16 *) &mixer_mixBuf[4 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] += (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];

				srcBuf_ptr += 3;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) {
		srcBuf_ptr = srcBuf;
		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] +=  (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
									 		  
				mixBufCurCell[1] +=  (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
											  
				mixBufCurCell[2] += ((uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))]
									+ (int16) ampTable[2 * (srcBuf_ptr[3] | ((srcBuf_ptr[4] & 0xF) * 256))]) / 2;
											  
				mixBufCurCell[3] += ((uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))]
									+ (int16) ampTable[2 * (srcBuf_ptr[5] | ((srcBuf_ptr[4] & 0xF0) * 16))]) / 2;

				srcBuf_ptr += 3;
				mixBufCurCell += 4;
			}
		}
		mixBufCurCell[0] += (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
		mixBufCurCell[1] += (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
		mixBufCurCell[2] += (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
		mixBufCurCell[3] += (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];
	} else if (2 * feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] += (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];

				srcBuf_ptr += 6;
				mixBufCurCell += 2;
			}
		}
	} else {
		residualLength = -inFrameCount;
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[2 * (srcBuf_ptr[0] | ((srcBuf_ptr[1] & 0xF) * 256))];
				mixBufCurCell[1] += (uint16) ampTable[2 * (srcBuf_ptr[2] | ((srcBuf_ptr[1] & 0xF0) * 16))];

				mixBufCurCell += 2;
				for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 3)
					residualLength -= feedSize;
			}
		}
	}
}

// Validated
void mixer_mixBits16Stereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable) {
	uint16 *mixBufCurCell;
	uint16 *srcBuf_ptr;
	int residualLength;

	mixBufCurCell = (uint16*)&mixer_mixBuf[4 * mixBufStartIndex];
	if (feedSize == inFrameCount) {
		if (feedSize) {
			srcBuf_ptr = srcBuf;

			for (int i = 0; i < feedSize; i++) {
				mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
				srcBuf_ptr += 2;
				mixBufCurCell += 2;
			}
		}
	} else if (2 * inFrameCount == feedSize) { 
		srcBuf_ptr = srcBuf;

		if (inFrameCount - 1 != 0) {
			for (int i = 0; i < inFrameCount - 1; i++) {
				mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
				mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];

				mixBufCurCell[2] += ((int16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096]
								   + (int16) ampTable[(int16)(srcBuf_ptr[2] & 0xFFF7) / 8 + 4096]) / 2;
				
				mixBufCurCell[4] += ((int16) ampTable[((int16)((*((uint8*)srcBuf_ptr + 2) | (*((uint8 *)srcBuf_ptr + 3) * 256)) & 0xFFF7) / 8) + 4096]
								   + (int16) ampTable[((int16)(srcBuf_ptr[3] & 0xFFF7) / 8) + 4096]) / 2;
				mixBufCurCell += 4;
				srcBuf_ptr += 2;
			}
		}
		mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[2] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
		mixBufCurCell[3] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
	} else {
		if (2 * feedSize == inFrameCount) {
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {			
					mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];																	  
					mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
					mixBufCurCell += 2;
					srcBuf_ptr += 4;
				}
			}
		} else {
			residualLength = -inFrameCount;
			if (feedSize) {
				srcBuf_ptr = srcBuf;
				for (int i = 0; i < feedSize; i++) {
					mixBufCurCell[0] += (uint16) ampTable[(int16)(srcBuf_ptr[0] & 0xFFF7) / 8 + 4096];
					mixBufCurCell[1] += (uint16) ampTable[(int16)(srcBuf_ptr[1] & 0xFFF7) / 8 + 4096];
					
					for (residualLength += inFrameCount; residualLength >= 0; srcBuf_ptr += 2)
						residualLength -= feedSize;
				}
			}
		}
	}
}

// Validated
void mixer_free() {
	free(mixer_amp8Table);
	free(mixer_amp12Table);
	free(mixer_softLTable);
	mixer_amp8Table = NULL;
	mixer_amp12Table = NULL;
	mixer_softLTable = NULL;
}

