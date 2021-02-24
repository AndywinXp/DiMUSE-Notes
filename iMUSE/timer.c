#include "timer.h"
#include "imuse.h"
#include <stdio.h> 

int timer_moduleInit(iMUSEInitData * initDataPtr) {
	if (initDataPtr) {
		initDataPtr->timerCallback = timer_callback;
		timer_oldVec = NULL;
		timer_usecPerInt = initDataPtr->usecPerInt;
	} else {
		printf("TcStartInts: Who owns timer?");
	}
	timer_intFlag = 0;
	return 0;
}

int timer_moduleDeinit() {
	return 0;
}

int timer_getUsecPerInt() {
	return timer_usecPerInt;
}

int timer_callback() {
	if (!timer_intFlag) {
		timer_intFlag = 1;
		iMUSEHeartbeat();
		timer_intFlag = 0;
	}
	return 0;
}

int timer_moduleFree() {
	return 0;
}
