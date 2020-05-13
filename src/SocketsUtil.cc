// By: wlmwang
// Date: Jun 17 2019

#include "SocketsUtil.h"
#include "build/BuildConfig.h"
#include "ByteOrder.h"
#include "Logging.h"
#include "EintrWrapper.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>		// close
#include <stdio.h>		// snprintf
#include <sys/socket.h>	// SOMAXCONN,struct sockaddr,sockaddr_in[6]
						// getsockopt,setsockopt
#include <sys/uio.h>	// struct iovec,readv,writev

namespace annety
{
namespace sockets {
// sin_family offset may not be 0, such as on MACOSX equal 1.
static_assert(offsetof(sockaddr_in, sin_family) == offsetof(sockaddr, sa_family), 
	"sin_family offset illegal");
static_assert(offsetof(sockaddr_in6, sin6_family) == offsetof(sockaddr, sa_family), 
	"sin6_family offset illegal");

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) 
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) 
{
	return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) 
{
	return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) 
{
	return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr) 
{
	return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}

// non-block, close-on-exec
int socket(sa_family_t family, bool nonblock, bool cloexec) 
{
	int flags = SOCK_STREAM;
#if defined(OS_MACOSX)
	int fd = ::socket(family, flags, IPPROTO_TCP);
	if (nonblock) {
		DCHECK(set_non_blocking(fd));
	}
	if (cloexec) {
		DCHECK(set_close_on_exec(fd));
	}
#elif defined(OS_POSIX)
	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}
	if (cloexec) {
		flags |= SOCK_CLOEXEC;
	}
	int fd = ::socket(family, flags, IPPROTO_TCP);
#else
#error Do not support your os platform in SocketsUtil.h
#endif

	PLOG_IF(ERROR, fd < 0) << "::socket failed";
	return fd;
}

// non-block, close-on-exec
int accept(int servfd, struct sockaddr_in6* dst, bool nonblock, bool cloexec)
{
	CHECK(servfd >= 0 && dst);

	// Always assume that the actual type of the `dst` is sockaddr_in6.
	socklen_t addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
#if defined(OS_MACOSX)
	// The kernel will fill in the value of `addrlen` and `dst`.
	int connfd = ::accept(servfd, sockaddr_cast(dst), &addrlen);
	if (nonblock) {
		DCHECK(set_non_blocking(servfd));
	}
	if (cloexec) {
		DCHECK(set_close_on_exec(servfd));
	}
#elif defined(OS_POSIX)
	int flags = 0;
	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}
	if (cloexec) {
		flags |= SOCK_CLOEXEC;
	}
	// The kernel will fill in the value of `addrlen` and `dst`.
	int connfd = ::accept4(servfd, sockaddr_cast(dst), &addrlen, flags);
#else
#error Do not support your os platform in SocketsUtil.h
#endif	// defined(OS_LINUX) || defined(OS_BSD) || ...

	PLOG_IF(ERROR, connfd < 0) << "::accept failed";

	if (connfd < 0) {
		// errno may be a macro.
		int err = errno;
		switch (err) {
			case EPERM:			// 1:	Operation not permitted.
			case EINTR:			// 4:	Interrupted system call.
			case EAGAIN:		// 11:	Resource temporarily unavailable.
			case EMFILE:		// 24:	Too many open files.
								//		- process limit of open file desctiptor ???
			case EPROTO:		// 71:	Protocol error.
								// 		- the client terminates the connection. SVR4 implementation.
			case ECONNABORTED:	// 103:	Software caused connection abort
								//		- the client terminates the connection. POSIX implementation.
				// expected errors.
				errno = err;
				break;

			case ENOMEM:		// 12:	Out of memory.
			case EFAULT:		// 14:	Bad address.
			case EINVAL:		// 22:	Invalid argument.
			case ENFILE:		// 23:	File table overflow.
			case EBADF:			// 77:	File descriptor in bad state.
			case ENOTSOCK:		// 88:	Socket operation on non-socket.
			case EOPNOTSUPP:	// 95:	Operation not supported on transport endpoint.
			case ENOBUFS:		// 105:	No buffer space available.
				// unexpected errors.
				PLOG(FATAL) << "unexpected error of ::accept4";
				break;

			default:
				PLOG(FATAL) << "unknown error of ::accept4";
				break;
		}
	}
	
	return connfd;
}

