// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef _ANT_END_POINT_H_
#define _ANT_END_POINT_H_

#include "BuildConfig.h"
#include "StringPiece.h"

#include <string>
#include <stdint.h>
#include <stddef.h>		// offsetof
#include <netinet/in.h>	// in_addr

namespace annety {
// wrapper of sockaddr_in[6]
class EndPoint {
#if defined(OS_MACOSX)
	static_assert(offsetof(sockaddr_in, sin_family) == 1, "sin_family offset 0");
	static_assert(offsetof(sockaddr_in6, sin6_family) == 1, "sin6_family offset 0");
	static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
	static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");
#else
	static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
	static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
	static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
	static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");
#endif	// defined(OS_MACOSX)

public:
	// Constructs an endpoint with given port number.
	// Mostly used in TcpServer listening.
	explicit EndPoint(uint16_t port, bool loopbackOnly = false, bool ipv6 = false);

	// Constructs an endpoint with given ip and port.
	// |ip| "0.0.0.0"
	EndPoint(const StringPiece& ip, uint16_t port, bool ipv6 = false);

	// Constructs an endpoint with given struct @c sockaddr_in
	// Mostly used when accepting new connections
	explicit EndPoint(const struct sockaddr_in& addr) : addr_(addr) {}
	explicit EndPoint(const struct sockaddr_in6& addr) : addr6_(addr) {}
	
	// default copy/assignment is ok
	EndPoint(const EndPoint&) = default;
	EndPoint& operator=(const EndPoint&) = default;
	~EndPoint() = default;
	
	void set_sock_addr_inet(const struct sockaddr_in& addr) {
		addr_ = addr;
	}
	void set_sock_addr_inet6(const struct sockaddr_in6& addr6) {
		addr6_ = addr6;
	}
	const struct sockaddr* get_sock_addr() const;

	sa_family_t family() const {
		return addr_.sin_family;
	}
	uint16_t port_net_endian() const {
		return addr_.sin_port;
	}
	uint32_t ip_net_endian() const;

	std::string to_ip() const;
	std::string to_ip_port() const;
	uint16_t to_port() const;

#if !defined(OS_MACOSX)
	// resolve hostname to IP address, not changing port or sin_family
	// return true on success.
	// thread safe
	static bool resolve(const StringPiece& hostname, EndPoint* result);
#endif	// !defined(OS_MACOSX)

private:
	// must sin[6]_family offset is 0
	union {
		struct sockaddr_in addr_;
		struct sockaddr_in6 addr6_;
	};
};

}	// namespace annety

#endif	// _ANT_END_POINT_H_
