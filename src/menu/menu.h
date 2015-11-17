#ifndef LAUNCHER_COREHANDLER_H_
#define LAUNCHER_COREHANDLER_H_

#include "../common/types.h"
#include "../common/fs_defs.h"
#include "../../libwiiu/src/vpad.h"

/* General defines */
#define MAX_GAME_COUNT      255
#define MAX_GAME_ON_PAGE    11

/* Main */
extern int (* const MiiMaker_main)(int argc, char *argv[]);
#define main (*MiiMaker_main)

/* System */
extern void _Exit (void);
extern void OSFatal(char* msg);
extern void DCFlushRange(const void *addr, uint length);

extern void GX2WaitForVsync(void);

/* Libs */
extern int OSDynLoad_Acquire(char* rpl, uint *handle);
extern int OSDynLoad_FindExport(uint handle, int isdata, char *symbol, void *address);

/* Screen */
extern void OSScreenInit(void);
extern uint OSScreenGetBufferSizeEx(uint bufferNum);
extern uint OSScreenSetBufferEx(uint bufferNum, void * addr);
extern uint OSScreenClearBufferEx(uint bufferNum, uint temp);
extern uint OSScreenFlipBuffersEx(uint bufferNum);
extern uint OSScreenPutFontEx(uint bufferNum, uint posX, uint posY, const void * buffer);

/* VPAD */
extern int VPADRead(int controller, VPADData *buffer, uint num, int *error);

/* FS Functions */
extern FSStatus FSInit(void);
extern FSStatus FSAddClient(FSClient *pClient, FSRetFlag errHandling);
extern void FSInitCmdBlock(FSCmdBlock *pCmd);
extern FSStatus FSGetMountSource(FSClient *pClient, FSCmdBlock *pCmd, FSSourceType type, FSMountSource *source, FSRetFlag errHandling);
extern FSStatus FSMount(FSClient *pClient, FSCmdBlock *pCmd, FSMountSource *source, char *target, uint32_t bytes, FSRetFlag errHandling);
extern FSStatus FSUnmount(FSClient *pClient, FSCmdBlock *pCmd, const char *target, FSRetFlag errHandling);
extern FSStatus FSOpenDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, int *dh, FSRetFlag errHandling);
extern FSStatus FSReadDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSDirEntry *dir_entry, FSRetFlag errHandling);
extern FSStatus FSOpenFile(FSClient *pClient, FSCmdBlock *pCmd, const char *path, const char *mode, int *fd, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, FSFlag flag, FSRetFlag errHandling);
extern FSStatus FSChangeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSMakeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSCloseDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSCloseFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, FSRetFlag errHandling);
extern FSStatus FSGetStat(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSStat *stats, FSRetFlag errHandling);

#endif
