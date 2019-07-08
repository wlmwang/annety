// Refactoring: Anny Wang
// Date: May 28 2019

#include "synchronization/CountDownLatch.h"
#include "Times.h"

namespace annety
{
CountDownLatch::CountDownLatch(int count) 
	: count_(count) {}

int CountDownLatch::get_count() const
{
	AutoLock locked(lock_);
	return count_;
}

void CountDownLatch::count_down()
{
	AutoLock locked(lock_);
	if (--count_ == 0) {
		cv_.broadcast();
	}
}

void CountDownLatch::await()
{
	AutoLock locked(lock_);
	while (count_ > 0) {
		cv_.wait();
	}
}

void CountDownLatch::await(const TimeDelta& max_time)
{
	AutoLock locked(lock_);
	while (count_ > 0) {
		cv_.timed_wait(max_time);
	}
}

}	// namespace annety
