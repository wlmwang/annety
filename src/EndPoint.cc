// By: wlmwang
// Date: Jun 17 2019

#include "EndPoint.h"
#include "ByteOrder.h"
#include "SocketsUtil.h"
#include "Logging.h"

#include <netdb.h>	// getaddrinfo,freeaddrinfo,struct addrinfo
#include <string.h>	// memset

namespace annety
{
// \file <netinet/in.h>
// #define INADDR_ANY       ((in_addr_t) 0x00000000)
// #define INADDR_LOOPBACK  ((in_addr_t) 0x7f000001) /* Inet 127.0.0.1. */
// ---
// extern const struct in6_addr in6addr_any;      /* :: */
// extern const struct in6_addr in6addr_loopback; /* ::1 */

// /* Structure sockaddr */
// struct sockaddr {
// 		sa_family_t sa_family;  /* address family: AF_INET */
// 		char sa_data[14];       /* Address data.  */
// };

// /* Structure describing an Internet socket address.  */
// struct sockaddr_in {
// 		sa_family_t    sin_family; /* address family: AF_INET */
// 		uint16_t       sin_port;   /* port in network byte order */
// 		struct in_addr sin_addr;   /* internet address */
// };

// /* address family: AF_INET/AF_INET6 */
// typedef unsigned short int sa_family_t;
//
// /* Internet address. */
// typedef uint32_t in_addr_t;
// struct in_addr {
// 		in_addr_t       s_addr;     /* address in network byte order */
// };

// /* Structure Internet socket address of IPv6 */
// struct sockaddr_in6 {
// 		sa_family_t     sin6_family;   /* address family: AF_INET6 */
// 		uint16_t        sin6_port;     /* port in network byte order */
// 		uint32_t        sin6_flowinfo; /* IPv6 flow information */
// 		struct in6_addr sin6_addr;     /* IPv6 address */
// 		uint32_t        sin6_scope_id; /* IPv6 scope-id */
// };

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

static_assert(offsetof(sockaddr_in, sin_family) == offsetof(sockaddr_in6, sin6_family), 
	"sin[6]_family offset illegal");
static_assert(offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port), 
	"sin[6]_port offset illegal");
static_assert(sizeof(EndPoint) == sizeof(struct sockaddr_in6),
	"EndPoint is same size as sockaddr_in6");

EndPoint::EndPoint(uint16_t port, bool loopback_only, bool ipv6)
{
	static_assert(offsetof(EndPoint, addr_) == 0, "addr_ offset 0");
	static_assert(offsetof(EndPoint, addr6_) == 0, "addr6_ offset 0");

	if (ipv6) {
		::memset(&addr6_, 0, sizeof addr6_);
		addr6_.sin6_family = AF_INET6;
		in6_addr ip = loopback_only ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ip;
		addr6_.sin6_port = host_to_net16(port);		// ::htons()
	} else {
		::memset(&addr_, 0, sizeof addr_);
		addr_.sin_family = AF_INET;
		in_addr_t ip = loopback_only ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = host_to_net32(ip);	// ::htonl()
		addr_.sin_port = host_to_net16(port);		// ::htons()
	}
}

EndPoint::EndPoint(const StringPiece& ip, uint16_t port, bool ipv6)
{
	if (ipv6) {
		::memset(&addr6_, 0, sizeof addr6_);
		sockets::from_ip_port(ip.as_string().c_str(), port, &addr6_);
	} else {
		::memset(&addr_, 0, sizeof addr_);
		sockets::from_ip_port(ip.as_string().c_str(), port, &addr_);
	}
}

const struct sockaddr* EndPoint::get_sockaddr() const
{
	static_assert(offsetof(sockaddr_in, sin_family) == offsetof(sockaddr, sa_family), 
		"sin_family offset illegal");
	static_assert(offsetof(sockaddr_in6, sin6_family) == offsetof(sockaddr, sa_family), 
		"sin6_family offset illegal");

	return sockets::sockaddr_cast(&addr6_);
}

std::string EndPoint::to_ip_port() const
{
	char buffer[64] = "";
	sockets::to_ip_port(get_sockaddr(), buffer, sizeof buffer);
	return buffer;
}

std::string EndPoint::to_ip() const
{
	char buffer[64] = "";
	sockets::to_ip(get_sockaddr(), buffer, sizeof buffer);
	return buffer;
}

uint16_t EndPoint::to_port() const
{
	static_assert(offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port), 
		"sin[6]_port offset illegal");

	return net_to_host16(addr_.sin_port);	// ::ntohs()
}

sa_family_t EndPoint::family() const
{
	static_assert(offsetof(sockaddr_in, sin_family) == offsetof(sockaddr_in6, sin6_family), 
		"sin[6]_family offset illegal");

	return addr_.sin_family;
}

bool EndPoint::resolve(const StringPiece& node, const StringPiece& service, EndPoint* dst)
{
	struct addrinfo *res = nullptr;
	struct addrinfo hints;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// any address family. AF_INET/AF_INET6/...
	hints.ai_socktype = SOCK_STREAM;	// 0: any type. SOCK_STREAM/SOCK_DGRAM
	hints.ai_protocol = IPPROTO_IP;		// 0: any protocol. IPPROTO_IP = IPv4/IPv6

	// Returns one or more addrinfo structures, each of which contains an Internet 
	// address that can be specified in a call to bind(2) or connect(2).
	int rt = ::getaddrinfo(node.as_string().c_str(), 
							service.as_string().c_str(), 
							&hints, &res);

	PLOG_IF(ERROR, rt != 0) << "::getaddrinfo failed";

	// Only get the first one, `res->ai_next` is ignored.
	if (rt == 0 && res != nullptr) {
		if (res->ai_family == AF_INET) {
			::memcpy(&dst->addr_, res->ai_addr, res->ai_addrlen);
		} else if (res->ai_family == AF_INET6) {
			::memcpy(&dst->addr6_, res->ai_addr, res->ai_addrlen);
		} else {
			NOTREACHED();
		}
	}
	::freeaddrinfo(res);

	return rt == 0;
}

}	// namespace annety