int bind(int fd, const struct sockaddr* addr)
{
	CHECK(fd >= 0 && addr);

	// Always assume that the actual type of the `addr` is sockaddr_in6.
	socklen_t addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in6));

#if defined(OS_MACOSX)
	// On MacOSX platform, `addrlen` should be the actual size of the address.
	if (addr->sa_family == AF_INET) {
		addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in));
	}
#endif	// defined(OS_MACOSX)

	// The kernel will judge according to `addr->sa_family`.
	int ret = ::bind(fd, addr, addrlen);
	PCHECK(!ret);

	return ret;
}

int connect(int servfd, const struct sockaddr* addr)
{
	CHECK(servfd >= 0 && addr);

	// Always assume that the actual type of the `addr` is sockaddr_in6.
	socklen_t addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in6));

#if defined(OS_MACOSX)
	// On MacOSX platform, `addrlen` should be the actual size of the address.
	if (addr->sa_family == AF_INET) {
		addrlen = static_cast<socklen_t>(sizeof(struct sockaddr_in));
	}
#endif	// defined(OS_MACOSX)

	// The kernel will judge according to `addr->sa_family`.
	int ret = ::connect(servfd, addr, addrlen);
	PLOG_IF(ERROR, ret < 0 && errno != EINPROGRESS) << "::connect failed";
	
	return ret;
}

int listen(int servfd, int backlog)
{
	CHECK(servfd >= 0);

	// If a connection request arrives when the queue of pending connections is full, 
	// the client may receive an error with an indication of ECONNREFUSED or, if the 
	// underlying protocol supports retransmission, the request may be ignored so that 
	// a later reattempt at connection succeeds.

	// The behavior of the backlog argument on TCP sockets changed with Linux 2.2.
	// Now it specifies the queue length for completely established sockets waiting
	// to be accepted, instead of the number of incomplete connection requests.
	//
	// If the backlog argument is greater than the value in /proc/sys/net/core/somaxconn, 
	// then it is silently truncated to that value; the default value in this file is 128.
	// In kernels before 2.4.25, this limit was a hard coded value, SOMAXCONN, with the 
	// value 128.

	// OS_BSD: If the backlog greater than kern.ipc.soacceptqueue or less than zero is 
	// specified, backlog is silently forced to kern.ipc.soacceptqueue.

	int ret = ::listen(servfd, backlog);
	PCHECK(!ret);
	
	return ret;
}

int shutdown(int fd, int how)
{
	CHECK(fd >= 0);

	int ret = ::shutdown(fd, how);
	PCHECK(!ret);

	return ret;
}

struct sockaddr_in6 get_local_addr(int fd)
{
	struct sockaddr_in6 addr;
	socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
	::memset(&addr, 0, sizeof addr);

	// The kernel will fill in the value of `addrlen` and `addr`.
	int ret = ::getsockname(fd, sockaddr_cast(&addr), &addrlen);
	PCHECK(!ret);

	return addr;
}

struct sockaddr_in6 get_peer_addr(int fd)
{
	struct sockaddr_in6 addr;
	::memset(&addr, 0, sizeof addr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof addr);

	// The kernel will fill in the value of `addrlen` and `addr`.
	int ret = ::getpeername(fd, sockaddr_cast(&addr), &addrlen);
	PCHECK(!ret);

	return addr;
}

int get_sock_error(int fd)
{
	int err;
	
	// If an error occurs, the return value of getsockopt is not portable:
	// 1. OS_BSD: The getsockopt will returns 0, and the error is written 
	// 	  to the variable `err`.
	// 2. OS_SOLARIS: The getsockopt will returns -1, and the error is 
	// 	  written to the global `errno`.
	socklen_t optlen = static_cast<socklen_t>(sizeof err);
	if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
		return errno;
	}
	
	return err;
}

int set_reuse_addr(int servfd, bool on)
{
	int opt = on ? 1 : 0;
	socklen_t optlen = static_cast<socklen_t>(sizeof opt);
	int ret = ::setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, optlen);

	PLOG_IF(ERROR, ret < 0) << "::setsockopt SO_REUSEADDR failed";

	return ret;
}

