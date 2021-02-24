#ifndef DIGITAL_IMUSE_TIMER
#define DIGITAL_IMUSE_TIMER

#include "imuse.h"

int timer_usecPerInt;
int timer_oldVec;
int timer_intFlag;

int timer_moduleInit(iMUSEInitData *initDataPtr);
int timer_moduleDeinit();
int timer_getUsecPerInt();
int timer_callback();
int timer_moduleFree();

#endif