// By: wlmwang
// Date: Jun 17 2019

#ifndef _ANT_END_POINT_H_
#define _ANT_END_POINT_H_

#include "build/BuildConfig.h"
#include "strings/StringPiece.h"

#include <string>
#include <stdint.h>
#include <stddef.h>		// offsetof macro
#include <netinet/in.h>	// in_addr

namespace annety
{
// Wrapper for sockaddr_in[6], it expose sockaddr* interface to users.
//
// EndPoint is standard layout class, but it is neither a trivial class nor a pod.
// It is value sematics, which means that it can be copied or assigned.
class EndPoint
{
public:
	// |port| to EndPoint, ip default "0.0.0.0"
	explicit EndPoint(uint16_t port = 0, bool loopback_only = false, bool ipv6 = false);

	// |ip| "127.0.0.1"/port to EndPoint
	EndPoint(const StringPiece& ip, uint16_t port, bool ipv6 = false);

	// sockaddr_in[6] => EndPoint
	explicit EndPoint(const struct sockaddr_in& addr) : addr_(addr) {}
	explicit EndPoint(const struct sockaddr_in6& addr) : addr6_(addr) {}
	
	// default copy/assign is ok
	EndPoint(const EndPoint&) = default;
	EndPoint& operator=(const EndPoint&) = default;
	~EndPoint() = default;
	
	// set sockaddr to EndPoint
	void set_sockaddr_in(const struct sockaddr_in& addr) { addr_ = addr;}
	void set_sockaddr_in(const struct sockaddr_in6& addr6) { addr6_ = addr6;}

	// for bind/connect/recvfrom/sendto etc.
	sa_family_t family() const;
	const struct sockaddr* get_sockaddr() const;

	std::string to_ip() const;
	uint16_t to_port() const;
	std::string to_ip_port() const;

#if defined(OS_LINUX)
	// Resolve hostname to IP address, change port or sin_family if return 
	// true.  --- For IPv4.
	// *Thread safe*
	static bool resolve(const StringPiece& hostname, EndPoint* dst);
#endif	// defined(OS_LINUX)

private:
	// must sin[6]_family offset is 0
	union
	{
		struct sockaddr_in addr_;
		struct sockaddr_in6 addr6_;
	};
};

}	// namespace annety

#endif	// _ANT_END_POINT_H_
