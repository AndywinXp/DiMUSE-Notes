#ifndef DIGITAL_IMUSE_FADES
#define DIGITAL_IMUSE_FADES

#include "imuse.h"

#define MAX_FADES 16

typedef struct {
	int status;
	int sound;
	int param;
	int currentVal;
	int counter;
	int length;
	int slope;
	int slopeMod;
	int modOvfloCounter;
	int nudge;
} iMUSEFade;

iMUSEFade fades[MAX_FADES];
int fadesOn;

int fades_moduleInit();
int fades_moduleDeinit();
int fades_save(unsigned char *buffer, int sizeLeft);
int fades_restore(unsigned char *buffer);
int fades_fadeParam(int soundId, int opcode, int destinationValue, int fadeLength);
void fades_clearFadeStatus(int soundId, int opcode);
void fades_loop();
void fades_moduleFree();
int fades_moduleDebug();

#endif