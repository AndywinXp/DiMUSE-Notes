#include "groups.h"

// Validated
// It's missing the call to VhInitModule, but we don't need it
int groups_moduleInit() {
	memset32(groupEffVols, 127, MAX_GROUPS);
	memset32(groupVols, 127, MAX_GROUPS);
	return 0;
}

// Validated
int groups_moduleDeinit() {
	return 0;
}

// Validated
int groups_setGroupVol(int id, int volume) {
	int l;

	if (id >= MAX_GROUPS) {
		return -5;
	}
	if (volume == -1) {
		return groupVols[id];
	}
	if (volume > 127)
		return -5;

	if (id) {
		groupVols[id] = volume;
		groupEffVols[id] = (groupVols[0] * (volume + 1)) / 128;
	} else {
		groupEffVols[0] = volume;
		groupVols[0] = volume;

		for (l = 1; l < MAX_GROUPS; l++) {
			groupEffVols[l] = (volume * (groupVols[id] + 1)) / 128;
		}
	}

	wave_setGroupVol();
	return groupVols[id];
}

// Validated
int groups_getGroupVol(int id) {
	if (id >= MAX_GROUPS) {
		return -5;
	}
	return groupEffVols[id];
}

// Validated
// Not sure this is what the original debug function does, 
// but it's good enough for our purpose
int groups_moduleDebug()
{
	printf("iMUSE:source:groups.c:groupVols[]: \n");
	for (int i = 0; i < MAX_GROUPS; i++)
	{
		printf("\t%d: %d\n", i, groupVols[i]);
	}
	printf("\n");

	printf("iMUSE:source:groups.c:groupEffVols[]");
	for (int i = 0; i < MAX_GROUPS; i++)
	{
		printf("\t%d: %d\n", i, groupEffVols[i]);
	}
	printf("\n");

	return 0;
}