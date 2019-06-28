// Refactoring: Anny Wang
// Date: Jun 25 2019

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"
#include "StringPrintf.h"

#include <stdio.h>

namespace annety
{
EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop, const std::string& name)
	: owner_loop_(loop), name_(name) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	owner_loop_->check_in_own_loop();

	DCHECK(!started_);
	started_ = true;

	for (int i = 0; i < num_threads_; ++i) {
		std::string name = name_ + string_printf("%d", i);

		EventLoopThread* t = new EventLoopThread(cb, name);
		threads_.push_back(std::unique_ptr<EventLoopThread>(t));
		loops_.push_back(t->start_loop());
	}
	if (num_threads_ == 0 && cb) {
		cb(owner_loop_);
	}
}

EventLoop* EventLoopThreadPool::get_next_loop()
{
	owner_loop_->check_in_own_loop();

	DCHECK(!started_);
	EventLoop* loop = owner_loop_;

	// round-robin
	if (!loops_.empty()) {
		loop = loops_[next_++];
		if (static_cast<size_t>(next_) >= loops_.size()) {
			next_ = 0;
		}
	}
	return loop;
}

EventLoop* EventLoopThreadPool::get_loop_with_hashcode(size_t hashcode)
{
	owner_loop_->check_in_own_loop();

	EventLoop* loop = owner_loop_;

	if (!loops_.empty()) {
		loop = loops_[hashcode % loops_.size()];
	}
	return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::get_all_loops()
{
	owner_loop_->check_in_own_loop();
	DCHECK(started_);

	if (loops_.empty()) {
		return std::vector<EventLoop*>(1, owner_loop_);
	} else {
		return loops_;
	}
}

}	// namespace annety
