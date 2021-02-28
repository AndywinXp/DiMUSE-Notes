#include "timer.h"
#include "imuse.h"
#include <stdio.h> 

// Validated
// Not including vh_initModule call
int timer_moduleInit(iMUSEInitData * initDataPtr) {
	if (initDataPtr->usecPerInt) {
		initDataPtr->timerCallback = timer_callback;
		timer_oldVec = NULL;
		timer_usecPerInt = initDataPtr->usecPerInt;
	} else {
		printf("TcStartInts: Who owns timer?");
	}
	timer_intFlag = 0;
	return 0;
}

// Validated
int timer_moduleDeinit() {
	return 0;
}

// Validated
int timer_getUsecPerInt() {
	return timer_usecPerInt;
}

// Validated
int timer_callback() {
	if (!timer_intFlag) {
		timer_intFlag = 1;
		iMUSEHeartbeat();
		timer_intFlag = 0;
	}
	return 0;
}

// Validated
int timer_moduleFree() {
	return 0;
}
