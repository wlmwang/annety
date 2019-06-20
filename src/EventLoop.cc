// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"
#include "Poller.h"
#include "EPollPoller.h"
#include "Channel.h"

namespace annety
{
namespace
{
const int kPollTimeoutMs = 10000;

void clean_tls_eventloop(void *ptr);

// one thread one loop
ThreadLocal<EventLoop> tls_eventloop{&clean_tls_eventloop};	// tls

void clean_tls_eventloop(void *ptr) {
	LOG(TRACE) << "TLS EventLoop has clean by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", Deleted EventLoop " << ptr;
}
}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_(new ThreadRef(PlatformThread::current_ref())),
	  poller_(new EPollPoller(this))
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
	check_in_own_thread(true);

	CHECK(!looping_);

	looping_ = true;
	LOG(TRACE) << "EventLoop " << this << " begin looping";

	while (!quit_) {
		active_channels_.clear();
		poll_tm_ = poller_->poll(kPollTimeoutMs, &active_channels_);

		// handling event channels
		event_handling_ = true;
		for (Channel* channel : active_channels_) {
			current_active_channel_ = channel;
			current_active_channel_->handle_event(poll_tm_);
		}
		current_active_channel_ = nullptr;
		event_handling_ = false;
	}

	LOG(TRACE) << "EventLoop " << this << " finish looping";
	looping_ = false;
}

void EventLoop::update_channel(Channel* channel)
{
	check_in_own_thread(true);
	DCHECK(channel->owner_loop() == this);

	poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel)
{
	check_in_own_thread(true);
	DCHECK(channel->owner_loop() == this);

	// if (event_handling_) {
	// 	DCHECK(current_active_channel_ == channel ||
	// 		std::find(active_channels_.begin(), active_channels_.end(), channel) 
	// 				== active_channels_.end());
	// }

	poller_->remove_channel(channel);
}

bool EventLoop::has_channel(Channel* channel)
{
	check_in_own_thread(true);
	DCHECK(channel->owner_loop() == this);

	return poller_->has_channel(channel);
}

bool EventLoop::check_in_own_thread(bool fatal) const
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
