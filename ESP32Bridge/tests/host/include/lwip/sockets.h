/**
 * @file sockets.h
 * @brief 定义脚本驱动的socket接口及字节序转换，不联网。
 * @details 仅为tests/host自有夹具提供lwip/sockets.h所需的声明、固定常量或内联替身。
 *     实现由宿主夹具或本头内联提供，不链接真实SDK；不证明DMA、调度或硬件能力。
 */
#pragma once
/* 所有socket调用均由夹具脚本驱动，不产生真实网络或调度证据。 */
#include "test_sdk.h"

typedef unsigned int socklen_t;
typedef unsigned int nfds_t;
typedef long ssize_t;

enum
{
    AF_INET = 2,
    SOCK_STREAM = 1,
    SOCK_DGRAM = 2
};

enum
{
    F_SETFL = 4,
    O_NONBLOCK = 1
};

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

struct in_addr
{
    uint32_t s_addr; /**< IPv4网络字节序值。 */
};

struct sockaddr
{
    uint8_t sa_len; /**< 保留socket地址长度字段。 */
    uint8_t sa_family; /**< 保留地址族字段。 */
    char sa_data[14]; /**< 固定通用socket地址存储。 */
};

struct sockaddr_in
{
    uint8_t sin_len; /**< 保留IPv4 socket长度字段。 */
    uint8_t sin_family; /**< IPv4地址族，核对AF_INET。 */
    uint16_t sin_port; /**< UDP/TCP端口，网络字节序。 */
    struct in_addr sin_addr; /**< IPv4地址结构值。 */
    char sin_zero[8]; /**< 固定IPv4地址布局填充。 */
};

struct pollfd
{
    int fd; /**< 本轮虚拟文件描述符。 */
    short events; /**< 生产请求等待的poll事件。 */
    short revents; /**< 夹具本轮返回的poll事件。 */
};

/**
 * @brief 交换16位主机序与网络序字节。
 * @param[in] value 夹具小端宿主上的16位值。
 * @return 字节交换后的16位值。
 */
static inline uint16_t htons(uint16_t value)
{
    return (uint16_t)((uint16_t)(value << 8) | (uint16_t)(value >> 8));
}

/**
 * @brief 交换32位主机序与网络序字节。
 * @param[in] value 夹具小端宿主上的32位值。
 * @return 字节交换后的32位值。
 */
static inline uint32_t htonl(uint32_t value)
{
    return ((value & 0xFFu) << 24) | ((value & 0xFF00u) << 8) | ((value >> 8) & 0xFF00u)
        | ((value >> 24) & 0xFFu);
}

int socket(int domain, int type, int protocol);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int s, void *mem, size_t len, int flags);
ssize_t send(int s, const void *data, size_t size, int flags);
ssize_t sendto(
    int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int close(int s);
int fcntl(int s, int cmd, ...);
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
