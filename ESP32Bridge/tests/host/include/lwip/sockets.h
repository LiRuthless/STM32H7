#pragma once
/* Host-only lwIP socket boundary. Every call is fixture-driven; no real
 * network, scheduling or error evidence is produced here. */
#include "test_sdk.h"

typedef unsigned int socklen_t;
typedef unsigned int nfds_t;
typedef long ssize_t;

enum { AF_INET = 2, SOCK_STREAM = 1, SOCK_DGRAM = 2 };
enum { F_SETFL = 4, O_NONBLOCK = 1 };
#define EAGAIN 11
#define EWOULDBLOCK EAGAIN
#define EPIPE 32
extern int bridge_stub_errno;
#define errno bridge_stub_errno

#define POLLIN 0x01
#define POLLOUT 0x02
#define POLLERR 0x08
#define POLLHUP 0x10
#define POLLNVAL 0x20

struct in_addr { uint32_t s_addr; };
struct sockaddr { uint8_t sa_len; uint8_t sa_family; char sa_data[14]; };
struct sockaddr_in {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};
struct pollfd { int fd; short events; short revents; };

static inline uint16_t htons(uint16_t value)
{
    return (uint16_t)((uint16_t)(value << 8) | (uint16_t)(value >> 8));
}
static inline uint32_t htonl(uint32_t value)
{
    return ((value & 0xFFu) << 24) | ((value & 0xFF00u) << 8) |
           ((value >> 8) & 0xFF00u) | ((value >> 24) & 0xFFu);
}

int socket(int domain, int type, int protocol);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int s, void *mem, size_t len, int flags);
ssize_t send(int s, const void *data, size_t size, int flags);
ssize_t sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int close(int s);
int fcntl(int s, int cmd, ...);
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
