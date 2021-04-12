#ifndef DIGITAL_IMUSE
#define DIGITAL_IMUSE

#include "commands.h"
#include "dispatch.h"
#include "fades.h"
#include "files.h"
#include "groups.h"
#include "imuseutils.h"
#include "mixer.h"
#include "streamer.h"
#include "timer.h"
#include "tracks.h"
#include "triggers.h"
#include "wave.h"
#include "waveapi.h"
#include "host_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <string.h>

// Remove this junk when porting to ScummVM

typedef          char   int8;
typedef unsigned char   uint8;
typedef          short  int16;
typedef unsigned short  uint16;
typedef          int    bool;

typedef struct {
	int (*getSoundDataAddr)();
	/*
	int __cdecl getSoundDataAddr(int a1)
	{
		return *(_DWORD *)(dword_4ACC16 + 4 * a1) + 16;
	}
	whatever that is
	*/
	int (*func_some1)();
	int (*scriptCallback)();
	int field_C; // Set to 0
	int hostIntHandler;
	int hostIntUsecCount;
	int (*timerCallback)();
	int usecPerInt; // Fixed at 20000
	int (*SMUSH_related_function)(); // This seems to be populated and used during smush videos
	int running12HzCount; // This appears to be the SMUSH framerate to microseconds as its calculated as 1000000 / 12
	int num60hzIterations;
	int (*handleCmdsFunc)(); // cmds_handleCmds
	int musicBundleLoaded; // Set to 0
	int (*waveCall)(); // IMUSE_nullsub_minus1 (returns -1) (might change during execution)
	int field_38;
	int unused;
	int halfSampleRateFlag; // Formerly field_40; fixed at 0 (at least for The Dig)
	int waveMixCount; // Fixed at 6
	int (*field_48)(); // Function related to SOU engine
	int (*seekFunc)();
	int (*readFunc)();
	iMUSESoundBuffer *(*bufInfoFunc)(); // ihost_getSoundBuffer
	int hWnd; // Window handle
	int codec_outputTempBuffer; // Used by BUN module
	int codec_saveBuffer; // Used by BUN module
	int compmgr_fileList; // Used by BUN module
	int field_68; // Apparently unused
} iMUSEInitData;

iMUSEInitData initDataPtr;

char *iMUSE_audioBuffer;
int iMUSE_feedSize; // Always 1024 apparently
int iMUSE_sampleRate;

#endif