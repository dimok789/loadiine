#ifndef __SOCKET_H_
#define __SOCKET_H_

/* socket.h */
#define AF_INET         2
#define SOCK_STREAM     1
#define IPPROTO_TCP     6
#define TCP_NODELAY     0x2004

extern void socket_lib_init();
extern int socket(int domain, int type, int protocol);
extern int socketclose(int socket);
extern int connect(int socket, void *addr, int addrlen);
extern int send(int socket, const void *buffer, int size, int flags);
extern int recv(int socket, void *buffer, int size, int flags);
extern int setsockopt(int fd, int level, int optname, void *optval, int optlen);

struct in_addr {
    unsigned int s_addr;
};
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

int recvwait(int sock, void *buffer, int len);
int recvbyte(int sock);
int sendwait(int sock, const void *buffer, int len);


#endif
