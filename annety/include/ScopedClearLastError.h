// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#ifndef ANT_SCOPED_CLEAR_LAST_ERROR_H_
#define ANT_SCOPED_CLEAR_LAST_ERROR_H_

#include "ScopedGeneric.h"

#include <errno.h>

namespace annety
{
namespace internal
{
class ScopedClearLastErrorTraits
{
public:
	ScopedClearLastErrorTraits() { errno = 0;}

	static int invalid_value() { return -1;}

	void free(int) { errno = last_errno_;}

private:
	const int last_errno_{errno};
};

}	// namespace internal

typedef ScopedGeneric<int, internal::ScopedClearLastErrorTraits> 
		ScopedClearLastError;

}	// namespace annety

#endif	// ANT_SCOPED_CLEAR_LAST_ERROR_H_
