#ifndef __FS_FUNCTIONS_H_
#define __FS_FUNCTIONS_H_

#include "../common/fs_defs.h"

/* FS Functions */
extern FSStatus FSInit(void);
extern FSStatus FSAddClient(FSClient *pClient, FSRetFlag errHandling);
extern void FSInitCmdBlock(FSCmdBlock *pCmd);
extern FSStatus FSGetMountSource(FSClient *pClient, FSCmdBlock *pCmd, FSSourceType type, FSMountSource *source, FSRetFlag errHandling);
extern FSStatus FSMount(FSClient *pClient, FSCmdBlock *pCmd, FSMountSource *source, const char *target, uint32_t bytes, FSRetFlag errHandling);
extern FSStatus FSUnmount(FSClient *pClient, FSCmdBlock *pCmd, const char *target, FSRetFlag errHandling);
extern FSStatus FSOpenDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, int *dh, FSRetFlag errHandling);
extern FSStatus FSReadDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSDirEntry *dir_entry, FSRetFlag errHandling);
extern FSStatus FSRewindDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSOpenFile(FSClient *pClient, FSCmdBlock *pCmd, const char *path, const char *mode, int *fd, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, FSFlag flag, FSRetFlag errHandling);
extern FSStatus FSChangeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSMakeDir(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSRetFlag errHandling);
extern FSStatus FSCloseDir(FSClient *pClient, FSCmdBlock *pCmd, int dh, FSRetFlag errHandling);
extern FSStatus FSCloseFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, FSRetFlag errHandling);
extern FSStatus FSGetStat(FSClient *pClient, FSCmdBlock *pCmd, const char *path, FSStat *stats, FSRetFlag errHandling);


#endif // __FS_FUNCTIONS_H_
