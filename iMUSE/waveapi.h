#ifndef DIGITAL_IMUSE_WAVEAPI
#define DIGITAL_IMUSE_WAVEAPI

#include "imuse.h"

#define NUM_HEADERS 8

typedef struct {
	int bytesPerSample;
	int numChannels;
	int mixBuf;
	int offsetBeginMixBuf;
	int sizeSampleKB;
} waveOutParams;

WAVEFORMATEX waveapi_waveFormat;
waveOutParams waveapi_waveOutParams;
LPWAVEHDR *waveHeaders;
HWAVEOUT waveHandle;

int waveapi_sampleRate;
int waveapi_bytesPerSample;
int waveapi_numChannels;
int waveapi_zeroLevel;
int *waveapi_mixBuf;
int *waveapi_outBuf;

int waveapi_xorTrigger = 0;
int waveapi_writeIndex = 0;
int waveapi_disableWrite = 0;

int waveapi_moduleInit(int sampleRate, waveOutParams *waveoutParamStruct);
void waveapi_write(LPSTR *lpData, int *feedSize, int *sampleRate);
int waveapi_free();
void waveapi_callback();
void waveapi_increaseSlice();
int waveapi_decreaseSlice();

#endif