// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 28 2019

#include "PlatformThread.h"
#include "Logging.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>

#if defined(OS_LINUX)
#include <sys/syscall.h>
#include <sys/prctl.h>
#endif

#include <memory>
#include <string>

namespace annety {
namespace {
struct ThreadParams {
	PlatformThread::ThreadMainFunc func_;

	ThreadParams(PlatformThread::ThreadMainFunc func) 
				: func_(std::move(func)) {}
};

// |params| -> ThreadParams
void* thread_func(void* params) {
	DCHECK(params);

	std::unique_ptr<ThreadParams> thread_params(
		static_cast<ThreadParams*>(params)
	);

	// start enter thread function
	thread_params->func_();
	return nullptr;
}

bool create_thread(bool joinable,
				   PlatformThread::ThreadMainFunc func,
				   PlatformThreadHandle* thread_handle) {
	DCHECK(thread_handle);

	pthread_attr_t attributes;
	pthread_attr_init(&attributes);

	// Pthreads are joinable by default, so only specify the detached
	// attribute if the thread should be non-joinable.
	if (!joinable) {
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	}

	std::unique_ptr<ThreadParams> params(
		new ThreadParams(std::move(func))
	);

	pthread_t handle;
	int err = pthread_create(&handle, &attributes, thread_func, params.get());
	bool success = !err;
	if (success) {
		// ThreadParams should be deleted on the created thread after used.
		IGNORE_UNUSED_RESULT(params.release());
	} else {
		// Value of |handle| is undefined if pthread_create fails.
		handle = 0;
		errno = err;
		PLOG(ERROR) << "pthread_create";
	}
	*thread_handle = PlatformThreadHandle(handle);

	pthread_attr_destroy(&attributes);
	return success;
}

}  // namespace anonymous

// static
PlatformThreadId PlatformThread::current_id() {
	// Pthreads doesn't have the concept of a thread ID, so we have to reach down
	// into the kernel.
#if defined(OS_MACOSX)
	return pthread_mach_thread_np(pthread_self());
#elif defined(OS_LINUX)
	return syscall(__NR_gettid);
#elif defined(OS_SOLARIS)
	return pthread_self();
#elif defined(OS_POSIX)
	return reinterpret_cast<int64_t>(pthread_self());
#endif
}

// static
PlatformThreadRef PlatformThread::current_ref() {
	return PlatformThreadRef(pthread_self());
}

// static
PlatformThreadHandle PlatformThread::current_handle() {
	return PlatformThreadHandle(pthread_self());
}

// static
void PlatformThread::yield_current_thread() {
	sched_yield();
}

// static
void PlatformThread::sleep(TimeDelta duration) {
	struct timespec sleep_time, remaining;

	// Break the duration into seconds and nanoseconds.
	// NOTE: TimeDelta's microseconds are int64s while timespec's
	// nanoseconds are longs, so this unpacking must prevent overflow.
	sleep_time.tv_sec = duration.in_seconds();
	duration -= TimeDelta::from_seconds(sleep_time.tv_sec);
	sleep_time.tv_nsec = duration.in_microseconds() * 1000;  // nanoseconds

	while (nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR) {
		sleep_time = remaining;
	}
}

// static
bool PlatformThread::create(ThreadMainFunc func,
							PlatformThreadHandle* thread_handle) {
	return create_thread(true, std::move(func), thread_handle);
}

// static
bool PlatformThread::create_non_joinable(ThreadMainFunc func) {
	PlatformThreadHandle unuse_handle;
	bool result = create_thread(false, std::move(func), &unuse_handle);
	return result;
}

// static
void PlatformThread::join(PlatformThreadHandle thread_handle) {
	CHECK_EQ(0, pthread_join(thread_handle.platform_handle(), nullptr));
}

// static
void PlatformThread::detach(PlatformThreadHandle thread_handle) {
	CHECK_EQ(0, pthread_detach(thread_handle.platform_handle()));
}

#if defined(OS_MACOSX)
// static
void PlatformThread::set_name(const std::string& name) {
	// Mac OS X does not expose the length limit of the name, so
	// hardcode it.
	const int kMaxNameLength = 63;
	std::string shortened_name = name.substr(0, kMaxNameLength);
	// pthread_setname() fails (harmlessly) in the sandbox, ignore when it does.
	// See http://crbug.com/47058
	pthread_setname_np(shortened_name.c_str());
}
#else
// static
void PlatformThread::set_name(const std::string& name) {
	if (PlatformThread::current_id() == getpid() || name.empty()) {
		return;
	}

	// http://0pointer.de/blog/projects/name-your-threads.html
	// Set the name for the LWP (which gets truncated to 15 characters).
	// Note that glibc also has a 'pthread_setname_np' api, but it may not be
	// available everywhere and it's only benefit over using prctl directly is
	// that it can set the name of threads other than the current thread.
	int err = prctl(PR_SET_NAME, name.c_str());

	// We expect EPERM failures in sandboxed processes, just ignore those.
	if (err < 0 && errno != EPERM) {
		DPLOG(ERROR) << "prctl(PR_SET_NAME)";
	}
}
#endif

}	// namespace annety
