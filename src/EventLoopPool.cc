// Refactoring: Anny Wang
// Date: Jun 25 2019

#include "EventLoopPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"
#include "strings/StringPrintf.h"

#include <stdio.h>

namespace annety
{
EventLoopPool::EventLoopPool(EventLoop* loop, const std::string& name)
	: owner_loop_(loop), name_(name) {}

EventLoopPool::~EventLoopPool() {}

void EventLoopPool::start(const ThreadInitCallback& cb)
{
	owner_loop_->check_in_own_loop();

	DCHECK(!started_);
	started_ = true;

	for (int i = 0; i < num_threads_; ++i) {
		std::string name = name_ + string_printf("%d", i);
		
		EventLoopThread* et = new EventLoopThread(cb, name);
		
		threads_.push_back(std::unique_ptr<EventLoopThread>(et));
		loops_.push_back(et->start_loop());
	}
	// no thread pool, reuse current threads
	if (num_threads_ == 0 && cb) {
		cb(owner_loop_);
	}
}

EventLoop* EventLoopPool::get_next_loop()
{
	owner_loop_->check_in_own_loop();

	DCHECK(started_);
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

EventLoop* EventLoopPool::get_loop_with_hashcode(size_t hashcode)
{
	owner_loop_->check_in_own_loop();

	EventLoop* loop = owner_loop_;

	if (!loops_.empty()) {
		loop = loops_[hashcode % loops_.size()];
	}
	return loop;
}

std::vector<EventLoop*> EventLoopPool::get_all_loops()
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