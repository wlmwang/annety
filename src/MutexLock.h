// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#ifndef ANT_MUTEX_LOCK_H_
#define ANT_MUTEX_LOCK_H_

#include "Macros.h"
#include "Logging.h"

#if DCHECK_IS_ON()
#include "ThreadForward.h"
#endif

#include <pthread.h>

namespace annety {
class ConditionVariable;

namespace internal {
// This class implements the underlying platform-specific spin-lock mechanism
// used for the Lock class.  Most users should not use LockImpl directly, but
// should instead use Lock.
class LockImpl {
public:
	LockImpl();
	~LockImpl();

	// If the lock is not held, take it and return true.  If the lock is already
	// held by something else, immediately return false.
	bool try_lock();

	// Take the lock, blocking until it is available if necessary.
	void lock();

	// Release the lock.  This must only be called by the lock's holder: after
	// a successful call to Try, or a call to Lock.
	void unlock();

	// Return the native underlying lock.
	pthread_mutex_t* native_handle() {
		return &native_handle_;
	}

private:
	pthread_mutex_t native_handle_;
	
	DISALLOW_COPY_AND_ASSIGN(LockImpl);
};

}	// namespace internal

// A convenient wrapper for an OS specific critical section.  The only real
// intelligence in this class is in debug mode for the support for the
// AssertAcquired() method.
class MutexLock {
public:
#if !DCHECK_IS_ON()
	// Optimized wrapper implementation
	MutexLock() : lock_() {}
	~MutexLock() {}

	// TODO(lukasza): https://crbug.com/831825: Add EXCLUSIVE_LOCK_FUNCTION
	// annotation to Acquire method and similar annotations to Release, Try and
	// AssertAcquired methods (here and in the #else branch).
	void lock() {
		lock_.lock();
	}
	void unlock() {
		lock_.unlock();
	}

	// If the lock is not held, take it and return true. If the lock is already
	// held by another thread, immediately return false. This must not be called
	// by a thread already holding the lock (what happens is undefined and an
	// assertion may fail).
	bool try_lock() {
		return lock_.try_lock();
	}

	// Null implementation if not debug.
	void assert_acquired() const {}

#else
	MutexLock();
	~MutexLock();

	// NOTE: We do not permit recursive locks and will commonly fire a DCHECK() if
	// a thread attempts to acquire the lock a second time (while already holding
	// it).
	void lock() {
		lock_.lock();
		check_unheld_and_mark();
	}
	void unlock() {
		check_held_and_unmark();
		lock_.unlock();
	}

	bool try_lock() {
		bool rv = lock_.try_lock();
		if (rv) {
	  		check_unheld_and_mark();
		}
		return rv;
	}

	void assert_acquired() const;

#endif  // DCHECK_IS_ON()
	// Both Windows and POSIX implementations of ConditionVariable need to be
	// able to see our lock and tweak our debugging counters, as they release and
	// acquire locks inside of their condition variable APIs.
	friend class ConditionVariable;

private:
#if DCHECK_IS_ON()
	// Members and routines taking care of locks assertions.
	// Note that this checks for recursive locks and allows them
	// if the variable is set.  This is allowed by the underlying implementation
	// on windows but not on Posix, so we're doing unneeded checks on Posix.
	// It's worth it to share the code.
	void check_held_and_unmark();
	void check_unheld_and_mark();

	// All private data is implicitly protected by lock_.
	// Be VERY careful to only access members under that lock.
	ThreadRef owning_thread_ref_;

#endif  // DCHECK_IS_ON()

	// Platform specific underlying lock implementation.
	internal::LockImpl lock_;

	DISALLOW_COPY_AND_ASSIGN(MutexLock);
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
public:
	struct AlreadyAcquired {};

	explicit AutoLock(MutexLock& lock) : lock_(lock) {
		lock_.lock();
	}

	AutoLock(MutexLock& lock, const AlreadyAcquired&) : lock_(lock) {
		lock_.assert_acquired();
	}

	~AutoLock() {
		lock_.assert_acquired();
		lock_.unlock();
	}

private:
	MutexLock& lock_;

	DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

// AutoUnlock is a helper that will Release() the |lock| argument in the
// constructor, and re-Acquire() it in the destructor.
class AutoUnlock {
public:
	explicit AutoUnlock(MutexLock& lock) : lock_(lock) {
		// We require our caller to have the lock.
		lock_.assert_acquired();
		lock_.unlock();
	}

	~AutoUnlock() {
		lock_.lock();
	}

private:
	MutexLock& lock_;

	DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

}  // namespace annety

#endif  // ANT_MUTEX_LOCK_H_
