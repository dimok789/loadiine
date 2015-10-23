#ifndef _FS_H_
#define _FS_H_

#include "../common/fs_defs.h"

/* mem functions */
void *memcpy(void *dst, const void *src, int bytes);
void *memset(void *dst, int val, int bytes);
extern void *(* const MEMAllocFromDefaultHeapEx)(int size, int align);
extern void *(* const MEMAllocFromDefaultHeap)(int size);
extern void (* const MEMFreeToDefaultHeap)(void *p);
#define memalign (*MEMAllocFromDefaultHeapEx)

/* socket.h */
#define AF_INET         2
#define SOCK_STREAM     1
#define IPPROTO_TCP     6

extern void socket_lib_init();
extern int socket(int domain, int type, int protocol);
extern int socketclose(int socket);
extern int connect(int socket, void *addr, int addrlen);
extern int send(int socket, const void *buffer, int size, int flags);
extern int recv(int socket, void *buffer, int size, int flags);

extern void GX2WaitForVsync(void);


struct in_addr {
    unsigned int s_addr;
};
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

/* OS stuff */
extern const long long title_id;
extern void DCFlushRange(const void *p, unsigned int s);

/* SDCard functions */
extern FSStatus FSGetMountSource(void *pClient, void *pCmd, FSSourceType type, FSMountSource *source, FSRetFlag errHandling);
extern FSStatus FSMount(void *pClient, void *pCmd, FSMountSource *source, char *target, uint bytes, FSRetFlag errHandling);
extern FSStatus FSReadFile(FSClient *pClient, FSCmdBlock *pCmd, void *buffer, int size, int count, int fd, FSFlag flag, FSRetFlag errHandling);
extern void FSInitCmdBlock(FSCmdBlock *pCmd);
extern FSStatus FSCloseFile(FSClient *pClient, FSCmdBlock *pCmd, int fd, int error);

/* Forward declarations */
#define MAX_CLIENT 32

struct bss_t {
    int global_sock;
    int socket_fs[MAX_CLIENT];
    void *pClient_fs[MAX_CLIENT];
    volatile int lock;
    int sd_mount[MAX_CLIENT];
    char mount_base[255]; // ex : /vol/external01/nesr
    char save_base[255]; // ex : /vol/external01/_SAV/nesr
};

#define bss_ptr (*(struct bss_t **)0x100000e4)
#define bss (*bss_ptr)

int  fs_connect(int *socket);
void fs_disconnect(int socket);
int  fs_mount_sd(int sock, void* pClient, void* pCmd);
void log_string(int sock, const char* str, char byte);

/* Communication bytes with the server */
#define BYTE_NORMAL             0xff
#define BYTE_SPECIAL            0xfe
#define BYTE_OK                 0xfd
#define BYTE_PING               0xfc
#define BYTE_LOG_STR            0xfb

#define BYTE_OPEN_FILE          0x00
#define BYTE_OPEN_FILE_ASYNC    0x01
#define BYTE_OPEN_DIR           0x02
#define BYTE_OPEN_DIR_ASYNC     0x03
#define BYTE_CHANGE_DIR         0x04
#define BYTE_CHANGE_DIR_ASYNC   0x05
#define BYTE_STAT               0x06
#define BYTE_STAT_ASYNC         0x07

#define BYTE_CLOSE_FILE         0x08
#define BYTE_CLOSE_FILE_ASYNC   0x09
#define BYTE_SETPOS             0x0A
#define BYTE_GETPOS             0x0B
#define BYTE_STATFILE           0x0C
#define BYTE_EOF                0x0D
#define BYTE_READ_FILE          0x0E
#define BYTE_READ_FILE_ASYNC    0x0F
#define BYTE_CLOSE_DIR          0x10
#define BYTE_CLOSE_DIR_ASYNC    0x11
#define BYTE_GET_CWD            0x12
#define BYTE_READ_DIR           0x13
#define BYTE_READ_DIR_ASYNC     0x14

#define BYTE_MAKE_DIR           0x15
#define BYTE_MAKE_DIR_ASYNC     0x16
#define BYTE_RENAME             0x17
#define BYTE_RENAME_ASYNC       0x18
#define BYTE_REMOVE             0x19
#define BYTE_REMOVE_ASYNC       0x1A

#define BYTE_MOUNT_SD           0x30
#define BYTE_MOUNT_SD_OK        0x31
#define BYTE_MOUNT_SD_BAD       0x32

#endif /* _FS_H */
