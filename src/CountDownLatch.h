// Modify: Anny Wang
// Date: May 8 2019

#ifndef ANT_COUNT_DOWN_LATCH_H
#define ANT_COUNT_DOWN_LATCH_H

#include "MutexLock.h"
#include "ConditionVariable.h"

namespace annety {
class TimeDelta;

class CountDownLatch {
public:
	explicit CountDownLatch(int count);
	
	void count_down();
	int get_count() const;

  	void await();
  	void await(const TimeDelta& max_time);

private:
	mutable MutexLock lock_;
	ConditionVariable cv_;
	int count_;
};

}	// namespace annety

#endif  // ANT_COUNT_DOWN_LATCH_H
