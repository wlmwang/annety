// By: wlmwang
// Date: May 28 2019

#ifndef ANT_SYNCHRONIZATION_COUNT_DOWN_LATCH_H
#define ANT_SYNCHRONIZATION_COUNT_DOWN_LATCH_H

#include "Macros.h"
#include "synchronization/MutexLock.h"
#include "synchronization/ConditionVariable.h"

namespace annety
{
class TimeDelta;

class CountDownLatch
{
public:
	explicit CountDownLatch(int count);
	
	void count_down();
	int get_count() const;

  	void wait();
  	void wait(const TimeDelta& max_time);

private:
	mutable MutexLock lock_;
	ConditionVariable cv_{lock_};
	
	int count_;

	DISALLOW_COPY_AND_ASSIGN(CountDownLatch);
};

}	// namespace annety

#endif  // ANT_SYNCHRONIZATION_COUNT_DOWN_LATCH_H
