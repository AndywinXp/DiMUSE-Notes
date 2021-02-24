#ifndef DIGITAL_IMUSE_FADES
#define DIGITAL_IMUSE_FADES

#define MAX_FADES 16

typedef struct
{
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
} iMUSEFades;

iMUSEFades fades[MAX_FADES];
int fadesOn;

int fades_moduleInit();
int fades_moduleDeinit();
int fades_save(unsigned char *buffer, int sizeLeft);
int fades_restore(unsigned char *buffer);
int fades_fadeParam(int soundId, int param, int value1, int value2);
void fades_clearFadeStatus(int soundId, int value);
void fades_loop();
void fades_moduleFree();
int fades_moduleDebug();

#endif