// Modify: Anny Wang
// Date: May 28 2019

#include "Thread.h"
#include "Logging.h"
#include "PlatformThread.h"
#include "CountDownLatch.h"
#include "StringPrintf.h"
#include "Exception.h"

namespace annety {
Thread::Thread(const ThreadMainFunc& func, const std::string& name_prefix)
	: Thread(func, name_prefix, Options()) {}

Thread::Thread(ThreadMainFunc&& func, const std::string& name_prefix)
	: Thread(std::move(func), name_prefix, Options()) {}

Thread::Thread(const ThreadMainFunc& func, const std::string& name_prefix,
			   const Options& options)
	: name_prefix_(name_prefix),
	  options_(options),
	  func_(func),
	  latch_(1) {}

Thread::Thread(ThreadMainFunc&& func, const std::string& name_prefix,
			   const Options& options)
	: name_prefix_(name_prefix),
	  options_(options),
	  func_(std::move(func)),
	  latch_(1) {}

Thread::~Thread() {
	DCHECK(has_been_started()) << "Thread was never started.";
	DCHECK(!options_.joinable || has_been_joined())
		<< "Joinable Thread destroyed without being Join()ed.";
}

void Thread::start() {
	start_async();
	// Wait for the thread to complete initialization.
	latch_.await();
	started_ = true;
}

void Thread::join() {
	DCHECK(options_.joinable) << "A non-joinable thread can't be joined.";
	DCHECK(has_start_been_attempted()) << "Tried to Join a never-started thread.";
	DCHECK(!has_been_joined()) << "Tried to Join a thread multiple times.";

	PlatformThread::join(thread_);
	thread_ = PlatformThreadHandle();
	joined_ = true;
}

void Thread::start_async() {
	DCHECK(!has_start_been_attempted()) << "Tried to Start a thread multiple times.";
	start_called_ = true;
	bool success = options_.joinable
		? PlatformThread::create(std::bind(&Thread::start_routine, this), &thread_)
		: PlatformThread::create_non_joinable(std::bind(&Thread::start_routine, this));
	CHECK(success);
}

PlatformThreadId Thread::tid() {
	DCHECK(has_been_started());
	return tid_;
}

void Thread::start_routine() {
	tid_ = PlatformThread::current_id();
	
	// Construct our full name of the form "name_prefix_/TID".
	name_ = string_printf("%s/%d", name_prefix_.c_str(), tid_);
	PlatformThread::set_name(name_);

	// We've initialized our new thread, signal that we're done to start().
	latch_.count_down();
	
	try {
		func_();
	} catch (const Exception& e) {
		LOG(FATAL) << "Thread:" << name_ 
				   << "Exception:" << e.what() 
				   << "Backtrace:" << e.backtrace();
	} catch (const std::exception& e) {
		LOG(FATAL) << "Thread:" << name_ 
				   << "Exception:" << e.what();
	} catch (...) {
		LOG(ERROR) << "Thread:" << name_;
		throw;
	}
}

}	// namespace annety
