#include "groups.h"

int groups_moduleInit() {
	memset(groupEffVols, 127, MAX_GROUPS);
	memset(groupVols, 127, MAX_GROUPS);
	return 0;
}

int groups_moduleDeinit() {
	return 0;
}

int groups_setGroupVol(int id, int volume) {

	if (id >= MAX_GROUPS) {
		return -5;
	}
	if (volume == -1) {
		return groupVols[id];
	}
	if (volume > 127)
		return -5;

	int value = groupVols[id];
	int l;

	if (id == 0) {
		groupEffVols[0] = volume;
		groupVols[0] = volume;

		for (l = 1; l < 16; l++) {
			groupEffVols[l] = (groupVols[0] * (volume + 1)) / 128;
		}
	} else {
		value = groupVols[l];
		groupEffVols[id] = (volume * (value + 1)) / 128;
	}

	wavedrv_setGroupVol();
	return value;
}

int groups_getGroupVol(int id) {
	if (id >= 16) {
		return -5;
	}
	return groupEffVols[id];
}

int groups_moduleDebug()
{
	printf("iMUSE:source:groups.c:groupVols[]: \n");
	for (int i = 0; i < 16; i++)
	{
		printf("\t%d: %d\n", i, groupVols[i]);
	}
	printf("\n");

	printf("iMUSE:source:groups.c:groupEffVols[]");
	for (int i = 0; i < 16; i++)
	{
		printf("\t%d: %d\n", i, groupEffVols[i]);
	}
	printf("\n");

	return 0;
}