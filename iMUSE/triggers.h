#ifndef DIGITAL_IMUSE_TRIGGERS
#define DIGITAL_IMUSE_TRIGGERS

#define MAX_TRIGGERS 8
#define MAX_DEFERS 8

typedef struct
{
	int sound;
	char text[256];
	int opcode;
	int args_0_;
	int args_1_;
	int args_2_;
	int args_3_;
	int args_4_;
	int args_5_;
	int args_6_;
	int args_7_;
	int args_8_;
	int args_9_;
	int clearLater;
} iMUSETriggers;

typedef struct
{
	int counter;
	int opcode;
	int args_0_;
	int args_1_;
	int args_2_;
	int args_3_;
	int args_4_;
	int args_5_;
	int args_6_;
	int args_7_;
	int args_8_;
	int args_9_;
} iMUSEDefers;

iMUSETriggers trigs[MAX_TRIGGERS];
iMUSEDefers defers[MAX_DEFERS];

int  triggers_moduleInit(iMUSEInitData *initDataPtr);
int  triggers_clear();
int  triggers_save(int *buffer, int bufferSize);
int  triggers_restore(int *buffer);
int  triggers_setTrigger(int soundId, char *marker, int opcode);
int  triggers_checkTrigger(int soundId, char *marker, int opcode);
int  triggers_clearTrigger(int soundId, char *marker, int opcode);
void triggers_processTriggers(int soundId, char *marker);
int  triggers_deferCommand(int count, int opcode);
void triggers_loop();
int  triggers_countPendingSounds(int soundId);
int  triggers_moduleFree();
void triggers_copyTEXT(char *dst, char *marker);
int  triggers_compareTEXT(char *marker1, char *marker2);
int  triggers_getMarkerLength(char *marker);

int  triggers_defersOn;
int  triggers_midProcessing;
iMUSEInitData * triggers_initDataPtr;
char triggers_textBuffer[256];
char triggers_empty_marker = '\0';

#endif