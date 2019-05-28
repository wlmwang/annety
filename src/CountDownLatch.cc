// Modify: Anny Wang
// Date: May 28 2019

#include "CountDownLatch.h"

namespace annety {
CountDownLatch::CountDownLatch(int count) 
	: lock_(), cv_(lock_), count_(count) {}

int CountDownLatch::get_count() const {
	AutoLock l(lock_);
	return count_;
}

void CountDownLatch::count_down() {
	AutoLock l(lock_);
	if (--count_ == 0) {
		cv_.broadcast();
	}
}

void CountDownLatch::await() {
	AutoLock l(lock_);
	while (count_ > 0) {
		cv_.wait();
	}
}

void CountDownLatch::await(const TimeDelta& max_time) {
	AutoLock l(lock_);
	while (count_ > 0) {
		cv_.timed_wait(max_time);
	}
}

}	// namespace annety
