#ifndef _FS_H_
#define _FS_H_

#include "../common/fs_defs.h"

extern void GX2WaitForVsync(void);

/* OS stuff */
extern void DCFlushRange(const void *p, unsigned int s);

/* SDCard functions */
extern FSStatus FSGetMountSource(void *pClient, void *pCmd, FSSourceType type, FSMountSource *source, FSRetFlag errHandling);
extern FSStatus FSMount(void *pClient, void *pCmd, FSMountSource *source, char *target, uint bytes, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, FSFlag flag, FSRetFlag errHandling);
extern void FSInitCmdBlock(FSCmdBlock *pCmd);
extern FSStatus FSCloseFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, int error);

/* Async callback definition */
typedef void (*FSAsyncCallback)(void *pClient, void *pCmd, int result, void *context);
typedef struct
{
    FSAsyncCallback userCallback;
    void            *userContext;
    void            *ioMsgQueue;
} FSAsyncParams;

/* Forward declarations */
#define MAX_CLIENT 32

struct bss_t {
    int global_sock;
    int socket_fs[MAX_CLIENT];
    void *pClient_fs[MAX_CLIENT];
    volatile int lock;
    char mount_base[255];
    char save_base[255];
};

#define bss_ptr (*(struct bss_t **)0x100000e4)
#define bss (*bss_ptr)

void PatchFsMethods(void);

#endif /* _FS_H */
