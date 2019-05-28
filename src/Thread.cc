// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 28 2019

#include "Thread.h"
#include "Logging.h"
#include "PlatformThread.h"
#include "CountDownLatch.h"
#include "StringPrintf.h"
#include "Exception.h"

namespace annety {
Thread::Thread(ThreadMainFunc func, const std::string& name_prefix)
	: Thread(std::move(func), name_prefix, Options()) {}

Thread::Thread(ThreadMainFunc func, const std::string& name_prefix,
			   const Options& options)
	: name_prefix_(name_prefix),
	options_(options),
	func_(std::move(func)) {}

Thread::~Thread() {
	DCHECK(has_been_started()) << "Thread was never started.";
	DCHECK(!options_.joinable || has_been_joined())
		<< "Joinable Thread destroyed without being Join()ed.";
}

void Thread::start() {
	start_async();
	// Wait for the thread to complete initialization.
	latch_.await();
	started_ = true;
}

void Thread::join() {
	DCHECK(options_.joinable) << "A non-joinable thread can't be joined.";
	DCHECK(has_start_been_attempted()) << "Tried to Join a never-started thread.";
	DCHECK(!has_been_joined()) << "Tried to Join a thread multiple times.";

	PlatformThread::join(thread_);
	thread_ = PlatformThreadHandle();
	joined_ = true;
}

void Thread::start_async() {
	DCHECK(!has_start_been_attempted()) << "Tried to Start a thread multiple times.";
	start_called_ = true;
	bool success = options_.joinable
		? PlatformThread::create(std::bind(&Thread::start_routine, this), &thread_)
		: PlatformThread::create_non_joinable(std::bind(&Thread::start_routine, this));
	CHECK(success);
}

PlatformThreadId Thread::tid() {
	DCHECK(has_been_started());
	return tid_;
}

void Thread::start_routine() {
	tid_ = PlatformThread::current_id();
	
	// Construct our full name of the form "name_prefix_/TID".
	name_ = string_printf("%s/%d", name_prefix_.c_str(), tid_);
	PlatformThread::set_name(name_);

	// We've initialized our new thread, signal that we're done to start().
	latch_.count_down();
	
	try {
		func_();
	} catch (const Exception& ex) {
		LOG(FATAL) << "thread name:" << name_ 
				   << "exception:" << ex.what() 
				   << "backtrace:" << ex.backtrace();
	} catch (const std::exception& ex) {
		LOG(FATAL) << "thread name:" << name_ 
				   << "exception:" << ex.what();
	} catch (...) {
		LOG(ERROR) << "thread name:" << name_;
		throw;
	}
}

}	// namespace annety
