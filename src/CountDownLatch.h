// Refactoring: Anny Wang
// Date: May 28 2019

#ifndef ANT_COUNT_DOWN_LATCH_H
#define ANT_COUNT_DOWN_LATCH_H

#include "CompilerSpecific.h"
#include "ConditionVariable.h"
#include "MutexLock.h"

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
	mutable MutexLock lock_{};
	ConditionVariable cv_{lock_};
	int count_{0};

	DISALLOW_COPY_AND_ASSIGN(CountDownLatch);
};

}	// namespace annety

#endif  // ANT_COUNT_DOWN_LATCH_H
