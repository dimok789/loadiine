#include "fs.h"

static int recvwait(int sock, void *buffer, int len);
static int recvbyte(int sock);
static int sendwait(int sock, const void *buffer, int len);

#define CHECK_ERROR(cond) if (cond) { goto error; }

int fs_connect(int *psock) {
    extern unsigned int server_ip;
    struct sockaddr_in addr;
    int sock, ret;

    // No ip means that we don't have any server running, so no logs
    if (server_ip == 0) {
        *psock = -1;
        return 0;
    }

    socket_lib_init();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(sock == -1);

    addr.sin_family = AF_INET;
    addr.sin_port = 7332;
    addr.sin_addr.s_addr = server_ip;

    ret = connect(sock, (void *)&addr, sizeof(addr));
    CHECK_ERROR(ret < 0);

    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);

    *psock = sock;
    return 0;

error:
    if (sock != -1)
        socketclose(sock);

    *psock = -1;
    return -1;
}

void fs_disconnect(int sock) {
    CHECK_ERROR(sock == -1);
    socketclose(sock);
error:
    return;
}

int fs_mount_sd(int sock, void* pClient, void* pCmd) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    int is_mounted = 0;
    char buffer[1];

    if (sock != -1) {
        buffer[0] = BYTE_MOUNT_SD;
        sendwait(sock, buffer, 1);
    }

    // mount sdcard
    FSMountSource mountSrc;
    char mountPath[FS_MAX_MOUNTPATH_SIZE];
    int status = FSGetMountSource(pClient, pCmd, FS_SOURCETYPE_EXTERNAL, &mountSrc, FS_RET_NO_ERROR);
    if (status == FS_STATUS_OK)
    {
        status = FSMount(pClient, pCmd, &mountSrc, mountPath, sizeof(mountPath), FS_RET_UNSUPPORTED_CMD);
        if (status == FS_STATUS_OK)
        {
            // set as mounted
            is_mounted = 1;
        }
    }

    if (sock != -1) {
        buffer[0] = is_mounted ? BYTE_MOUNT_SD_OK : BYTE_MOUNT_SD_BAD;
        sendwait(sock, buffer, 1);
    }

    bss.lock = 0;
    return is_mounted;
}

void log_string(int sock, const char* str, char flag_byte) {
    if(sock == -1) {
		return;
	}
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    int i;
    int len_str = 0;
    while (str[len_str++]);

    //
    {
        char buffer[1 + 4 + len_str];
        buffer[0] = flag_byte;
        *(int *)(buffer + 1) = len_str;
        for (i = 0; i < len_str; i++)
            buffer[5 + i] = str[i];
		
        buffer[5 + i] = 0;
        
        sendwait(sock, buffer, 1 + 4 + len_str);
    }
    
    bss.lock = 0;
}

static int recvwait(int sock, void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = recv(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}

static int recvbyte(int sock) {
    unsigned char buffer[1];
    int ret;

    ret = recvwait(sock, buffer, 1);
    if (ret < 0) return ret;
    return buffer[0];
}

static int sendwait(int sock, const void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = send(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}
