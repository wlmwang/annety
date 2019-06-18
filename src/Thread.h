// Refactoring: Anny Wang
// Date: May 28 2019

#ifndef ANT_THREAD_H_
#define ANT_THREAD_H_

#include "ThreadForward.h"
#include "CountDownLatch.h"

#include <string>
#include <functional>

namespace annety {
class Thread {
public:
	typedef std::function<void()> ThreadMainCallback;

	struct Options {
	public:
		Options() = default;
		~Options() = default;

		// Allow copies.
		Options(const Options& other) = default;
		Options& operator=(const Options& other) = default;

		// If false, the underlying thread's PlatformThreadHandle will not be kept
		// around and as such the SimpleThread instance will not be Join()able and
		// must not be deleted before Run() is invoked. After that, it's up to
		// the subclass to determine when it is safe to delete itself.
		bool joinable = true;
	};

	explicit Thread(const ThreadMainCallback& cb, const std::string& name_prefix = "");
	explicit Thread(ThreadMainCallback&& cb, const std::string& name_prefix = "");
	
  	Thread(const ThreadMainCallback& cb, const std::string& name_prefix, const Options& options);
  	Thread(ThreadMainCallback&& cb, const std::string& name_prefix, const Options& options);

	~Thread();

	// Starts the thread and returns only after the thread has started and
	// initialized (i.e. ThreadMainCallback() has been called).
	Thread& start();

	// Starts the thread, but returns immediately, without waiting for the thread
	// to have initialized first (i.e. this does not wait for ThreadMainCallback() to have
	// been run first).
	Thread& start_async();
	
	// Joins the thread. If start_async() was used to start the thread, then this
	// first waits for the thread to start cleanly, then it joins.
	void join();

	// Returns the thread id, only valid after the thread has started. If the
	// thread was started using start(), then this will be valid after the call to
	// start(). If start_async() was used to start the thread, then this must not
	// be called before has_been_started() returns True.
	ThreadId tid();

	// pthread_self
	ThreadRef ref();

	// Returns True if the thread has been started and initialized (i.e. if
	// ThreadMainCallback() has run). If the thread was started with start_async(), but it
	// hasn't been initialized yet (i.e. ThreadMainCallback() has not run), then this will
	// return False.
	bool has_been_started() const {
		return started_;
	}

	// Returns True if join() has ever been called.
	bool has_been_joined() const {
		return joined_;
	}
	
	// Returns true if start() or start_async() has been called.
	bool has_start_been_attempted() const {
		return start_called_;
	}

private:
	void start_routine();

private:
	const std::string name_prefix_;
	std::string name_;
	const Options options_;
	ThreadRef ref_;	// PlatformThread handle, reset after Join.
	ThreadId tid_ = kInvalidThreadId;	// The backing thread's id.
	bool joined_ = false;	// True if Join has been called.
	// Set to true when the platform-thread creation has started.
	bool start_called_ = false;
	bool started_ = false;
	ThreadMainCallback cb_;
	CountDownLatch latch_;
};

}	// namespace annety

#endif	// ANT_THREAD_H_
