// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"
#include "EventFD.h"
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
const int kPollTimeoutMs = 30000;

// Ignore SIGPIPE signal
BEFORE_MAIN_EXECUTOR() {
	::signal(SIGPIPE, SIG_IGN);
}

}	// namespace anonymous

EventLoop::EventLoop() 
	: owning_thread_(new ThreadRef(PlatformThread::current_ref())),
	  poller_(new PollPoller(this)),
	  wakeup_socket_(new EventFD(true, true)),
	  wakeup_channel_(new Channel(this, wakeup_socket_.get()))
{
	LOG(INFO) << "EventLoop::EventLoop is creating by thread " 
		<< owning_thread_->ref() 
		<< ", EventLoop " << this;
	
	CHECK(tls_event_loop.empty()) << "EventLoop::EventLoop has created by thread " 
		<< owning_thread_->ref() << ", now current thread is " 
		<< PlatformThread::current_ref().ref();

	tls_event_loop.set(this);

	wakeup_channel_->set_read_callback(
		std::bind(&EventLoop::handle_read, this));
	
	wakeup_channel_->enable_read_event();
}

EventLoop::~EventLoop()
{
	CHECK(looping_ == false);
	
	LOG(TRACE) << "EventLoop::~EventLoop is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", deleted EventLoop " << this;
}

void EventLoop::loop()
{
	check_in_own_loop();

	CHECK(!looping_);
	looping_ = true;
	LOG(TRACE) << "EventLoop::loop " << this << " is begin looping";

	while (looping_) {
		active_channels_.clear();
		poll_tm_ = poller_->poll(kPollTimeoutMs, &active_channels_);

		if (LOG_IS_ON(TRACE)) {
			print_active_channels();
		}
		looping_times_++;
		
		// handling event channels
		event_handling_ = true;
		for (Channel* ch : active_channels_) {
			current_channel_ = ch;
			current_channel_->handle_event(poll_tm_);
		}
		current_channel_ = nullptr;
		event_handling_ = false;
		
		// wakeup and run queue functions
		do_calling_wakeup_functors();
	}

	LOG(TRACE) << "EventLoop::loop " << this << " has finish looping";
	looping_ = false;
}

void EventLoop::quit()
{
	looping_ = false;
	if (!is_in_own_loop()) {
		wakeup();
	}
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = wakeup_socket_->write(&one, sizeof one);
	if (n != sizeof one) {
		PLOG(ERROR) << "EventLoop::wakeup writes " << n << " bytes instead of 8";
	}
	LOG(TRACE) << "EventLoop::wakeup";
}

void EventLoop::handle_read()
{
	uint64_t one = 1;
	ssize_t n = wakeup_socket_->read(&one, sizeof one);
	if (n != sizeof one) {
		PLOG(ERROR) << "EventLoop::handle_read reads " << n << " bytes instead of 8";
	}
	LOG(TRACE) << "EventLoop::handle_read";
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

	if (!is_in_own_loop() || calling_wakeup_functors_) {
		wakeup();
	}
}

void EventLoop::check_in_own_loop() const
{
	CHECK(is_in_own_loop()) << " EventLoop::check_in_own_loop was created by thread " 
		<< owning_thread_->ref() << ", but current thread is " 
		<< PlatformThread::current_ref().ref();
}

bool EventLoop::is_in_own_loop() const
{
	return *owning_thread_ == PlatformThread::current_ref();
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

void EventLoop::print_active_channels() const
{
	for (const Channel* ch : active_channels_) {
		LOG(TRACE) << "EventLoop::print_active_channels {" << ch->revents_to_string() << "}";
	}
}

}	// namespace annety
