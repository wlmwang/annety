// By: Anny Wang
// Date: Oct 28 2019

#ifndef ANT_CRC32C_H
#define ANT_CRC32C_H

#include "build/BuildConfig.h"
#include "strings/StringPiece.h"

#include <stdint.h>
#include <stddef.h>

namespace annety
{
// crc32 lookup tables
namespace internal
{
extern uint32_t crc32_table16[];
extern uint32_t crc32_table256[];
}	// namespace anonymous

// The code are based on the algorithm described at core/ngx_crc32.h
class Crc32c
{
public:
	static uint32_t crc32_short(const StringPiece& buff)
	{
		return crc32_short(buff.data(), buff.size());
	}

	static uint32_t crc32_long(const StringPiece& buff)
	{
		return crc32_long(buff.data(), buff.size());
	}

	static void crc32_update(uint32_t *crc, const StringPiece& buff)
	{
		crc32_update(crc, buff.data(), buff.size());
	}

	// crc32, the effect is better when the buff is shorter, about 30-60 bytes
	static uint32_t crc32_short(const char *buff, size_t len)
	{
	    char    c;
	    uint32_t  crc;

	    crc = 0xffffffff;

	    while (len--) {
	        c = *buff++;
	        crc = internal::crc32_table16[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
	        crc = internal::crc32_table16[(crc ^ (c >> 4)) & 0xf] ^ (crc >> 4);
	    }

	    return crc ^ 0xffffffff;
	}

	// crc32, the effect is better when the buff is longer
	static uint32_t crc32_long(const char *buff, size_t len)
	{
	    uint32_t  crc;

	    crc = 0xffffffff;

	    while (len--) {
	        crc = internal::crc32_table256[(crc ^ *buff++) & 0xff] ^ (crc >> 8);
	    }

	    return crc ^ 0xffffffff;
	}

	static void crc32_update(uint32_t *crc, const char *buff, size_t len)
	{
	    uint32_t  c;

	    c = *crc;

	    while (len--) {
	        c = internal::crc32_table256[(c ^ *buff++) & 0xff] ^ (c >> 8);
	    }

	    *crc = c;
	}
};

}	// namespace annety

#endif  // ANT_CRC32C_H
