// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#include "MutexLock.h"
#include "SafeStrerror.h"

#include <string>

namespace annety {
namespace internal {
// errno to string ------------------------------------------------
namespace {
#if DCHECK_IS_ON()
const char* additional_hint_for_system_error_code(int error_code) {
	switch (error_code) {
	case EINVAL:
		return "Hint: This is often related to a use-after-free.";
	default:
		return "";
	}
}
#endif  // DCHECK_IS_ON()

std::string system_error_code_to_string(int error_code) {
#if DCHECK_IS_ON()
	return safe_strerror(error_code) + ". " +
		additional_hint_for_system_error_code(error_code);

#else
	return std::string();

#endif  // DCHECK_IS_ON()
}

}	// namespace anonymous

LockImpl::LockImpl() {
	pthread_mutexattr_t mta;
	int rv = ::pthread_mutexattr_init(&mta);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
	
#ifndef NDEBUG
	// In debug, setup attributes for lock error checking.
	rv = ::pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
#endif

	rv = ::pthread_mutex_init(&native_handle_, &mta);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
	rv = ::pthread_mutexattr_destroy(&mta);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
}

LockImpl::~LockImpl() {
	int rv = ::pthread_mutex_destroy(&native_handle_);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
}

bool LockImpl::try_lock() {
	int rv = ::pthread_mutex_trylock(&native_handle_);
	DCHECK(rv == 0 || rv == EBUSY) << ". " << system_error_code_to_string(rv);
	return rv == 0;
}

void LockImpl::lock() {
	int rv = ::pthread_mutex_lock(&native_handle_);
	DCHECK_EQ(rv, 0) << ". " << system_error_code_to_string(rv);
}

void LockImpl::unlock() {
	int rv = ::pthread_mutex_unlock(&native_handle_);
	DCHECK_EQ(rv, 0) << ". " << safe_strerror(rv);
}

}  // namespace internal

#if DCHECK_IS_ON()
MutexLock::MutexLock() : lock_() {}

MutexLock::~MutexLock() {
	DCHECK(owning_thread_ref_.is_null());
}

void MutexLock::assert_acquired() const {
	DCHECK(owning_thread_ref_ == PlatformThread::current_ref());
}

void MutexLock::check_held_and_unmark() {
	DCHECK(owning_thread_ref_ == PlatformThread::current_ref());
	owning_thread_ref_ = PlatformThreadRef();
}

void MutexLock::check_unheld_and_mark() {
	DCHECK(owning_thread_ref_.is_null());
	owning_thread_ref_ = PlatformThread::current_ref();
}
#endif  // DCHECK_IS_ON()

}	// namespace annety


