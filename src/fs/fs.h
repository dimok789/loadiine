#ifndef _FS_H_
#define _FS_H_

#include "../common/fs_defs.h"
#include "fs_functions.h"

extern void GX2WaitForVsync(void);

/* OS stuff */
extern void DCFlushRange(const void *p, unsigned int s);

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
