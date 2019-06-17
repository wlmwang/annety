// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"

namespace annety {
static thread_local EventLoop* tls_event_loop{nullptr};

EventLoop::EventLoop() :
	looping_(false),
	owning_thread_ref_(PlatformThread::current_ref())
{
	LOG(INFO) << "EventLoop created " << this 
			<< " in thread " << owning_thread_ref_.ref();
	
	CHECK(!tls_event_loop);
	tls_event_loop = this;
}

EventLoop::~EventLoop() {
	LOG(INFO) << "~EventLoop";
}

void EventLoop::loop() {
	//
}

void EventLoop::check_in_owning_thread() const {
	CHECK(is_in_owning_thread());
}

bool EventLoop::is_in_owning_thread() const {
	return owning_thread_ref_ == PlatformThread::current_ref();
}

}	// namespace annety
