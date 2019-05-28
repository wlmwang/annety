// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "Exception.h"

#include <cxxabi.h>     // abi::__cxa_demangle
#include <execinfo.h>   // backtrace, backtrace_symbols
#include <stdlib.h>

namespace annety {
std::string backtrace_to_string(bool demangle) {
	std::string stack;
	const int max_frames = 200;
	void* frame[max_frames];
	int nptrs = ::backtrace(frame, max_frames);
	char** strings = ::backtrace_symbols(frame, nptrs);
	if (strings) {
		size_t len = 256;
		char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
		for (int i = 1; i < nptrs; ++i) {
			if (demangle) {
				char* left_par = nullptr;
				char* plus = nullptr;
				// parsing format
				for (char* p = strings[i]; *p; ++p) {
					if (*p == '(') {
						left_par = p;
					} else if (*p == '+') {
						plus = p;
					}	
				}
				if (left_par && plus) {
					*plus = '\0';
					int status = 0;
					char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
					*plus = '+';
					if (status == 0) {
						demangled = ret;  // ret could be realloc()
						stack.append(strings[i], left_par+1);
						stack.append(demangled);
						stack.append(plus);
						stack.push_back('\n');
						continue;
					}
				}
			}
			// Fallback to mangled names
			stack.append(strings[i]);
			stack.push_back('\n');
		}
		free(demangled);
		free(strings);
	}
	return stack;
}

Exception::Exception(std::string msg)
	: message_(std::move(msg)),
	stack_(backtrace_to_string(true)) {}

}  // namespace annety
