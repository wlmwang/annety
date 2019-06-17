// Refactoring: Anny Wang
// Date: May 29 2019

#ifndef ANT_THREAD_POOL_H_
#define ANT_THREAD_POOL_H_

#include "Thread.h"
#include "MutexLock.h"
#include "ConditionVariable.h"

#include <functional>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <stddef.h>

namespace annety {
// You just call run_tasker() to add a task to the list of work to be done.
// joinall() will make sure that all outstanding work is processed, and wait
// for everything to finish.  You can reuse a pool, so you can call start()
// again after you've called join_all().
class ThreadPool {
public:
	typedef std::function<void()> Tasker;

	explicit ThreadPool(int num_threads, const std::string& name_prefix);
	~ThreadPool();

	// Start up all of the underlying threads, and start processing work if we
	// have any.
	ThreadPool& start();

	// Force stop all of the underlying threads even if the work is not finished
	void stop();

	// Make sure all outstanding work is finished, and wait for and destroy all
	// of the underlying threads in the pool.
	void joinall();

	// It is safe to run_tasker() any time, before or after start().
	void run_tasker(const Tasker& tasker, int repeat_count = 1);

	// Taskers queue size
	size_t get_tasker_size() const;

	// Must be called before start().
	void set_max_tasker_size(size_t max_tasker_size) {
		max_tasker_size_ = max_tasker_size;
	}
	void set_thread_init_cb(const Tasker& cb) {
		thread_init_cb_ = cb;
	}

private:
	bool is_full() const;
	void thread_loop();

private:
	const std::string name_prefix_;
	int num_threads_{0};
	size_t max_tasker_size_{0};
	bool running_{false};
	Tasker thread_init_cb_{};

	mutable MutexLock lock_;
	ConditionVariable empty_cv_;
	ConditionVariable full_cv_;

	std::deque<Tasker> taskers_;
	std::vector<std::unique_ptr<Thread>> threads_;
};

}	// namespace annety

#endif	// ANT_THREAD_POOL_H_
