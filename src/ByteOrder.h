// Modify: Anny Wang
// Date: May 08 2019

#ifndef ANT_BYTE_ORDER_H
#define ANT_BYTE_ORDER_H

#include <stdint.h>
#include <endian.h>

namespace annety {
inline uint64_t host_to_net64(uint64_t host64) {
	return htobe64(host64);
}

inline uint32_t host_to_net32(uint32_t host32) {
	return htobe32(host32);
}

inline uint16_t host_to_net16(uint16_t host16) {
	return htobe16(host16);
}

inline uint64_t net_to_host64(uint64_t net64) {
	return be64toh(net64);
}

inline uint32_t net_to_host32(uint32_t net32) {
	return be32toh(net32);
}

inline uint16_t net_to_host16(uint16_t net16) {
	return be16toh(net16);
}

}	// namespace annety

#endif  // ANT_BYTE_ORDER_H
