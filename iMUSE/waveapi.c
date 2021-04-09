#include "waveapi.h"

// Validated
int waveapi_moduleInit(int sampleRate, waveOutParams *waveoutParamStruct) {
	struct tagWAVEOUTCAPSA pwoc;

	waveapi_sampleRate = sampleRate;
	waveOutGetDevCapsA(-1, (LPWAVEOUTCAPSA)&pwoc, 52);
	if (pwoc.dwFormats & 1) {
		waveapi_bytesPerSample = 1;
	}
	if (pwoc.dwFormats & 2) {
		waveapi_bytesPerSample = 2;
	}
	if (waveapi_sampleRate > 40000 && pwoc.dwFormats & 1) {
		waveapi_bytesPerSample = 2;
	}
	waveapi_zeroLevel = 128;
	waveapi_numChannels = pwoc.wChannels;

	if (waveapi_bytesPerSample != 1) {
		waveapi_zeroLevel = 0;
	}
	waveapi_outBuf = malloc(waveapi_numChannels * waveapi_bytesPerSample * 9216);
	
	waveapi_waveFormat.nChannels = waveapi_numChannels;
	waveapi_waveFormat.wFormatTag = 1;
	waveapi_waveFormat.nSamplesPerSec = waveapi_sampleRate;
	waveapi_mixBuf = waveapi_outBuf + (waveapi_numChannels * waveapi_bytesPerSample * 8192);
	waveapi_waveFormat.nAvgBytesPerSec = waveapi_bytesPerSample * waveapi_sampleRate * waveapi_numChannels;
	waveapi_waveFormat.nBlockAlign = waveapi_numChannels * waveapi_bytesPerSample;
	waveapi_waveFormat.wBitsPerSample = waveapi_bytesPerSample * 8;
	
	if (waveOutOpen((LPHWAVEOUT)&waveHandle, -1, (LPCWAVEFORMATEX)&waveapi_waveFormat.wFormatTag, 0, 0, 0)) {
		warning("iWIN init: waveOutOpen failed\n");
		return -1;
	}

	waveapi_waveOutParams.bytesPerSample = waveapi_bytesPerSample * 8;
	waveapi_waveOutParams.numChannels = waveapi_numChannels;
	waveapi_waveOutParams.offsetBeginMixBuf = (waveapi_bytesPerSample * waveapi_numChannels) * 1024;
	waveapi_waveOutParams.sizeSampleKB = 0;
	waveapi_waveOutParams.mixBuf = waveapi_mixBuf;
	
	// Init the buffer at volume zero
	memset32(waveapi_outBuf, waveapi_zeroLevel, (unsigned int) waveapi_numChannels * (unsigned int) waveapi_bytesPerSample * 9216);
	*waveHeaders = (LPWAVEHDR) malloc(32 * sizeof(LPWAVEHDR));
	for (int l = 0; l < NUM_HEADERS; l++) {
		waveHeaders[l]->lpData = waveapi_outBuf + (waveapi_numChannels * waveapi_bytesPerSample * l * 1024);
		waveHeaders[l]->dwBufferLength = waveapi_bytesPerSample * waveapi_numChannels * 1024;
		waveHeaders[l]->dwFlags = 0;
		waveHeaders[l]->dwLoops = 0;
		if (waveOutPrepareHeader(waveHandle, waveHeaders[l], 32)) {
			printf("iWIN init: waveOutPrepareHeader failed.\n");
			for (l = 0; l < 8; l++) {
				waveOutUnprepareHeader(waveHandle, waveHeaders[l], 32);
				waveHeaders[l] = NULL;
			}
			free(waveHeaders);
			return -1;
		}
	}	
	for (int l = 0; l < NUM_HEADERS; l++) {
		if (waveOutWrite(waveHandle, waveHeaders[l], 32)) {
			printf("iWIN init: waveOutWrite failed.\n");
			for (int r = 0; r < NUM_HEADERS; r++) {
				waveOutUnprepareHeader(waveHandle, waveHeaders[r], 32);
				waveHeaders[r] = NULL;
			}
			free(waveHeaders);
			waveapi_disableWrite = 0;
			return -1;
		}
	}

	waveapi_disableWrite = 0;
	return 0;
}

// Validated
// TODO: check comment
void waveapi_write(LPSTR *lpData, int *feedSize, int *sampleRate) {
	if (waveapi_disableWrite)
		return;
	if (waveHeaders == NULL)
		return;
	waveapi_xorTrigger ^= 1; // Only in The DIG
	if (!waveapi_xorTrigger) // Only in The DIG
		return;
	*feedSize = 0;
	LPWAVEHDR headerToUse = waveHeaders[waveapi_writeIndex];
	if ((headerToUse->dwFlags & 1) == (headerToUse->lpData))
		return;
	if (waveOutUnprepareHeader(waveHandle, headerToUse, 32))
		return;
	LPWAVEHDR *ptrToWriteIndex = *(&waveHeaders + waveapi_writeIndex);
	if (!ptrToWriteIndex || !*ptrToWriteIndex)
		return;

	ptrToWriteIndex = waveapi_outBuf + ((waveapi_numChannels * waveapi_bytesPerSample * waveapi_writeIndex) << 10);
	*lpData = waveHeaders[waveapi_writeIndex]->lpData; // Is it right?
	waveOutPrepareHeader(waveHeaders, headerToUse, 32);
	waveOutWrite(waveHeaders, headerToUse, 32);
	*sampleRate = waveapi_sampleRate;
	*feedSize = 1024;
	waveapi_writeIndex = (waveapi_writeIndex + 1) % 8;
}

// Validated
int waveapi_free() {
	waveapi_disableWrite = 1;
	for (int l = 0; l < NUM_HEADERS; l++) {
		waveOutUnprepareHeader(waveHandle, waveHeaders[l], 32);
		waveHeaders[l] = NULL;
	}
	free(waveHeaders);
	waveOutReset(waveHandle);
	waveOutClose(waveHandle);
	free(waveapi_outBuf);
	waveapi_outBuf = NULL;
	return 0;
}

// Validated
void waveapi_callback() {
	if (!wvSlicingHalted) {
		tracks_callback();
	}
}

// Validated
void waveapi_increaseSlice() {
	wvSlicingHalted++;
}

// Validated
int waveapi_decreaseSlice() {
	int result = wvSlicingHalted;
	if (wvSlicingHalted--)
		result = wvSlicingHalted - 1;
	return result;
}
	