
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#include "strings/StringPrintf.h"
#include "Macros.h"		// arraysize
#include "Logging.h"
#include "ScopedClearLastError.h"

#include <vector>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>	// errno
#include <stdio.h>	// vsnprintf

namespace annety
{
namespace
{
// Overloaded wrappers around vsnprintf and vswprintf. The buf_size parameter
// is the size of the buffer. These return the number of characters in the
// formatted string excluding the NUL terminator. If the buffer is not
// large enough to accommodate the formatted string without truncation, they
// return the number of characters that would be in the fully-formatted string
// (vsnprintf, and vswprintf on Windows), or -1 (vswprintf on POSIX platforms).
inline int vsnprintf_T(char* buffer,
					  size_t buf_size,
					  const char* format,
					  va_list argptr)
{
	return ::vsnprintf(buffer, buf_size, format, argptr);
}

// Templatized backend for string_printf/string_appendf. This does not finalize
// the va_list, the caller is expected to do that.
template <class StringType>
static void string_appendvt(StringType* dst,
							const typename StringType::value_type* format,
							va_list ap)
{
	// First try with a small fixed size buffer.
	typename StringType::value_type stack_buf[1024];

	va_list ap_copy;
	::va_copy(ap_copy, ap);

	// save last errno
	ScopedClearLastError last_error;

	int result = vsnprintf_T(stack_buf, arraysize(stack_buf), format, ap_copy);
	va_end(ap_copy);

	if (result >= 0 && result < static_cast<int>(arraysize(stack_buf))) {
		// It fit.
		dst->append(stack_buf, result);
		return;
	}

	// Repeatedly increase buffer size until it fits.
	int mem_length = arraysize(stack_buf);
	while (true) {
		if (result < 0) {
			if (errno != 0 && errno != EOVERFLOW) {
				return;
			}
			// Try doubling the buffer size.
			mem_length *= 2;
		} else {
			// We need exactly "result + 1" characters.
			mem_length = result + 1;
		}

		if (mem_length > 32 * 1024 * 1024) {
			// That should be plenty, don't try anything larger.  This protects
			// against huge allocations when using vsnprintf_T implementations that
			// return -1 for reasons other than overflow without setting errno.
			DLOG(WARNING) << "Unable to printf the requested string due to size.";
			return;
		}

		std::vector<typename StringType::value_type> mem_buf(mem_length);

		// NOTE: You can only use a va_list once.  Since we're in a while loop, we
		// need to make a new copy each time so we don't use up the original.
		::va_copy(ap_copy, ap);
		result = vsnprintf_T(&mem_buf[0], mem_length, format, ap_copy);
		::va_end(ap_copy);

		if ((result >= 0) && (result < mem_length)) {
			// It fit.
			dst->append(&mem_buf[0], result);
			return;
		}
	}
}

} // namespace anonymous

std::string string_printf(const char* format, ...)
{
	va_list ap;
	::va_start(ap, format);
	std::string result;
	string_appendv(&result, format, ap);
	va_end(ap);
	return result;
}

std::string string_printv(const char* format, va_list ap)
{
	std::string result;
	string_appendv(&result, format, ap);
	return result;
}

const std::string& sstring_printf(std::string* dst, const char* format, ...)
{
	va_list ap;
	::va_start(ap, format);
	dst->clear();
	string_appendv(dst, format, ap);
	::va_end(ap);
	return *dst;
}

const std::string& string_appendf(std::string* dst, const char* format, ...)
{
	va_list ap;
	::va_start(ap, format);
	string_appendv(dst, format, ap);
	::va_end(ap);
	return *dst;
}

void string_appendv(std::string* dst, const char* format, va_list ap)
{
	string_appendvt(dst, format, ap);
}

}	// namespace annety
