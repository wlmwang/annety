// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 28 2019

#ifndef ANT_PLATFORM_THREAD_H_
#define ANT_PLATFORM_THREAD_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "threading/ThreadForward.h"

#include <functional>
#include <string>
#include <stddef.h>
#include <pthread.h>

namespace annety
{
// Example:
// // PlatformThreadHandle
// PlatformThreadHandle handle;
// PlatformThread::create([]() {
//		//... your code in thread...
// }, &handle);
// PlatformThread::join(handle);
// ...

// A namespace for low-level thread functions.
class PlatformThread
{
public:
	// Gets the current thread id, which may be useful for logging purposes.
	static ThreadId current_id();

	// Gets the current thread reference, which can be used to check if
	// we're on the right thread quickly.
	static ThreadRef current_ref();

	// FIXME: Can only work on threads created by PlatformThread class
	// Is it the main thread.
	static bool is_main_thread();

	// Yield the current thread so another thread can be scheduled.
	static void yield_current_thread();

	// Sleeps for the specified duration.
	static void sleep(TimeDelta duration);

	// Sets the thread name visible to debuggers/tools. This will try to
	// initialize the context for current thread unless it's a WorkerThread.
	static void set_name(const std::string& name);

	// Creates a new thread.  Upon success,
	// |*thread_ref| will be assigned a handle to the newly created thread,
	// and |fuc|'s TaskCallback method will be executed on the newly created
	// thread.
	// NOTE: When you are done with the thread handle, you must call Join to
	// release system resources associated with the thread.  You must ensure that
	// the Delegate object outlives the thread.
	static bool create(TaskCallback cb, ThreadRef* thread_ref);

	// create_non_joinable() does the same thing as Create() except the thread
	// cannot be join()'d.  Therefore, it also does not output a
	// ThreadRef.
	static bool create_non_joinable(TaskCallback cb);

	// Joins with a thread created via the Create function.  This function blocks
	// the caller until the designated thread exits.  This will invalidate
	// |thread_ref|.
	static void join(ThreadRef thread_ref);

	// Detaches and releases the thread handle. The thread is no longer joinable
	// and |thread_ref| is invalidated after this call.
	static void detach(ThreadRef thread_ref);
};

}	// namespace annety

#endif  // ANT_PLATFORM_THREAD_H_
