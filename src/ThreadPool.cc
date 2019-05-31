// Modify: Anny Wang
// Date: May 29 2019

#include "ThreadPool.h"
#include "Logging.h"
#include "Exception.h"

namespace annety {
ThreadPool::ThreadPool(const std::string& name_prefix, int num_threads) 
	: name_prefix_(name_prefix),
	  num_threads_(num_threads),
	  lock_(),
	  empty_ev_(lock_),
	  full_ev_(lock_) {}

ThreadPool::~ThreadPool() {
	DCHECK(threads_.empty());
	DCHECK(taskers_.empty());
}

void ThreadPool::start() {
	DCHECK(threads_.empty() && running_ == false) 
		<< "start() called with outstanding threads.";
	running_ = true;

	// start all tasker thread
	threads_.reserve(num_threads_);
	for (int i = 0; i < num_threads_; ++i) {
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::thread_loop, this), 
										 name_prefix_));
		threads_[i]->start();
	}
	// current thread acts as a tasker thread 
	if (num_threads_ == 0 && thread_init_cb_) {
		thread_init_cb_();
	}
}

void ThreadPool::stop() {
	// tell all threads to quit their tasker loop.
	DCHECK(running_ == true) 
		<< "stop() called with no outstanding threads.";
	{
		AutoLock locked(lock_);
		running_ = false;
		empty_ev_.broadcast();
	}

	// join all the tasker threads.
	for (auto& td : threads_) {
		td->join();
	}
	threads_.clear();
	taskers_.clear();
}

void ThreadPool::join_all() {
	DCHECK(running_ == true) 
		<< "join_all() called with no outstanding threads.";

	// Tell all our threads to quit their worker loop.
	run_tasker(nullptr, num_threads_);

	// Join and destroy all the worker threads.
	for (auto& td : threads_) {
		td->join();
	}
	threads_.clear();
	DCHECK(taskers_.empty());
	running_ = false;
}

size_t ThreadPool::get_tasker_size() const {
	AutoLock locked(lock_);
	return taskers_.size();
}

bool ThreadPool::is_full() const {
	lock_.assert_acquired();
	return max_tasker_size_ >0 && taskers_.size() >=max_tasker_size_;
}

void ThreadPool::run_tasker(Tasker tasker, int repeat_count) {
	DCHECK(running_ == true) << 
		"run_tasker() called with no outstanding threads.";

	auto push_back = [&, repeat_count] (Tasker& tasker) {
		lock_.assert_acquired();
		if (repeat_count == 1) {
			taskers_.push_back(std::move(tasker));
		} else {
			taskers_.push_back(tasker);
		}
	};

	if (threads_.empty()) {
		while (repeat_count-- > 0) {
			if (tasker) {
				tasker();
			}
		}
	} else {
		AutoLock locked(lock_);
		while (repeat_count-- > 0) {
			while (is_full()) {
				full_ev_.wait();
			}
			DCHECK(!is_full()) << "full the taskers.";
			push_back(tasker);
		}
		empty_ev_.signal();
	}
}

void ThreadPool::thread_loop() {
	try {
		if (thread_init_cb_) {
			thread_init_cb_();
		}
		while (running_) {
			Tasker task = nullptr;
			{
				// Get one task. /FIFO/
				AutoLock locked(lock_);
				while (taskers_.empty() && running_) {
					empty_ev_.wait();
				}
				if (!running_) {
					break;
				}
				DCHECK(!taskers_.empty()) << "empty the taskers.";

				task = taskers_.front();
				taskers_.pop_front();
				if (max_tasker_size_ > 0) {
					full_ev_.signal();
				}
			}
			{
				// A nullptr std::function task signals us to quit.
				if (!task) {
					// Signal to any other threads that we're currently 
					// out of work.
					empty_ev_.signal();
					break;
				}
				// going to execute
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
