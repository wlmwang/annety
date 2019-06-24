// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"
#include "Channel.h"
#include "Poller.h"
#include "PollPoller.h"
// #include "EPollPoller.h"

#include <signal.h>

namespace annety
{
namespace
{
void clean_tls_event_loop(void *ptr) {
	LOG(TRACE) << "TLS EventLoop has clean by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", Deleted EventLoop " << ptr;
}

// There is at most one EventLoop object per thread
ThreadLocal<EventLoop> tls_event_loop{&clean_tls_event_loop};
const int kPollTimeoutMs = 10000;

// Ignore the SIGPIPE signal
BEFORE_MAIN_EXECUTOR() {
	::signal(SIGPIPE, SIG_IGN);
}

}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_(new ThreadRef(PlatformThread::current_ref())),
	  poller_(new PollPoller(this))
{
	LOG(TRACE) << "EventLoop is creating by thread " 
		<< owning_thread_->ref() 
		<< ", EventLoop " << this;
	
	CHECK(tls_event_loop.empty()) << "EventLoop has created by thread " 
		<< owning_thread_->ref() << ", Now current thread is " 
		<< PlatformThread::current_ref().ref();

	tls_event_loop.set(this);
}

EventLoop::~EventLoop()
{
	LOG(TRACE) << "~EventLoop is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", Deleted EventLoop " << this;
}

void EventLoop::loop()
{
	check_in_own_loop();

	CHECK(!looping_);
	looping_ = true;
	LOG(TRACE) << "EventLoop " << this << " begin looping";

	while (!quit_) {
		active_channels_.clear();
		poll_tm_ = poller_->poll(kPollTimeoutMs, &active_channels_);

		// handling event channels
		event_handling_ = true;
		for (Channel* ch : active_channels_) {
			current_channel_ = ch;
			current_channel_->handle_event(poll_tm_);
		}
		current_channel_ = nullptr;
		event_handling_ = false;
	}

	LOG(TRACE) << "EventLoop " << this << " finish looping";
	looping_ = false;
}

void EventLoop::update_channel(Channel* channel)
{
	check_in_own_loop();
	DCHECK(channel->owner_loop() == this);

	poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel)
{
	check_in_own_loop();
	DCHECK(channel->owner_loop() == this);

	poller_->remove_channel(channel);
}

bool EventLoop::has_channel(Channel* channel)
{
	check_in_own_loop();
	DCHECK(channel->owner_loop() == this);

	return poller_->has_channel(channel);
}

void EventLoop::check_in_own_loop() const
{
	CHECK(is_in_own_loop()) << " EventLoop was created by thread " 
		<< owning_thread_->ref() << ", But current thread is " 
		<< PlatformThread::current_ref().ref();
}

bool EventLoop::is_in_own_loop() const {
	return *owning_thread_ == PlatformThread::current_ref();
}

}	// namespace annety
