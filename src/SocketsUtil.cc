// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SocketsUtil.h"
#include "ByteOrder.h"
#include "Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>		// snprintf
#include <sys/socket.h>	// SOMAXCONN,struct sockaddr,sockaddr_in[6]
#include <sys/uio.h>	// struct iovec,readv
#include <unistd.h>		// close

namespace annety {
namespace sockets {
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) {
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) {
	return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
	return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr) {
	return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}

int socket(sa_family_t family, bool nonblock, bool cloexec) {
	// non-block, close-on-exec
	int flags = 0;
	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}
	if (cloexec) {
		flags |= SOCK_CLOEXEC;
	}
	flags |= SOCK_STREAM;

	int sockfd = ::socket(family, flags, IPPROTO_TCP);
	PLOG_IF(ERROR, sockfd < 0) << "::socket failed";

	return sockfd;
}

int accept(int sockfd, struct sockaddr_in6* addr, bool nonblock, bool cloexec) {
	// non-block, close-on-exec
	int flags = 0;
	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}
	if (cloexec) {
		flags |= SOCK_CLOEXEC;
	}

	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
	int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, flags);
	PLOG_IF(ERROR, connfd < 0) << "::accept4 failed";

	if (connfd < 0) {
		int err = errno;
		switch (err) {
			case EAGAIN:
			case ECONNABORTED:
			case EINTR:
			case EPROTO: // ???
			case EPERM:
			case EMFILE: // per-process lmit of open file desctiptor ???
				// expected errors
				errno = err;
				break;

			case EBADF:
			case EFAULT:
			case EINVAL:
			case ENFILE:
			case ENOBUFS:
			case ENOMEM:
			case ENOTSOCK:
			case EOPNOTSUPP:
				// unexpected errors
				PLOG(FATAL) << "unexpected error of ::accept4";
				break;

			default:
				PLOG(FATAL) << "unknown error of ::accept4";
				break;
		}
	}
	return connfd;
}

void bind(int sockfd, const struct sockaddr* addr) {
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
	PCHECK(ret >= 0);
}

void listen(int sockfd) {
	int ret = ::listen(sockfd, SOMAXCONN);	// #define SOMAXCONN 128
	PCHECK(ret >= 0);
}

int connect(int sockfd, const struct sockaddr* addr) {
	int ret = ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
	PLOG_IF(ERROR, ret < 0) << "::connect failed";
	return ret;
}

void shutdown(int sockfd, int how) {
	int ret = ::shutdown(sockfd, how);
	PLOG_IF(ERROR, ret < 0) << "::shutdown failed";
}

struct sockaddr_in6 get_local_addr(int sockfd) {
	struct sockaddr_in6 localaddr;
	::memset(&localaddr, 0, sizeof localaddr);
	
	socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
	int ret = ::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen);
	PLOG_IF(ERROR, ret < 0) << "::getsockname failed";

	return localaddr;
}

struct sockaddr_in6 get_peer_addr(int sockfd) {
	struct sockaddr_in6 peeraddr;
	::memset(&peeraddr, 0, sizeof peeraddr);

	socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
	int ret = ::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen);
	PLOG_IF(ERROR, ret < 0) << "::getpeername failed";

	return peeraddr;
}

int get_socket_error(int sockfd) {
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof optval);
	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
		return errno;
	}
	return optval;
}

bool is_self_connect(int sockfd) {
	struct sockaddr_in6 localaddr = get_local_addr(sockfd);
	struct sockaddr_in6 peeraddr = get_peer_addr(sockfd);
	
	if (localaddr.sin6_family == AF_INET) {
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port 
				&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	} else if (localaddr.sin6_family == AF_INET6) {
		return localaddr.sin6_port == peeraddr.sin6_port
				&& ::memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, 
							sizeof localaddr.sin6_addr) == 0;
	}
	return false;
}

void to_ip_port(char* buf, size_t size, const struct sockaddr* addr) {
	to_ip(buf, size, addr);

	const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
	uint16_t port = net_to_host16(addr4->sin_port);
	
	size_t end = ::strlen(buf);	// c-style string
	DCHECK(size > end);

	::snprintf(buf+end, size-end, ":%u", port);
}

void to_ip(char* buf, size_t size, const struct sockaddr* addr) {
	if (addr->sa_family == AF_INET) {
		DCHECK(size >= INET_ADDRSTRLEN);

		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
	} else if (addr->sa_family == AF_INET6) {
		DCHECK(size >= INET6_ADDRSTRLEN);

		const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
		::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in* addr) {
	addr->sin_family = AF_INET;
	addr->sin_port = host_to_net16(port);

	int ret = ::inet_pton(AF_INET, ip, &addr->sin_addr);
	PLOG_IF(ERROR, ret < 0) << "::inet_pton failed";
}

// 将 "ip6 + port" 地址转换 struct sockaddr_in6
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
	addr->sin6_family = AF_INET6;
	addr->sin6_port = host_to_net16(port);

	int ret = ::inet_pton(AF_INET6, ip, &addr->sin6_addr);
	PLOG_IF(ERROR, ret < 0) << "::inet_pton failed";
}

void close(int sockfd) {
	int ret = ::close(sockfd);
	PLOG_IF(ERROR, ret < 0) << "::close failed";
}

ssize_t read(int sockfd, void *buf, size_t count) {
	ssize_t ret = ::read(sockfd, buf, count);
	PLOG_IF(VERBOSE, ret < 0) << "::read failed";
	return ret;
}
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt) {
	ssize_t ret = ::readv(sockfd, iov, iovcnt);
	PLOG_IF(VERBOSE, ret < 0) << "::readv failed";
	return ret;
}

ssize_t write(int sockfd, const void *buf, size_t count) {
	int ret = ::write(sockfd, buf, count);
	PLOG_IF(VERBOSE, ret < 0) << "::write failed";
	return ret;
}
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt) {
	ssize_t ret = ::writev(sockfd, iov, iovcnt);
	PLOG_IF(VERBOSE, ret < 0) << "::writev failed";
	return ret;
}

}	// namespace sockets
}	// namespace annety
