#ifndef DIGITAL_IMUSE
#define DIGITAL_IMUSE

typedef struct 
{
	int getSoundDataAddr;
	int func_some1;
	int (*scriptCallback)();
	int field_C;
	int hostIntHandler;
	int hostIntUsecCount;
	int (*timerCallback)();
	int usecPerInt;
	int field_20;
	int field_24;
	int num60hzIterations;
	int field_2C;
	int field_30;
	int someFuncPtr;
	int field_38;
	int field_3C;
	int field_40;
	int waveMixCount;
	int field_48;
	int (*seekFunc)();
	int(*readFunc)();
	int field_54;
	int field_58;
	int field_5C;
	int field_60;
	int field_64;
	int field_68;
} iMUSEInitData;

#endif