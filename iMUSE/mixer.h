#ifndef DIGITAL_IMUSE_MIXER
#define DIGITAL_IMUSE_MIXER

// Remove this junk when porting to ScummVM

typedef          char   int8;
typedef unsigned char   uint8;
typedef          short  int16;
typedef unsigned short  uint16;

iMUSEInitData *mixer_initDataPtr;
int *mixer_amp8Table;
int *mixer_amp12Table;
int *mixer_softLMID;
int *mixer_softLTable;
int *mixer_mixBuf;

int mixer_mixBufSize;
int mixer_radioChatter = 0;
int mixer_outWordSize;
int mixer_outChannelCount;
int mixer_stereoReverseFlag;

// Lookup table for a linear volume ramp (0 to 16) accounting for panning (-8 to 8)
// This is for The Dig, it appears to be different for COMI
int8 mixer_table[] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
					   0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  1,  1,  1,  1,  2,  2,  2,
					   2,  2,  3,  3,  3,  3,  3,  3,  0,  0,  1,  1,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  0,
					   0,  1,  1,  2,  2,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
					   5,  5,  6,  6,  6,  6,  6,  0,  1,  1,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  7,  7,  7,  7,  0,  1,
					   2,  2,  3,  4,  4,  5,  6,  6,  7,  7,  7,  8,  8,  8,  8,  0,  1,  2,  3,  3,  4,  5,  6,  6,  7,  7,
					   8,  8,  9,  9,  9,  9,  0,  1,  2,  3,  4,  5,  6,  6,  7,  8,  8,  9,  9,  10, 10, 10, 10, 0,  1,  2,
					   3,  4,  5,  6,  7,  8,  9,  9,  10, 10, 11, 11, 11, 11, 0,  1,  2,  3,  5,  6,  7,  8,  8,  9,  10, 11,
					   11, 11, 12, 12, 12, 0,  1,  3,  4,  5,  6,  7,  8,  9,  10, 11, 11, 12, 12, 13, 13, 13, 0,  1,  3,  4,
					   5,  7,  8,  9,  10, 11, 12, 12, 13, 13, 14, 14, 14, 0,  1,  3,  4,  6,  7,  8,  10, 11, 12, 12, 13, 14,
					   14, 15, 15, 15, 0,  2,  3,  5,  6,  8,  9,  10, 11, 12, 13, 14, 15, 15, 16, 16, 16, 0,  0,  0};



int  mixer_initModule(iMUSEInitData *initDataPtr, waveOutParams *waveParams);

void mixer_setRadioChatter();
void mixer_clearRadioChatter();
int  mixer_clearMixBuff();

void mixer_mix(uint8 *srcBuf, int inFrameCount, int wordSize, int channelCount, int feedSize, int mixBufStartIndex, int volume, int pan);
int  mixer_loop(uint8 *destBuffer, unsigned int len);

void mixer_mixBits8Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits12Mono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits16Mono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

void mixer_mixBits8ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits12ConvertToMono(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits16ConvertToMono(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

void mixer_mixBits8ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
void mixer_mixBits12ConvertToStereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);
void mixer_mixBits16ConvertToStereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *leftAmpTable, int *rightAmpTable);

void mixer_mixBits8Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits12Stereo(uint8 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);
void mixer_mixBits16Stereo(uint16 *srcBuf, int inFrameCount, int feedSize, int mixBufStartIndex, int *ampTable);

void mixer_free();

#endif