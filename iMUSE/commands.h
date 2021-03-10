#ifndef DIGITAL_IMUSE_CMDS
#define DIGITAL_IMUSE_CMDS

#include "imuse.h"
#include "triggers.h"
#include "wave.h"
#include "groups.h"

void iMUSEHeartbeat();

int handleCmds(int cmd, int arg_0, int arg_1, int arg_2, int arg_3, int arg_4,
						int arg_5, int arg_6, int arg_7, int arg_8, int arg_9,
						int arg_10, int arg_11, int arg_12, int arg_13);

int cmds_init(iMUSEInitData *initDataPtr);
int cmds_deinit();
int cmds_terminate();
int cmds_persistence(int cmd, void *funcCall);
int cmds_print(int param1, int param2, int param3, int param4, int param5, int param6, int param7);
int cmds_pause();
int cmds_resume();
int cmds_save(int *buffer, int bufferSize);
int cmds_restore(int *buffer);
int cmds_startSound(int soundId, int priority);
int cmds_stopSound(int soundId);
int cmds_stopAllSounds();
int cmds_getNextSound(int soundId);
int cmds_setParam(int soundId, int opcode, int value);
int cmds_getParam(int soundId, int opcode);
int cmds_setHook(int soundId, int hookId);
int cmds_getHook(int soundId);
int cmds_debug();

iMUSEInitData * cmd_initDataPtr;
int cmd_pauseCount;
int cmd_hostIntHandler;
int cmd_hostIntUsecCount;
int cmd_runningHostCount;
int cmd_running60HzCount;
int cmd_running10HzCount;

int (*iMUSE_FuncList[31])() = { cmds_init,             cmds_terminate,         cmds_print,             cmds_pause,            cmds_resume,
	 					        cmds_save,             cmds_restore,           groups_setGroupVol,     cmds_startSound,       cmds_stopSound,
	 						    cmds_stopAllSounds,    cmds_getNextSound,      cmds_setParam,          cmds_getParam,         fades_fadeParam,
								cmds_setHook,          cmds_getHook,           triggers_setTrigger,    triggers_checkTrigger, triggers_clearTrigger,
							    triggers_deferCommand, 0,                      0,                      0,                     0,
							    wave_startStream,      wave_switchStream,      wave_processStreams,    wave_queryStream,      wave_feedStream,
								wave_lipSync };
#endif