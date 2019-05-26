// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef ANT_CONDITION_VARIABLE_H_
#define ANT_CONDITION_VARIABLE_H_

#include <pthread.h>

#include "BuildConfig.h"
#include "Logging.h"
#include "MutexLock.h"

namespace annety {

class TimeDelta;

class ConditionVariable {
public:
  // Construct a cv for use with ONLY one user lock.
  explicit ConditionVariable(MutexLock* user_lock);

  ~ConditionVariable();

  // Wait() releases the caller's critical section atomically as it starts to
  // sleep, and the reacquires it when it is signaled. The wait functions are
  // susceptible to spurious wakeups. (See usage note 1 for more details.)
  void wait();
  void timed_wait(const TimeDelta& max_time);

  // Broadcast() revives all waiting threads. (See usage note 2 for more
  // details.)
  void broadcast();
  // Signal() revives one waiting thread.
  void signal();

private:
  pthread_cond_t condition_;
  pthread_mutex_t* user_mutex_;

#if DCHECK_IS_ON()
  MutexLock* const user_lock_;  // Needed to adjust shadow lock state on wait.
#endif
};

}  // namespace annety

#endif  // ANT_CONDITION_VARIABLE_H_
