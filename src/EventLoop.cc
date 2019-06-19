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
// one thread one loop
void clean_tls_eventloop(void *ptr);
ThreadLocal<EventLoop> tls_eventloop{&clean_tls_eventloop};

void clean_tls_eventloop(void *ptr) {
	LOG(TRACE) << "TLS EventLoop has clean by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", EventLoop " << ptr;
}

const int kPollTimeMs = 10000;

}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_(PlatformThread::current_ref()),
	  poller_(new EPollPoller(this))
{
	LOG(TRACE) << "EventLoop is creating by thread " 
		<< owning_thread_.ref() 
		<< ", EventLoop " << this;
	
	CHECK(tls_eventloop.empty()) << " EventLoop has created by thread " 
		<< owning_thread_.ref() 
		<< ", Current thread " << PlatformThread::current_ref().ref();

	tls_eventloop.set(this);
}

EventLoop::~EventLoop()
{
	LOG(TRACE) << "~EventLoop is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", EventLoop " << this;
}

void EventLoop::loop()
{
	CHECK(!looping_);
	check_in_own_thread();

	looping_ = true;
	LOG(TRACE) << "EventLoop " << this << " start looping";

	while (!quit_) {
		active_channels_.clear();
		poll_tm_ = poller_->poll(kPollTimeMs, &active_channels_);

		event_handling_ = true;
		for (Channel* channel : active_channels_) {
			current_active_channel_ = channel;
			current_active_channel_->handle_event(poll_tm_);
		}
		current_active_channel_ = nullptr;
		event_handling_ = false;
	}

	LOG(TRACE) << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::update_channel(Channel* channel)
{
	DCHECK(channel->owner_loop() == this);
	check_in_own_thread();
	poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel)
{
	DCHECK(channel->owner_loop() == this);
	check_in_own_thread();

	// if (event_handling_) {
	// 	DCHECK(current_active_channel_ == channel ||
	// 		std::find(active_channels_.begin(), active_channels_.end(), channel) 
	// 				== active_channels_.end());
	// }
	poller_->remove_channel(channel);
}

bool EventLoop::has_channel(Channel* channel)
{
	DCHECK(channel->owner_loop() == this);
	check_in_own_thread();
	return poller_->has_channel(channel);
}

void EventLoop::check_in_own_thread() const
{
	CHECK(is_in_own_thread()) << " EventLoop was created by thread " 
		<< owning_thread_.ref()
		<< ", Current thread " << PlatformThread::current_ref().ref();
}

bool EventLoop::is_in_own_thread() const
{
	return owning_thread_ == PlatformThread::current_ref();
}

}	// namespace annety
