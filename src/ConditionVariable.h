// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 8 2019

#ifndef ANT_CONDITION_VARIABLE_H_
#define ANT_CONDITION_VARIABLE_H_

#include "BuildConfig.h"
#include "MutexLock.h"
#include "Logging.h"

#include <pthread.h>

namespace annety {
class TimeDelta;

// ConditionVariable wraps pthreads condition variable synchronization .
// This functionality is very helpful for having several threads wait for 
// an event, as is common with a thread pool managed by a master.  
// The meaning of such an event in the (worker) thread pool scenario is 
// that additional tasks are now available for processing.

class ConditionVariable {
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
};

}  // namespace annety

#endif  // ANT_CONDITION_VARIABLE_H_