int set_reuse_port(int servfd, bool on)
{
#ifdef SO_REUSEPORT
	int opt = on ? 1 : 0;
	socklen_t optlen = static_cast<socklen_t>(sizeof opt);
	int ret = ::setsockopt(servfd, SOL_SOCKET, SO_REUSEPORT, &opt, optlen);

	PLOG_IF(ERROR, ret < 0) << "::setsockopt SO_REUSEPORT failed";
	return ret;
#else
	if (on) {
		LOG(ERROR) << "SO_REUSEPORT is not supported.";
	}
	return -1;
#endif
}

int set_keep_alive(int fd, bool on)
{
	int opt = on ? 1 : 0;
	socklen_t optlen = static_cast<socklen_t>(sizeof opt);
	int ret = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, optlen);

	PLOG_IF(ERROR, ret < 0) << "::setsockopt SO_KEEPALIVE failed";
	
	return ret;
}

int set_tcp_nodelay(int fd, bool on)
{
	int opt = on ? 1 : 0;
	socklen_t optlen = static_cast<socklen_t>(sizeof opt);
	int ret = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, optlen);
	
	PLOG_IF(ERROR, ret < 0) << "::setsockopt TCP_NODELAY failed";
	
	return ret;
}

#if defined(OS_LINUX)
int get_tcp_info(int fd, struct tcp_info* dst)
{
	CHECK(dst);

	::memset(dst, 0, sizeof(*dst));
	socklen_t optlen = static_cast<socklen_t>(sizeof(*dst));
	int ret = ::getsockopt(fd, SOL_TCP, TCP_INFO, dst, &optlen);
	
	PLOG_IF(ERROR, ret < 0) << "::getsockopt TCP_INFO failed";

	return ret;
}

int get_tcp_info_string(int fd, char* dst, size_t size)
{
	CHECK(dst);

	struct tcp_info tcpi;
	int ret = get_tcp_info(fd, &tcpi);
	
	if (ret) {
		::snprintf(dst, size, "unrecovered=%u "
		"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
		"lost=%u retrans=%u rtt=%u rttvar=%u "
		"sshthresh=%u cwnd=%u total_retrans=%u",
		tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
		tcpi.tcpi_rto,          // Retransmit timeout in usec
		tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
		tcpi.tcpi_snd_mss,
		tcpi.tcpi_rcv_mss,
		tcpi.tcpi_lost,         // Lost packets
		tcpi.tcpi_retrans,      // Retransmitted packets out
		tcpi.tcpi_rtt,          // Smoothed round trip time in usec
		tcpi.tcpi_rttvar,       // Medium deviation
		tcpi.tcpi_snd_ssthresh,
		tcpi.tcpi_snd_cwnd,
		tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
	}

	return ret;
}
#endif	// OS_LINUX

// This can happen if the target server is local and has not been started.
bool is_self_connect(int fd)
{
	struct sockaddr_in6 localaddr = get_local_addr(fd);
	struct sockaddr_in6 peeraddr = get_peer_addr(fd);
	
	if (localaddr.sin6_family == AF_INET) {
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port 
				&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	} else if (localaddr.sin6_family == AF_INET6) {
		return localaddr.sin6_port == peeraddr.sin6_port
				&& ::memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, 
							sizeof localaddr.sin6_addr) == 0;
	} else {
		NOTREACHED();
	}
	// For Compile warning.
	return false;
}

// Convert struct sockaddr into "IPv4/IPv6 + port" address
void to_ip_port(const struct sockaddr* addr, char* dst, size_t size)
{
	CHECK(addr && dst);

	to_ip(addr, dst, size);

	uint16_t port = 0;
	if (addr->sa_family == AF_INET) {
		// Recover actual type.
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);

		port = net_to_host16(addr4->sin_port);	// ::ntohs()
	} else if (addr->sa_family == AF_INET6) {
		const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);

		port = net_to_host16(addr6->sin6_port);	// ::ntohs()
	} else {
		NOTREACHED();
	}
	
	// c-style string
	size_t end = ::strlen(dst);
	CHECK(size > end);

	::snprintf(dst+end, size-end, ":%u", port);
}

