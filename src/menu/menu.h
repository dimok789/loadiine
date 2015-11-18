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

#endif
