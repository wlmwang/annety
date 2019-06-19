// Refactoring: Anny Wang
// Date: May 29 2019

#include "ThreadPool.h"
#include "Logging.h"
#include "Exception.h"

#include <string>
#include <functional>

namespace annety
{
ThreadPool::ThreadPool(int num_threads, const std::string& name_prefix) 
	: name_prefix_(name_prefix),
	  num_threads_(num_threads),
	  lock_(),
	  empty_cv_(lock_),
	  full_cv_(lock_) {}

ThreadPool::~ThreadPool()
{
	DCHECK(threads_.empty());
	DCHECK(thread_main_cbs_.empty());
}

ThreadPool& ThreadPool::start()
{
	DCHECK(threads_.empty() && running_ == false) 
		<< "start() called with outstanding threads.";
	running_ = true;

	// start all tasker thread
	threads_.reserve(num_threads_);
	for (int i = 0; i < num_threads_; ++i) {
		threads_.emplace_back(new Thread(std::bind(&ThreadPool::loop, this), 
										 name_prefix_));
		threads_[i]->start();
	}
	// current thread acts as a tasker thread 
	if (num_threads_ == 0 && thread_init_cb_) {
		thread_init_cb_();
	}
	return *this;
}

void ThreadPool::stop()
{
	// tell all threads to quit their tasker loop.
	DCHECK(running_ == true) 
		<< "stop() called with no outstanding threads.";
	{
		AutoLock locked(lock_);
		running_ = false;
		empty_cv_.broadcast();
	}

	// join all the tasker threads.
	for (auto& td : threads_) {
		td->join();
	}
	threads_.clear();
	thread_main_cbs_.clear();
}

void ThreadPool::joinall()
{
	DCHECK(running_ == true) 
		<< "join_all() called with no outstanding threads.";

	// Tell all our threads to quit their worker loop.
	run_task(nullptr, num_threads_);

	// Join and destroy all the worker threads.
	for (auto& td : threads_) {
		td->join();
	}
	threads_.clear();
	DCHECK(thread_main_cbs_.empty());
	running_ = false;
}

size_t ThreadPool::get_task_size() const
{
	AutoLock locked(lock_);
	return thread_main_cbs_.size();
}

bool ThreadPool::full() const
{
	lock_.assert_acquired();
	return max_task_size_ > 0 && max_task_size_ <= thread_main_cbs_.size();
}

void ThreadPool::run_task(const TaskCallback& cb, int repeat_count)
{
	DCHECK(running_ == true) << 
		"run_task() called with no outstanding threads.";

	if (threads_.empty()) {
		while (repeat_count-- > 0) {
			if (cb) {
				cb();
			}
		}
	} else {
		AutoLock locked(lock_);
		while (repeat_count-- > 0) {
			while (full()) {
				full_cv_.wait();
			}
			DCHECK(!full()) << "full the thread_main_cbs_.";
			// copy to vector
			thread_main_cbs_.push_back(cb);
		}
		empty_cv_.signal();
	}
}

void ThreadPool::loop()
{
	try {
		if (thread_init_cb_) {
			thread_init_cb_();
		}
		while (running_) {
			TaskCallback task = nullptr;
			
			// get one task /FIFO/
			{
				AutoLock locked(lock_);
				while (thread_main_cbs_.empty() && running_) {
					empty_cv_.wait();
				}
				if (!running_) {
					break;
				}
				DCHECK(!thread_main_cbs_.empty()) << "empty the taskers.";

				task = thread_main_cbs_.front();
				thread_main_cbs_.pop_front();
				if (max_task_size_ > 0) {
					full_cv_.signal();
				}
			}
			
			// run a task
			{
				// A nullptr std::function task signals us to quit.
				if (!task) {
					// Signal to any other threads that we're currently 
					// out of work.
					empty_cv_.signal();
					break;
				}
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
