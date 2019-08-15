// Refactoring: Anny Wang
// Date: Jun 25 2019

#ifndef ANT_EVENT_LOOP_THREAD_H_
#define ANT_EVENT_LOOP_THREAD_H_

#include "Macros.h"
#include "threading/Thread.h"
#include "synchronization/MutexLock.h"
#include "synchronization/ConditionVariable.h"

#include <functional>
#include <string>

namespace annety
{
class EventLoop;

class EventLoopThread
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
					const std::string& name = std::string());

	~EventLoopThread();

	void quit_loop();
	
	EventLoop* start_loop();

private:
	void thread_func();

private:
	EventLoop* owning_loop_{nullptr};
	bool exiting_{false};

	MutexLock lock_;
	ConditionVariable cv_{lock_};
	Thread thread_;

	ThreadInitCallback thread_init_cb_;

	DISALLOW_COPY_AND_ASSIGN(EventLoopThread);
};

}	// namespace annety

#endif	//	ANT_EVENT_LOOP_THREAD_H_
