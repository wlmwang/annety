// Refactoring: Anny Wang
// Date: Jun 25 2019

#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"
#include "PlatformThread.h"

namespace annety
{
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
								 const std::string& name)
	: thread_(std::bind(&EventLoopThread::thread_func, this), name)
	, thread_init_cb_(cb)
{
	LOG(DEBUG) << "EventLoopThread::EventLoopThread is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", name is " << thread_.name();
}

EventLoopThread::~EventLoopThread()
{
	LOG(DEBUG) << "EventLoopThread::~EventLoopThread is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", name is " << thread_.name();

	quit_loop();
}

void EventLoopThread::quit_loop()
{
	EventLoop* loop = nullptr;
	{
		AutoLock locked(lock_);
		loop = owning_loop_;	
	}

	LOG(DEBUG) << "EventLoopThread::quit_loop is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", EventLoop address is " << loop;
	
	// *Not 100% thread safe*
	// e.g. thread_func could be running thread_init_cb_
	if (loop) {
		// still a tiny chance to call destructed object, if thread_func exits just now.
		// but when EventLoopThread destructs, usually programming is exiting anyway.
		loop->quit();
	}
	thread_.join();

	exiting_ = true;
}

EventLoop* EventLoopThread::start_loop()
{
	DCHECK(!thread_.has_been_started());
	thread_.start();

	EventLoop* loop = nullptr;
	{
		AutoLock locked(lock_);
		while (!owning_loop_) {
			cv_.wait();
		}
		loop = owning_loop_;
	}

	return loop;
}

void EventLoopThread::thread_func()
{
	EventLoop loop;

	if (thread_init_cb_) {
		thread_init_cb_(&loop);
	}

	{
		AutoLock locked(lock_);
		owning_loop_ = &loop;
		cv_.signal();
	}

	loop.loop();
	DCHECK(!exiting_);

	{
		AutoLock locked(lock_);
		owning_loop_ = nullptr;	
	}
}

}	// namespace annety
