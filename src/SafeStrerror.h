// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 8 2019

#ifndef ANT_SAFE_STRERROR_H_
#define ANT_SAFE_STRERROR_H_

#include "StringPiece.h"

#include <string>
#include <stddef.h>

namespace annety {
// Use this instead of strerror_r().
StringPiece safe_strerror_r(int err, char *buf, size_t len);

// Use C++11 thread_local keyword to share memory in one thread
StringPiece fast_safe_strerror(int err);

// Calls safe_strerror_r with a buffer of suitable size and returns the result
// in a C++ string.
//
// Use this instead of strerror(). Note though that safe_strerror_r will be
// more robust in the case of heap corruption errors, since it doesn't need to
// allocate a string.
std::string safe_strerror(int err);

}  // namespace annety

#endif  // ANT_SAFE_STRERROR_H_

