// By: Anny Wang
// Date: May 08 2019

#ifndef ANT_BYTE_ORDER_H
#define ANT_BYTE_ORDER_H

#include "build/BuildConfig.h"
#include "Logging.h"

#include <stdint.h>		// uint16_t,uint64_t,uintptr_t

namespace annety
{
// see: https://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Other-Builtins.html
// Returns a value with all bytes in |x| swapped.
inline uint16_t byte_swap(uint16_t x)
{
	return __builtin_bswap16(x);
}
inline uint32_t byte_swap(uint32_t x)
{
	return __builtin_bswap32(x);
}
inline uint64_t byte_swap(uint64_t x)
{
	return __builtin_bswap64(x);
}

inline uintptr_t byte_swap_uintptr_t(uintptr_t x)
{
	// We do it this way because some build configurations are ILP32 even when
	// defined(ARCH_CPU_64_BITS). Unfortunately, we can't use sizeof in #ifs. But,
	// because these conditionals are constexprs, the irrelevant branches will
	// likely be optimized away, so this construction should not result in code
	// bloat.
	if (sizeof(uintptr_t) == 4) {
		return byte_swap(static_cast<uint32_t>(x));
	} else if (sizeof(uintptr_t) == 8) {
		return byte_swap(static_cast<uint64_t>(x));
	}
	NOTREACHED();
}

// Converts the bytes in |x| from network to host order (endianness), and
// returns the result.
inline uint16_t net_to_host16(uint16_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}
inline uint32_t net_to_host32(uint32_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}
inline uint64_t net_to_host64(uint64_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}

// Converts the bytes in |x| from host to network order (endianness), and
// returns the result.
inline uint16_t host_to_net16(uint16_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}
inline uint32_t host_to_net32(uint32_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}
inline uint64_t host_to_net64(uint64_t x)
{
#if defined(ARCH_CPU_LITTLE_ENDIAN)
	return byte_swap(x);
#else
	return x;
#endif
}

}	// namespace annety

#endif  // ANT_BYTE_ORDER_H
