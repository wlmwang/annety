// By: wlmwang
// Date: May 28 2019

#ifndef ANT_SYNCHRONIZATION_COUNT_DOWN_LATCH_H
#define ANT_SYNCHRONIZATION_COUNT_DOWN_LATCH_H

#include "Macros.h"
#include "synchronization/MutexLock.h"
#include "synchronization/ConditionVariable.h"

namespace annety
{
// Example:
// // CountDownLatch
// CountDownLatch latch(1);
//
// Thread master([&latch]() {
//		LOG(INFO) << "master start:" << pthread_self();
//		latch.wait();
//		LOG(INFO) << "master end:" << pthread_self();
// }, "annety-master");
// master.start();
//
// Thread worker([&latch]() {
//		LOG(INFO) << "worker start:" << pthread_self();
//		latch.count_down();
//		LOG(INFO) << "worker end:" << pthread_self();
// }, "annety-worker");
// worker.start();
//
// master.join();
// worker.join();
// ...

class TimeDelta;

// CountDownLatch can make multi-threads wait at the "door" until the latch count is 0.
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
