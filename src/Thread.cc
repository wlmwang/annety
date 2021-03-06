// By: wlmwang
// Date: May 28 2019

#include "build/CompilerSpecific.h"
#include "FormatMacros.h"
#include "PlatformThread.h"
#include "threading/Thread.h"
#include "strings/StringPiece.h"
#include "strings/StringPrintf.h"
#include "Exceptions.h"
#include "Logging.h"

#include <string>
#include <utility>
#include <functional>

namespace annety
{
namespace threads {
// threading cache colums
thread_local static ThreadId tls_tid_int{kInvalidThreadId};
thread_local static char tls_tid_string[20]{'\0'};

ThreadId tid()
{
	if (UNLIKELY(tls_tid_int == kInvalidThreadId)) {
		tls_tid_int = PlatformThread::current_id();
	}
	return tls_tid_int;
}

StringPiece tid_string()
{
	if (UNLIKELY(tls_tid_string[0] == 0)) {
		sstring_printf(tls_tid_string, sizeof tls_tid_string, "%" PRId64, tid());
	}
	return tls_tid_string;
}

bool is_main_thread()
{
	return PlatformThread::is_main_thread();
}
}	// namespace threads

Thread::Thread(const TaskCallback& cb, const std::string& name_prefix)
	: Thread(cb, name_prefix, Options()) {}

Thread::Thread(TaskCallback&& cb, const std::string& name_prefix)
	: Thread(std::move(cb), name_prefix, Options()) {}

Thread::Thread(const TaskCallback& cb, const std::string& name_prefix,
			   const Options& options)
	: name_prefix_(name_prefix)
	, options_(options)
	, tid_(kInvalidThreadId)
	, thread_main_cb_(cb)
	, latch_(1) {}

Thread::Thread(TaskCallback&& cb, const std::string& name_prefix,
			   const Options& options)
	: name_prefix_(name_prefix)
	, options_(options)
	, tid_(kInvalidThreadId)
	, thread_main_cb_(std::move(cb))
	, latch_(1) {}

Thread::~Thread()
{
	CHECK(has_been_started()) << "Thread was never started.";
	CHECK(!options_.joinable || has_been_joined())
		<< "Joinable Thread destroyed without being Join()ed.";
}

Thread& Thread::start()
{
	start_async();
	
	// wait for the thread to complete initialization.
	latch_.wait();
	
	started_ = true;

	return *this;
}

Thread& Thread::start_async()
{
	CHECK(!has_start_been_attempted()) << "Tried to Start a thread multiple times.";
	start_called_ = true;

	bool success = options_.joinable
		? PlatformThread::create(std::bind(&Thread::start_routine, this), &ref_)
		: PlatformThread::create_non_joinable(std::bind(&Thread::start_routine, this));
	
	CHECK(success);
	
	return *this;
}

void Thread::join()
{
	CHECK(options_.joinable) << "A non-joinable thread can't be joined.";
	CHECK(!has_been_joined()) << "Tried to Join a thread multiple times.";
	CHECK(has_start_been_attempted()) << "Tried to Join a never-started thread.";

	PlatformThread::join(ref_);
	ref_ = ThreadRef();	// clear ref
	
	joined_ = true;
}

ThreadId Thread::tid()
{
	CHECK(has_been_started());
	return tid_;
}

ThreadRef Thread::ref()
{
	CHECK(has_been_started());
	return ref_;
}

void Thread::start_routine()
{
	tid_ = PlatformThread::current_id();
	
	// construct our full name of the form "name_prefix_/TID".
	name_ = string_printf("%s/%" PRId64, name_prefix_.c_str(), tid_);
	PlatformThread::set_name(name_);

	// we've initialized our new thread, signal that we're done to start().
	latch_.count_down();
	
	try {
		thread_main_cb_();
	} catch (const Exceptions& e) {
		LOG(FATAL) << "Thread:" << name_ 
				   << "Exceptions:" << e.what() 
				   << "Backtrace:" << e.backtrace();
	} catch (const std::exception& e) {
		LOG(FATAL) << "Thread:" << name_ 
				   << "Exceptions:" << e.what();
	} catch (...) {
		LOG(ERROR) << "Thread:" << name_;
		throw;
	}
}

}	// namespace annety
