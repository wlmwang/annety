// Modify: Anny Wang
// Date: May 29 2019

#include "ThreadPool.h"
#include "Logging.h"
#include "Exception.h"

#include <iostream>

namespace annety {
ThreadPool::ThreadPool(const std::string& name_prefix, int num_threads) 
	: name_prefix_(name_prefix),
	  num_threads_(num_threads),
	  lock_(),
	  empty_ev_(lock_),
	  full_ev_(lock_) {}

ThreadPool::~ThreadPool() {
	if (running_) {
		stop();
	}
}

void ThreadPool::start() {
	DCHECK(threads_.empty() && running_ == false) 
		<< "start() called with outstanding threads.";
	running_ = true;

	// start all tasker thread
	threads_.reserve(num_threads_);
	for (int i = 0; i < num_threads_; ++i) {
		threads_.emplace_back(new Thread(
								std::bind(&ThreadPool::thread_loop, this), 
								name_prefix_)
							);
		threads_[i]->start();
	}
	// current thread acts as a tasker thread 
	if (num_threads_ == 0 && thread_init_cb_) {
		thread_init_cb_();
	}
}

void ThreadPool::stop() {
	// tell all threads to quit their tasker loop.
	{
		AutoLock locked(lock_);
		DCHECK(!threads_.empty() && running_ == true) 
			<< "stop() called with no outstanding threads.";
		running_ = false;
		empty_ev_.broadcast();
	}
	// join all the tasker threads.
	for (auto& td : threads_) {
		td->join();
	}
}

size_t ThreadPool::get_tasker_size() const {
  AutoLock locked(lock_);
  return taskers_.size();
}

bool ThreadPool::is_full() const {
	lock_.assert_acquired();
	return max_tasker_size_ > 0 && taskers_.size() >= max_tasker_size_;
}

void ThreadPool::run_tasker(Tasker tasker) {
	if (threads_.empty()) {
		tasker();
	} else {
		AutoLock locked(lock_);
		while (is_full()) {
			full_ev_.wait();
		}
		DCHECK(!is_full()) << "full the taskers.";

		taskers_.push_back(std::move(tasker));
		empty_ev_.signal();
	}
}

ThreadPool::Tasker ThreadPool::pop() {
	AutoLock locked(lock_);
	while (taskers_.empty() && running_) {
		empty_ev_.wait();
	}
	DCHECK(!taskers_.empty() || running_ == false) << "empty the taskers.";

	Tasker task;
	if (!taskers_.empty()) {
		task = taskers_.front();
		taskers_.pop_front();

		if (max_tasker_size_ > 0) {
			full_ev_.signal();
		}
	}
	return task;
}

void ThreadPool::thread_loop() {
	try {
		if (thread_init_cb_) {
			thread_init_cb_();
		}
		// main loop
		while (running_) {
			Tasker task(pop());
			if (task) {
				task();
			}
		}
	} catch (const Exception& e) {
		LOG(FATAL) << "Thread Pool:" << name_prefix_ 
				   << "Exception:" << e.what() 
				   << "Backtrace:" << e.backtrace();
	} catch (const std::exception& e) {
		LOG(FATAL) << "Thread Pool:" << name_prefix_ 
				   << "Exception:" << e.what();
	} catch (...) {
		LOG(ERROR) << "Thread:" << name_prefix_;
		throw;
	}
}

}	// namespace annety
