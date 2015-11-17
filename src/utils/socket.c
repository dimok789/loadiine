#include "socket.h"

int recvwait(int sock, void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = recv(sock, buffer, len, 0);
        if(ret < 0)
            return ret;

        len -= ret;
        buffer += ret;
    }
    return len;
}

int recvbyte(int sock) {
    unsigned char buffer[1];
    int ret;

    ret = recvwait(sock, buffer, 1);
    if (ret < 0)
        return ret;

    return buffer[0];
}

int sendwait(int sock, const void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = send(sock, buffer, len, 0);
        if(ret < 0)
            return ret;

        len -= ret;
        buffer += ret;
    }
    return len;
}