// Convert struct sockaddr into "IPv4/IPv6" address.
void to_ip(const struct sockaddr* addr, char* dst, size_t size)
{
	CHECK(addr && dst);

	if (addr->sa_family == AF_INET) {
		CHECK(size >= INET_ADDRSTRLEN);

		// Recover actual type.
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		
		// Convert numeric format to dotted decimal IPv4 address format.
		// inet_ntoa() is *Not thread safe*.
		const char *ptr = ::inet_ntop(AF_INET, &addr4->sin_addr, dst, static_cast<socklen_t>(size));
		PCHECK(ptr);
	} else if (addr->sa_family == AF_INET6) {
		CHECK(size >= INET6_ADDRSTRLEN);

		// Recover actual type.
		const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
		
		// Convert number(network byte-order) to IPv6 address.
		// inet_ntoa() is *Not thread safe*.
		const char *ptr = ::inet_ntop(AF_INET6, &addr6->sin6_addr, dst, static_cast<socklen_t>(size));
		PCHECK(ptr);
	} else {
		NOTREACHED();
	}
}

// Convert "IPv4 + port" address into struct sockaddr_in
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in* dst)
{
	CHECK(ip && dst);

	dst->sin_family = AF_INET;
	dst->sin_port = host_to_net16(port);	// ::htons()

	// Convert IP address to number(network byte-order).
	int ret = ::inet_pton(AF_INET, ip, &dst->sin_addr);
	CHECK(ret >= 0);
}

// Convert "IPv6 + port" address into struct sockaddr_in6
void from_ip_port(const char* ip, uint16_t port, struct sockaddr_in6* dst)
{
	CHECK(ip && dst);

	dst->sin6_family = AF_INET6;
	dst->sin6_port = host_to_net16(port);	// ::htons()

	// Convert IP address to number(network byte-order).
	int ret = ::inet_pton(AF_INET6, ip, &dst->sin6_addr);
	CHECK(ret >= 0);
}

int close(int fd)
{
	int ret = ::close(fd);
	PLOG_IF(ERROR, ret < 0) << "::close failed fd=" << fd;
	
	return ret;
}

ssize_t read(int sockfd, void *buf, size_t len)
{
	CHECK(buf);

	ssize_t ret = ::read(sockfd, buf, len);
	
	DPLOG_IF(ERROR, ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) 
		<< "::read failed";
	
	return ret;
}
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt)
{
	CHECK(iov);

	ssize_t ret = ::readv(sockfd, iov, iovcnt);
	
	DPLOG_IF(ERROR, ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) 
		<< "::readv failed";
	
	return ret;
}

ssize_t write(int sockfd, const void *buf, size_t len)
{
	CHECK(buf);

	int ret = ::write(sockfd, buf, len);
	
	DPLOG_IF(ERROR, ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) 
		<< "::write failed";
	
	return ret;
}
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt)
{
	CHECK(iov);

	ssize_t ret = ::writev(sockfd, iov, iovcnt);
	
	DPLOG_IF(ERROR, ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) 
		<< "::writev failed";
	
	return ret;
}

bool set_non_blocking(int fd) 
{
	const int flags = ::fcntl(fd, F_GETFL);
	if (flags == -1) {
		PLOG(ERROR) << "Unable to fcntl file F_GETFL " << fd;
		return false;
	}
	if (flags & O_NONBLOCK) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) == -1) {
		PLOG(ERROR) << "Unable to fcntl file O_NONBLOCK";
		return false;
	}
	return true;
}

bool set_close_on_exec(int fd) 
{
	const int flags = ::fcntl(fd, F_GETFD);
	if (flags == -1) {
		PLOG(ERROR) << "Unable to fcntl file F_GETFD " << fd;
		return false;
	}
	if (flags & FD_CLOEXEC) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFD, flags | FD_CLOEXEC)) == -1) {
		PLOG(ERROR) << "Unable to fcntl file FD_CLOEXEC " << fd;
		return false;
	}
	return true;
}

}	// namespace sockets
}	// namespace annety
