#ifndef DIGITAL_IMUSE_GROUPS
#define DIGITAL_IMUSE_GROUPS

#define MAX_GROUPS 16
#define IMUSE_GROUP_SFX 1
#define IMUSE_GROUP_SPEECH 2
#define IMUSE_GROUP_MUSIC 3
#define IMUSE_GROUP_MUSICEFF 4

int groupEffVols[16];
int groupVols[16];

int groups_moduleInit();
int groups_moduleDeinit();
int groups_setGroupVol(int id, int volume);
int groups_getGroupVol(int id);
int groups_moduleDebug();

#endif