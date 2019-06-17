// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_SOCKETS_UTIL_H_
#define ANT_SOCKETS_UTIL_H_

#include <arpa/inet.h>	// for sockaddr*
#include <sys/socket.h>	// for SHUT_WR

namespace annety {
namespace sockets {
// static_cast
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

int socket(sa_family_t family, bool nonblock = true, bool cloexec = true);
int accept(int sockfd, struct sockaddr_in6* addr, bool nonblock = true, bool cloexec = true);
void bind(int sockfd, const struct sockaddr* addr);
void listen(int sockfd);
int connect(int sockfd, const struct sockaddr* addr);
void shutdown(int sockfd, int how = SHUT_WR);

struct sockaddr_in6 get_local_addr(int sockfd);
struct sockaddr_in6 get_peer_addr(int sockfd);

int get_sock_error(int sockfd);
bool is_self_connect(int sockfd);

void to_ip_port(char* buf, size_t size, const struct sockaddr* addr);
void to_ip(char* buf, size_t size, const struct sockaddr* addr);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in* addr);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in6* addr);

void close(int sockfd);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt);

}	// namespace sockets
}	// namespace annety

#endif	// ANT_SOCKETS_UTIL_H_
