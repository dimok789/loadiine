#include "../common/common.h"
#include "../fs/fs.h"
#include "logger.h"
#include "socket.h"

#define CHECK_ERROR(cond) if (cond) { goto error; }

int logger_connect(int *psock) {
    struct sockaddr_in addr;
    int sock, ret;

    // No ip means that we don't have any server running, so no logs
    if (SERVER_IP == 0) {
        *psock = -1;
        return 0;
    }

    socket_lib_init();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(sock == -1);

    addr.sin_family = AF_INET;
    addr.sin_port = 7332;
    addr.sin_addr.s_addr = SERVER_IP;

    ret = connect(sock, (void *)&addr, sizeof(addr));
    CHECK_ERROR(ret < 0);

    int enable = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));

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

void logger_disconnect(int sock) {
    CHECK_ERROR(sock == -1);

    char byte = BYTE_DISCONNECT;
    sendwait(sock, &byte, 1);

    socketclose(sock);
error:
    return;
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

void log_byte(int sock, char byte) {
    while (bss.lock) GX2WaitForVsync();
        bss.lock = 1;

    if(sock != -1) {
        sendwait(sock, &byte, 1);
    }
    bss.lock = 0;
}
