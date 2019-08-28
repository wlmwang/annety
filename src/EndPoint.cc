// By: wlmwang
// Date: Jun 17 2019

#include "EndPoint.h"
#include "ByteOrder.h"
#include "SocketsUtil.h"
#include "Logging.h"

#include <netdb.h>	// struct hostent

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

static_assert(sizeof(EndPoint) == sizeof(struct sockaddr_in6),
			"EndPoint is same size as sockaddr_in6");

EndPoint::EndPoint(uint16_t port, bool loopback_only, bool ipv6)
{
	static_assert(offsetof(EndPoint, addr6_) == 0, "addr6_ offset 0");
	static_assert(offsetof(EndPoint, addr_) == 0, "addr_ offset 0");

	if (ipv6) {
		::memset(&addr6_, 0, sizeof addr6_);
		addr6_.sin6_family = AF_INET6;
		in6_addr ip = loopback_only ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ip;
		addr6_.sin6_port = host_to_net16(port);
	} else {
		::memset(&addr_, 0, sizeof addr_);
		addr_.sin_family = AF_INET;
		in_addr_t ip = loopback_only ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = host_to_net32(ip);
		addr_.sin_port = host_to_net16(port);
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
	return sockets::sockaddr_cast(&addr6_);
}

std::string EndPoint::to_ip_port() const
{
	char buf[64] = "";
	sockets::to_ip_port(buf, sizeof buf, get_sockaddr());
	return buf;
}

std::string EndPoint::to_ip() const
{
	char buf[64] = "";
	sockets::to_ip(buf, sizeof buf, get_sockaddr());
	return buf;
}

uint16_t EndPoint::to_port() const
{
	return net_to_host16(addr_.sin_port);
}

uint32_t EndPoint::ip_net_endian() const
{
	DCHECK(family() == AF_INET);
	return addr_.sin_addr.s_addr;
}

#if defined(OS_LINUX)
static thread_local char tls_resolve_buffer[64 * 1024];
bool EndPoint::resolve(const StringPiece& hostname, EndPoint* out)
{
	CHECK(out != nullptr);

	struct hostent hent;
	struct hostent* he = nullptr;
	int herrno = 0;
	::memset(&hent, 0, sizeof(hent));

	int ret = ::gethostbyname_r(hostname.as_string().c_str(), &hent, 
								tls_resolve_buffer, sizeof tls_resolve_buffer, 
								&he, &herrno);
	PLOG_IF(ERROR, ret < 0) << "::gethostbyname_r failed";

	if (ret == 0 && he != nullptr) {
		CHECK(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));

		out->addr_.sin_family = AF_INET;
		out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	return false;
}
#endif	// defined(OS_LINUX)

}	// namespace annety
