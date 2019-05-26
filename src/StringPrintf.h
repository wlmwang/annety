
// Copyright (c) 2018 Anny Wang Authors. All rights reserved.

#ifndef _ANT_STRING_PRINTF_H_
#define _ANT_STRING_PRINTF_H_

#include <stdarg.h>   // va_list

#include <string>

#include "BuildConfig.h"
#include "CompilerSpecific.h"

namespace annety {

std::string StringPrintf(const char* format, ...);

std::string StringPrintV(const char* format, va_list ap);

const std::string& SStringPrintf(std::string* dst, const char* format, ...);

void StringAppendF(std::string* dst, const char* format, ...);

void StringAppendV(std::string* dst, const char* format, va_list ap);

}  // namespace annety

#endif  // _ANT_STRING_PRINTF_H_
