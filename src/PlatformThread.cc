// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 28 2019

#include "PlatformThread.h"
#include "build/CompilerSpecific.h"
#include "Logging.h"

#include <unistd.h>		// getpid
#include <errno.h>		// errno
#include <sched.h>		// sched_yield
#include <time.h>		// timespec, nanosleep
#include <pthread.h>
#include <stddef.h>

#if defined(OS_LINUX)
#include <sys/syscall.h>	// syscall
#include <sys/prctl.h>		// prctl
#endif

#include <utility>	// std::move
#include <memory>	// std::unique_ptr
#include <string>

namespace annety
{
namespace
{
// default is the main thread
thread_local bool tls_is_main_thread{true};

struct ThreadParams
{
public:
	TaskCallback thread_main_cb_;

public:
	ThreadParams(TaskCallback cb) 
		: thread_main_cb_(std::move(cb)) {}
};

// |params| -> ThreadParams
void* thread_func(void* params)
{
	tls_is_main_thread = false;

	DCHECK(params);
	std::unique_ptr<ThreadParams> thread_params(static_cast<ThreadParams*>(params));

	// start enter thread function
	thread_params->thread_main_cb_();

	::pthread_exit(0);
	return nullptr;
}

bool create_thread(bool joinable, TaskCallback cb, ThreadRef* thread_ref)
{
	DCHECK(thread_ref);

	pthread_attr_t attributes;
	::pthread_attr_init(&attributes);

	// Pthreads are joinable by default, so only specify the detached
	// attribute if the thread should be non-joinable.
	if (!joinable) {
		::pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	}

	std::unique_ptr<ThreadParams> params(new ThreadParams(std::move(cb)));

	pthread_t ref;
	int err = ::pthread_create(&ref, &attributes, thread_func, params.get());
	bool success = !err;
	if (success) {
		// ThreadParams should be deleted on the created thread after used.
		IGNORE_UNUSED_RESULT(params.release());
	} else {
		// Value of |ref| is undefined if pthread_create fails.
		ref = 0;
		errno = err;
		PLOG(ERROR) << "pthread_create";
	}
	*thread_ref = ThreadRef(ref);

	::pthread_attr_destroy(&attributes);
	return success;
}

}  // namespace anonymous

// static
ThreadId PlatformThread::current_id()
{
	// Pthreads doesn't have the concept of a thread ID, so we have to reach down
	// into the kernel.
#if defined(OS_MACOSX)
	return pthread_mach_thread_np(::pthread_self());
#elif defined(OS_LINUX)
	return ::syscall(__NR_gettid);
#elif defined(OS_SOLARIS)
	return ::pthread_self();
#elif defined(OS_POSIX)
	return reinterpret_cast<int64_t>(::pthread_self());
#endif
}

// static
ThreadRef PlatformThread::current_ref()
{
	return ThreadRef(::pthread_self());
}

// static
bool PlatformThread::is_main_thread()
{
	return tls_is_main_thread;
}

// static
void PlatformThread::yield_current_thread()
{
	::sched_yield();
}

// static
void PlatformThread::sleep(TimeDelta duration)
{
	struct timespec sleep_time, remaining;

	// Break the duration into seconds and nanoseconds.
	// NOTE: TimeDelta's microseconds are int64s while timespec's
	// nanoseconds are longs, so this unpacking must prevent overflow.
	sleep_time.tv_sec = duration.in_seconds();
	duration -= TimeDelta::from_seconds(sleep_time.tv_sec);
	sleep_time.tv_nsec = duration.in_microseconds() * 1000;  // nanoseconds

	while (::nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR) {
		sleep_time = remaining;
	}
}

// static
bool PlatformThread::create(TaskCallback cb, ThreadRef* thread_ref)
{
	return create_thread(true, std::move(cb), thread_ref);
}

// static
bool PlatformThread::create_non_joinable(TaskCallback cb)
{
	ThreadRef unuse_ref;
	return create_thread(false, std::move(cb), &unuse_ref);
}

// static
void PlatformThread::join(ThreadRef thread_ref)
{
	CHECK_EQ(0, pthread_join(thread_ref.ref(), nullptr));
}

// static
void PlatformThread::detach(ThreadRef thread_ref)
{
	CHECK_EQ(0, pthread_detach(thread_ref.ref()));
}

#if defined(OS_MACOSX)
// static
void PlatformThread::set_name(const std::string& name)
{
	// Mac OS X does not expose the length limit of the name, so
	// hardcode it.
	const int kMaxNameLength = 63;
	std::string shortened_name = name.substr(0, kMaxNameLength);
	// pthread_setname() fails (harmlessly) in the sandbox, ignore when it does.
	// See http://crbug.com/47058
	::pthread_setname_np(shortened_name.c_str());
}
#else
// static
void PlatformThread::set_name(const std::string& name)
{
	if (PlatformThread::current_id() == ::getpid() || name.empty()) {
		return;
	}

	// http://0pointer.de/blog/projects/name-your-threads.html
	// Set the name for the LWP (which gets truncated to 15 characters).
	// Note that glibc also has a 'pthread_setname_np' api, but it may not be
	// available everywhere and it's only benefit over using prctl directly is
	// that it can set the name of threads other than the current thread.
	int err = ::prctl(PR_SET_NAME, name.c_str());

	// We expect EPERM failures in sandboxed processes, just ignore those.
	if (err < 0 && errno != EPERM) {
		DPLOG(ERROR) << "prctl(PR_SET_NAME)";
	}
}
#endif

}	// namespace annety
