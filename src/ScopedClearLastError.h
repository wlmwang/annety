

// Copyright (c) 2018 Anny Wang Authors. All rights reserved.

#ifndef _ANT_SCOPED_CLEAR_LAST_ERROR_H_
#define _ANT_SCOPED_CLEAR_LAST_ERROR_H_

#include <errno.h>

#include "BuildConfig.h"
#include "CompilerSpecific.h"
// #include "logging.h"

namespace annety {

class ScopedClearLastError {
public:
	ScopedClearLastError() : last_errno_(errno) { errno = 0; }
	~ScopedClearLastError() { errno = last_errno_; }

private:
  const int last_errno_;
};

}	// namespace annety

#endif

