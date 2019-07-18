// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 16 2019

#ifndef ANT_THREAD_FORWARD_H_
#define ANT_THREAD_FORWARD_H_

#include "Macros.h"
#include "strings/StringPiece.h"

#if defined(OS_MACOSX)
#include <mach/mach_types.h>
#endif

#include <unistd.h>
#include <pthread.h>

namespace annety
{
// Used for logging. Always an integer value.
#if defined(OS_MACOSX)
typedef mach_port_t ThreadId;
#elif defined(OS_POSIX)
typedef pid_t ThreadId;
#endif

namespace threads
{
ThreadId tid();
StringPiece tid_string();
bool is_main_thread();
}	// namespace threads

const ThreadId kInvalidThreadId{0};

// Used for thread checking and debugging.
// Meant to be as fast as possible.
// These are produced by PlatformThread::CurrentRef(), and used to later
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
		return id_ == other.id_;
	}

	bool operator!=(ThreadRef other) const
	{
		return id_ != other.id_;
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

#endif	// ANT_THREAD_FORWARD_H_
