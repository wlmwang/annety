
//
//

#ifndef _ANT_SOCKETOPS_H_
#define _ANT_SOCKETOPS_H_

#include <arpa/inet.h>	// for sockaddr*
#include <sys/socket.h>	// for SHUT_WR

namespace annety {
namespace socket {
namespace tcp {

// 封装底层 socket 函数簇（有利于 mock）
int socket(sa_family_t family, bool nonblock = true, bool cloexec = true);
int accept(int sockfd, struct sockaddr_in6* addr, bool nonblock = true, bool cloexec = true);
int connect(int sockfd, const struct sockaddr* addr);
void bind(int sockfd, const struct sockaddr* addr);
void listen(int sockfd);
void close(int sockfd);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt);
void shutdown(int sockfd, int how = SHUT_WR);

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

struct sockaddr_in6 get_local_addr(int sockfd);
struct sockaddr_in6 get_peer_addr(int sockfd);

int get_sock_error(int sockfd);

bool is_self_connect(int sockfd);

void to_ip_port(char* buf, size_t size, const struct sockaddr* addr);
void to_ip(char* buf, size_t size, const struct sockaddr* addr);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in* addr);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in6* addr);

}	// namespace tcp

namespace unix {

}	// namespace unix

}	// namespace socket
}	// namespace annety

#endif	// _ANT_SOCKETOPS_H_