// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_SOCKETS_UTIL_H_
#define ANT_SOCKETS_UTIL_H_

#include <sys/socket.h>		// SHUT_WR, ...
#include <arpa/inet.h>		// sockaddr*, ...
#include <netinet/tcp.h>	// TCP_NODELAY,IPPROTO_TCP, 
							// tcp_info - linux 2.6.37

namespace annety
{
// when someone directly using annety namespace, it can also protect the same name 
// function in the system from being override.
namespace sockets {
// static convert between sockaddr and sockaddr_in/sockaddr_in6.
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

// Compatible with IPv4 and IPv6.
int socket(sa_family_t family, bool nonblock = true, bool cloexec = true);
int accept(int servfd, struct sockaddr_in6* dst, bool nonblock = true, bool cloexec = true);
int connect(int servfd, const struct sockaddr* addr);
int bind(int fd, const struct sockaddr* addr);
int listen(int servfd, int backlog = -1);
int shutdown(int fd, int how = SHUT_WR);

// Compatible with IPv4 and IPv6.
struct sockaddr_in6 get_local_addr(int fd);
struct sockaddr_in6 get_peer_addr(int fd);

// Compatible with IPv4 and IPv6.
void to_ip_port(const struct sockaddr* addr, char* dst, size_t size);
void to_ip(const struct sockaddr* addr, char* dst, size_t size);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in* dst);
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in6* dst);

int get_sock_error(int fd);
int set_reuse_addr(int servfd, bool on);
int set_reuse_port(int servfd, bool on);
int set_keep_alive(int servfd, bool on);

int set_tcp_nodelay(int fd, bool on);

#if defined(OS_LINUX)
int get_tcp_info(int fd, struct tcp_info* dst);
int get_tcp_info_string(int fd, char* dst, size_t size);
#endif	// OS_LINUX

// This can happen if the target server is local and has not been started.
bool is_self_connect(int fd);

int close(int sockfd);
ssize_t read(int sockfd, void *buf, size_t len);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t len);
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt);

bool set_non_blocking(int fd);
bool set_close_on_exec(int fd);

}	// namespace sockets
}	// namespace annety

#endif	// ANT_SOCKETS_UTIL_H_
