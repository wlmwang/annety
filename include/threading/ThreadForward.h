// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 16 2019

#ifndef ANT_THREADING_THREAD_FORWARD_H_
#define ANT_THREADING_THREAD_FORWARD_H_

#include "Macros.h"
#include "strings/StringPiece.h"

#if defined(OS_MACOSX)
#include <mach/mach_types.h>
#endif

#include <unistd.h>
#include <pthread.h>

namespace annety
{
// Thread user entry function
using TaskCallback = std::function<void()>;

// Used for logging. Always an integer value.
#if defined(OS_MACOSX)
// FIXME: consistent with POSIX for printf*
// typedef mach_port_t ThreadId;
typedef int64_t ThreadId;
#elif defined(OS_POSIX)
typedef int64_t ThreadId;
#endif

const ThreadId kInvalidThreadId{0};

namespace threads {
ThreadId tid();
StringPiece tid_string();
// It may not work before running the main function.
bool is_main_thread();
}	// namespace threads

// Used for thread checking and debugging.
// Meant to be as fast as possible.
// These are produced by PlatformThread::current_ref(), and used to later
// check if we are on the same thread or not by using ==. These are safe
// to copy between threads, but can't be copied to another process as they
// have no meaning there. Also, the internal identifier can be re-used
// after a thread dies, so a ThreadRef cannot be reliably used
// to distinguish a new thread from an old, dead thread.
class ThreadRef
{
public:
	using RefType = pthread_t;

	constexpr ThreadRef() : id_(0) {}
	explicit constexpr ThreadRef(RefType id) : id_(id) {}

	~ThreadRef() = default;

	bool operator==(ThreadRef other) const
	{
		return pthread_equal(id_, other.id_);
	}

	bool operator!=(ThreadRef other) const
	{
		return !(*this==other);
	}

	bool empty() const
	{
		return id_ == 0;
	}
	
	RefType ref() const
	{
		return id_;
	}

private:
	RefType id_;
};

}	// namespace annety

#endif	// ANT_THREADING_THREAD_FORWARD_H_
