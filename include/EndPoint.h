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
// It is value sematics, which means that it can be copied or assigned.
// EndPoint is standard layout class, but it is neither a trivial class 
// nor a pod.
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
	
	// Set sockaddr to EndPoint
	void set_sockaddr_in(const struct sockaddr_in& addr) { addr_ = addr;}
	void set_sockaddr_in(const struct sockaddr_in6& addr6) { addr6_ = addr6;}

	std::string to_ip() const;
	uint16_t to_port() const;
	std::string to_ip_port() const;
	
	// For bind/connect/recvfrom/sendto...
	sa_family_t family() const;
	const struct sockaddr* get_sockaddr() const;

	// Resolve node+service to IPv4/IPv6 address (`dst`).
	// For bind/connect/recvfrom/sendto...
	// *Thread safe*
	static bool resolve(const StringPiece& node, const StringPiece& service, EndPoint* dst);

private:
	union
	{
		struct sockaddr_in addr_;
		struct sockaddr_in6 addr6_;
	};
};

}	// namespace annety

#endif	// _ANT_END_POINT_H_
