// Refactoring: Anny Wang
// Date: Jun 25 2019

#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"

namespace annety
{
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
								 const std::string& name)
	: thread_(std::bind(&EventLoopThread::thread_func, this), name),
	  thread_init_cb_(cb) {}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;

	// not 100% race-free
	// eg. thread_func could be running thread_init_cb_.
	if (loop_ != nullptr) {
		// still a tiny chance to call destructed object, if thread_func exits just now.
		// but when EventLoopThread destructs, usually programming is exiting anyway.
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::start_loop()
{
	DCHECK(!thread_.has_been_started());
	thread_.start();

	EventLoop* loop = nullptr;
	{
		AutoLock locked(lock_);
		while (loop_ == nullptr) {
			cv_.wait();
		}
		loop = loop_;
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
		loop_ = &loop;
		cv_.signal();
	}

	loop.loop();

	// DCHECK(exiting_);
	{
		AutoLock locked(lock_);
		loop_ = nullptr;	
	}
}

}	// namespace annety