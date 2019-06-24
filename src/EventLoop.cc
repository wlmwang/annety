// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"
#include "Poller.h"
// #include "EPollPoller.h"
#include "PollPoller.h"
#include "Channel.h"

namespace annety
{
namespace
{
void clean_tls_eventloop(void *ptr) {
	LOG(TRACE) << "TLS EventLoop has clean by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", Deleted EventLoop " << ptr;
}

// one loop one thread
ThreadLocal<EventLoop> tls_eventloop{&clean_tls_eventloop};
const int kPollTimeoutMs = 10000;
}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_(new ThreadRef(PlatformThread::current_ref())),
	  poller_(new PollPoller(this))
{
	LOG(TRACE) << "EventLoop is creating by thread " 
		<< owning_thread_->ref() 
		<< ", EventLoop " << this;
	
	CHECK(tls_eventloop.empty()) << "EventLoop has created by thread " 
		<< owning_thread_->ref() << ", Now current thread is " 
		<< PlatformThread::current_ref().ref();

	tls_eventloop.set(this);
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
	check_in_own_loop(true);
	DCHECK(channel->owner_loop() == this);

	poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel)
{
	check_in_own_loop(true);
	DCHECK(channel->owner_loop() == this);

	poller_->remove_channel(channel);
}

bool EventLoop::has_channel(Channel* channel)
{
	check_in_own_loop(true);
	DCHECK(channel->owner_loop() == this);

	return poller_->has_channel(channel);
}

bool EventLoop::check_in_own_loop(bool fatal) const
{
	bool is_in_own_thread = *owning_thread_ == PlatformThread::current_ref();
	if (fatal) {
		CHECK(is_in_own_thread) << " EventLoop was created by thread " 
			<< owning_thread_->ref() << ", But current thread is " 
			<< PlatformThread::current_ref().ref();
	}
	return is_in_own_thread;
}

}	// namespace annety
