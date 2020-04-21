// By: wlmwang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "EventFD.h"
#include "Channel.h"
#include "Poller.h"
#include "PollPoller.h"
#include "EPollPoller.h"
#include "TimerPool.h"
#include "PlatformThread.h"

#include <signal.h>	// signal

namespace annety
{
// TCP is a full-duplex channel. When close() is called by the peer, even 
// if it is intended to close the two channels, the local will still only 
// receive FIN packet. Because according to the semantics of TCP protocol, 
// the local may think that the peer only close the write channel, the read 
// channel did not close.
// In other words, due to the limitations of the TCP protocol, an endpoint 
// cannot know that the peer has been completely closed.
//
// If you call read() on a socket that has received a FIN packet, if the receive 
// buffer is empty, it returns 0, which is normal connection close.
// But when you first call write() on it, if the send buffer is ok, will return 
// the correct write. However, the send packet will cause the peer to send a RST 
// packet. After receiving the RST, if write() is called a second time, a SIGPIPE 
// signal will be generated, causing the process to exit.
//
// In short, when we receive a FIN packet, we can still write(), which is a 
// limitation of the TCP protocol. However, if we write() twice at this time, 
// it is very likely to cause SIGPIPE signal (although it causes RST for the 
// first time, but the kernel returns correctly if the send buffer is ok).
BEFORE_MAIN_EXECUTOR() { ::signal(SIGPIPE, SIG_IGN);}

namespace {
thread_local EventLoop* tls_event_loop = nullptr;
}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_id_(new ThreadId(PlatformThread::current_id()))
	, owning_thread_ref_(new ThreadRef(PlatformThread::current_ref()))
	, poller_(new PollPoller(this))
	, timers_(new TimerPool(this))
	, wakeup_socket_(new EventFD(true, true))
	, wakeup_channel_(new Channel(this, wakeup_socket_.get()))
{
	LOG(DEBUG) << "EventLoop::EventLoop is creating by thread " 
		<< owning_thread_id_.get() 
		<< ", EventLoop address is " << this;

	{
		CHECK(!tls_event_loop) << "EventLoop::EventLoop has been created by thread " 
			<< owning_thread_id_.get() << ", now current thread is " 
			<< PlatformThread::current_id();
		tls_event_loop = this;
	}

	// Thread ipc: EventLoop is unlocked! Other threads add a wakeup task, 
	// and then system will wakeup the own thread to execute it.
	wakeup_channel_->set_read_callback(
		std::bind(&EventLoop::handle_read, this));
	wakeup_channel_->enable_read_event();
}

EventLoop::~EventLoop()
{
	CHECK(!looping_);
	
	LOG(DEBUG) << "EventLoop::~EventLoop is called by thread " 
		<< owning_thread_id_.get() << ", now current thread is " 
		<< PlatformThread::current_id()
		<< ", deleted EventLoop address is " << this;

	wakeup_channel_->disable_all_event();
	wakeup_channel_->remove();

	tls_event_loop = nullptr;
}

void EventLoop::quit()
{
	quit_ = true;
	if (!is_in_own_loop()) {
		wakeup();
	}
}

void EventLoop::loop()
{
	check_in_own_loop();

	DCHECK(!looping_);
	looping_ = true;

	while (!quit_) {
		DLOG(TRACE) << "EventLoop::loop timeout " << poll_timeout_ms_ << "ms";

		active_channels_.clear();
		poll_active_ms_ = poller_->poll(poll_timeout_ms_, &active_channels_);
		
		if (LOG_IS_ON(TRACE)) {
			print_active_channels();
		}
		
		looping_times_++;

		// Handling active event channels.
		handling_event_ = true;
		for (Channel* channel : active_channels_) {
			channel->handle_event(poll_active_ms_);
		}
		handling_event_ = false;

#if !defined(OS_LINUX)
		// On non-Linux platforms, Use the traditional poller timeout to implement 
		// the timers, here you need to manually check the timeout timers.
		timers_->check_timer(TimeStamp::now());
#endif	// !defined(OS_LINUX)

		// wakeup and run queue functions.
		do_calling_wakeup_functors();
	}

	looping_ = false;
}

TimerId EventLoop::run_at(double time_s, TimerCallback cb)
{
	return run_at(TimeStamp()+TimeDelta::from_seconds_d(time_s), std::move(cb));
}
TimerId EventLoop::run_at(TimeStamp time, TimerCallback cb)
{
	return timers_->add_timer(std::move(cb), time, 0.0);
}

TimerId EventLoop::run_after(double delay_s, TimerCallback cb)
{
	return run_after(TimeDelta::from_seconds_d(delay_s), std::move(cb));
}
TimerId EventLoop::run_after(TimeDelta delta, TimerCallback cb)
{
	return run_at(TimeStamp::now()+delta, std::move(cb));
}

TimerId EventLoop::run_every(double interval_s, TimerCallback cb)
{
	return run_every(TimeDelta::from_seconds_d(interval_s), std::move(cb));
}
TimerId EventLoop::run_every(TimeDelta delta, TimerCallback cb)
{
	return timers_->add_timer(std::move(cb), TimeStamp::now()+delta, delta.in_seconds_f());
}

void EventLoop::cancel(TimerId timerId)
{
	return timers_->cancel_timer(timerId);
}

void EventLoop::set_poll_timeout(int64_t time_ms)
{
	poll_timeout_ms_ = time_ms;
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

void EventLoop::run_in_own_loop(Functor cb)
{
	if (is_in_own_loop()) {
		cb();
	} else {
		queue_in_own_loop(std::move(cb));
	}
}

void EventLoop::queue_in_own_loop(Functor cb)
{
	{
		AutoLock locked(lock_);
		wakeup_functors_.push_back(std::move(cb));
	}

	// wakeup the own loop thread:
	// 1. current thread is not the own loop thread.
	// 2. the poller has returned (Executing wakeup function).
	if (!is_in_own_loop() || calling_wakeup_functors_) {
		wakeup();
	}
}

void EventLoop::check_in_own_loop() const
{
	CHECK(is_in_own_loop()) << " EventLoop::check_in_own_loop was created by thread " 
		<< owning_thread_id_.get() << ", but current calling thread is " 
		<< PlatformThread::current_id()
		<< ", EventLoop address is " << this;
}

bool EventLoop::is_in_own_loop() const
{
	return *owning_thread_ref_ == PlatformThread::current_ref();
}

void EventLoop::do_calling_wakeup_functors()
{
	calling_wakeup_functors_ = true;
	
	std::vector<Functor> functors;
	{
		AutoLock locked(lock_);
		functors.swap(wakeup_functors_);
	}
	for (const Functor& func : functors) {
		func();
	}

	calling_wakeup_functors_ = false;
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = wakeup_socket_->write(&one, sizeof one);
	if (n != sizeof one) {
		PLOG(ERROR) << "EventLoop::wakeup writes " << n << " bytes instead of 8";
		return;
	}
	DLOG(TRACE) << "EventLoop::wakeup is called";
}

void EventLoop::handle_read()
{
	uint64_t one;
	ssize_t n = wakeup_socket_->read(&one, sizeof one);
	if (n != sizeof one) {
		PLOG(ERROR) << "EventLoop::handle_read reads " << n << " bytes instead of 8";
		return;
	}
	DLOG(TRACE) << "EventLoop::handle_read is called";
}

void EventLoop::print_active_channels() const
{
	check_in_own_loop();
	
	for (const Channel* channel : active_channels_) {
		DLOG(TRACE) << "EventLoop::print_active_channels {" 
			<< channel->revents_to_string() << "}";
	}
}

}	// namespace annety
