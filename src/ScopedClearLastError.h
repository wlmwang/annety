// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 8 2019

#ifndef _ANT_SCOPED_CLEAR_LAST_ERROR_H_
#define _ANT_SCOPED_CLEAR_LAST_ERROR_H_

#include "BuildConfig.h"
#include "CompilerSpecific.h"

#include <errno.h>

namespace annety {
class ScopedClearLastError {
public:
	ScopedClearLastError() : last_errno_(errno) {
		errno = 0;
	}
	~ScopedClearLastError() {
		errno = last_errno_;
	}

private:
  const int last_errno_;
};

}	// namespace annety

#endif
