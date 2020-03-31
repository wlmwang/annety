// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 02 2019

#ifndef ANT_SCOPED_FILE_H_
#define ANT_SCOPED_FILE_H_

#include "ScopedGeneric.h"
#include "EintrWrapper.h"
#include "Logging.h"

#include <memory>	// std::unique_ptr
#include <stdio.h>	// FILE,::fclose
#include <errno.h>	// errno
#include <unistd.h>	// ::close

namespace annety
{
namespace internal
{
struct ScopedFDCloseTraits
{
public:
	static int invalid_value() { return -1;}

	static void free(int fd)
	{
		// It's important to crash here.
		// There are security implications to not closing a file descriptor
		// properly. As file descriptors are "capabilities", keeping them open
		// would make the current process keep access to a resource. Much of
		// Chrome relies on being able to "drop" such access.
		// It's especially problematic on Linux with the setuid sandbox, where
		// a single open directory would bypass the entire security model.
		int ret = IGNORE_EINTR(::close(fd));

#if defined(OS_LINUX) || defined(OS_MACOSX)
		// NB: Some file descriptors can return errors from close() e.g. network
		// filesystems such as NFS and Linux input devices. On Linux, macOS, and
		// Fuchsia's POSIX layer, errors from close other than EBADF do not indicate
		// failure to actually close the fd.
		if (ret != 0 && errno != EBADF) {
			ret = 0;
		}
#endif
		PCHECK(0 == ret);
	}
};

// Functor for |ScopedFILE| (below).
struct ScopedFILECloser
{
public:
	inline void operator()(FILE* x) const
	{
		if (x) {
			::fclose(x);
		}
	}
};

}  // namespace internal

// -----------------------------------------------------------------------------
// A low-level Posix file descriptor closer class. Use this when writing
// platform-specific code, especially that does non-file-like things with the
// FD (like sockets).
//
// If you're writing cross-platform code that deals with actual files, you
// should generally use annety::File instead which can be constructed with a
// handle, and in addition to handling ownership, has convenient cross-platform
// file manipulation functions on it.
typedef ScopedGeneric<int, internal::ScopedFDCloseTraits> ScopedFD;

// Automatically closes |FILE*|s.
typedef std::unique_ptr<FILE, internal::ScopedFILECloser> ScopedFILE;

}  // namespace annety

#endif  // ANT_SCOPED_FILE_H_
