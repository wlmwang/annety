// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#ifndef ANT_SYNCHRONIZATION_CONDITION_VARIABLE_H_
#define ANT_SYNCHRONIZATION_CONDITION_VARIABLE_H_

#include "Macros.h"
#include "Logging.h"
#include "TimeStamp.h"
#include "synchronization/MutexLock.h"

#include <pthread.h>

namespace annety
{
// Example:
// // ConditionVariable
// MutexLock lock;
// ConditionVariable cv(lock);
// std::vector<int> queue;
//
// Thread producer([&lock, &cv]() {
//		for (int i = 0; i < 10; i++) {
//			AutoLock locked(lock);
// 			while (!queue.empty()) {
//				cv.wait();
//			}
//			LOG(INFO) << "producer:" << i << "|" << pthread_self();
//			queue.push(i);
//			cv.signal();
//		}
// }, "annety-producer");
// producer.start();
//
// Thread consumer([&lock, &cv]() {
//		while (true) {
//			AutoLock locked(lock);
//			while (queue.empty()) {
//				cv.wait();
//			}
// 			LOG(INFO) << "consumer:" << queue.pop() << "|" << pthread_self();
//			cv.signal();
//		}
// }, "annety-consumer");
// consumer.start();
//
// producer.join();
// consumer.join();	// will hold
// ...

// ConditionVariable wraps pthreads condition variable synchronization .
// This functionality is very helpful for having several threads wait for 
// an event, as is common with a thread pool managed by a master.  
// The meaning of such an event in the (worker) thread pool scenario is 
// that additional tasks are now available for processing.

class TimeDelta;

class ConditionVariable
{
public:
	// Construct a cv for use with ONLY one user lock.
	explicit ConditionVariable(MutexLock& user_lock);

	~ConditionVariable();

	// wait() releases the caller's critical section atomically as it starts to
	// sleep, and the reacquires it when it is signaled. The wait functions are
	// susceptible to spurious wakeups. (See usage note 1 for more details.)
	void wait();
	void timed_wait(const TimeDelta& max_time);

	// broadcast() revives all waiting threads. (See usage note 2 for more
	// details.)
	void broadcast();

	// signal() revives one waiting thread.
	void signal();

private:
	pthread_cond_t condition_;
	pthread_mutex_t* user_mutex_;

#if DCHECK_IS_ON()
	MutexLock& user_lock_;  // Needed to adjust shadow lock state on wait.
#endif

	DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

}	// namespace annety

#endif  // ANT_SYNCHRONIZATION_CONDITION_VARIABLE_H_
