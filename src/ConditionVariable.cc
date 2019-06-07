// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#include "ConditionVariable.h"
#include "BuildConfig.h"
#include "MutexLock.h"
#include "Logging.h"
#include "Time.h"

#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace annety {
ConditionVariable::ConditionVariable(MutexLock& user_lock)
	: user_mutex_(user_lock.lock_.native_handle())
#if DCHECK_IS_ON()
	, user_lock_(user_lock)
#endif
{
	int rv = 0;
// Mac can use relative time deadlines.
#if !defined(OS_MACOSX)
	pthread_condattr_t attrs;
	rv = ::pthread_condattr_init(&attrs);
	DCHECK_EQ(0, rv);
	
	::pthread_condattr_setclock(&attrs, CLOCK_MONOTONIC);
	rv = ::pthread_cond_init(&condition_, &attrs);
	::pthread_condattr_destroy(&attrs);
#else
	rv = ::pthread_cond_init(&condition_, NULL);
#endif
	DCHECK_EQ(0, rv);
}

ConditionVariable::~ConditionVariable() {
#if defined(OS_MACOSX)
	{
		// This hack is necessary to avoid a fatal pthreads subsystem bug in the
		// Darwin kernel. http://crbug.com/517681.
		MutexLock lock;
		AutoLock locked(lock);
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 1;
		::pthread_cond_timedwait_relative_np(&condition_, 
											lock.lock_.native_handle(),
											&ts);
	}
#endif
	int rv = ::pthread_cond_destroy(&condition_);
	DCHECK_EQ(0, rv);
}

void ConditionVariable::wait() {
#if DCHECK_IS_ON()
	user_lock_.check_held_and_unmark();
#endif
	int rv = ::pthread_cond_wait(&condition_, user_mutex_);
	DCHECK_EQ(0, rv);
#if DCHECK_IS_ON()
	user_lock_.check_unheld_and_mark();
#endif
}

void ConditionVariable::timed_wait(const TimeDelta& max_time) {
	int64_t usecs = max_time.in_microseconds();
	struct timespec relative_time;
	relative_time.tv_sec = usecs / Time::kMicrosecondsPerSecond;
	relative_time.tv_nsec = (usecs % Time::kMicrosecondsPerSecond) * 
							Time::kNanosecondsPerMicrosecond;

#if DCHECK_IS_ON()
	user_lock_.check_held_and_unmark();
#endif

#if defined(OS_MACOSX)
	int rv = ::pthread_cond_timedwait_relative_np(&condition_, 
												  user_mutex_,
												  &relative_time);
#else
	// The timeout argument to pthread_cond_timedwait is in absolute time.
	struct timespec absolute_time;
	struct timespec now;
	::clock_gettime(CLOCK_MONOTONIC, &now);
	absolute_time.tv_sec = now.tv_sec;
	absolute_time.tv_nsec = now.tv_nsec;

	absolute_time.tv_sec += relative_time.tv_sec;
	absolute_time.tv_nsec += relative_time.tv_nsec;
	absolute_time.tv_sec += absolute_time.tv_nsec / Time::kNanosecondsPerSecond;
	absolute_time.tv_nsec %= Time::kNanosecondsPerSecond;
	DCHECK_GE(absolute_time.tv_sec, now.tv_sec);  // Overflow paranoia

	int rv = ::pthread_cond_timedwait(&condition_, user_mutex_, &absolute_time);
#endif  // OS_MACOSX

	// On failure, we only expect the CV to timeout. Any other error value means
	// that we've unexpectedly woken up.
	DCHECK(rv == 0 || rv == ETIMEDOUT);

#if DCHECK_IS_ON()
	user_lock_.check_unheld_and_mark();
#endif
}

void ConditionVariable::broadcast() {
	int rv = ::pthread_cond_broadcast(&condition_);
	DCHECK_EQ(0, rv);
}

void ConditionVariable::signal() {
	int rv = ::pthread_cond_signal(&condition_);
	DCHECK_EQ(0, rv);
}

}	// namespace annety
