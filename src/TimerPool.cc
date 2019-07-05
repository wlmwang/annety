// Refactoring: Anny Wang
// Date: Jul 05 2019

#include "TimerPool.h"
#include "Logging.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TimerFD.h"
#include "Timer.h"

#include <algorithm>
#include <iostream>

namespace annety
{
TimerPool::TimerPool(EventLoop* loop)
	: owner_loop_(loop),
	  timer_socket_(new TimerFD(true, true)),
	  timer_channel_(new Channel(owner_loop_, timer_socket_.get()))
{
	LOG(TRACE) << "TimerPool::TimerPool" << " fd=" << 
		timer_socket_->internal_fd() << " is constructing";

	timer_channel_->set_read_callback(
		std::bind(&TimerPool::handle_read, this));

	timer_channel_->enable_read_event();
}

TimerPool::~TimerPool()
{
	timer_channel_->disable_all_event();
	timer_channel_->remove();
	
	// todo
	for (const Entry& timer : timers_) {
		delete timer.second;
	}
}

TimerId TimerPool::add_timer(TimerCallback cb, Time when, double interval)
{
	Timer* timer = new Timer(std::move(cb), when, interval);
	owner_loop_->run_in_own_loop(
		std::bind(&TimerPool::add_timer_in_own_loop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerPool::cancel_timer(TimerId timer_id)
{
	owner_loop_->run_in_own_loop(
		std::bind(&TimerPool::cancel_timer_in_own_loop, this, timer_id));
}

void TimerPool::add_timer_in_own_loop(Timer* timer)
{
	owner_loop_->check_in_own_loop();
	DCHECK(timers_.size() == active_timers_.size());

	bool earliest_changed = save(timer);
	if (earliest_changed) {
		// todo
		// resetTimerfd(timerfd_, timer->expired());

		// FIXME: implicit_cast<>
		TimerFD* ts = static_cast<TimerFD*>(timer_socket_.get());
		std::cout << timer->expired() << "|" << Time::now() << std::endl;

		ts->reset(timer->expired() - Time::now());
	}
}

void TimerPool::cancel_timer_in_own_loop(TimerId timer_id)
{
	owner_loop_->check_in_own_loop();
	DCHECK(timers_.size() == active_timers_.size());

	ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
	ActiveTimerSet::iterator it = active_timers_.find(timer);
	if (it != active_timers_.end()) {
		size_t n = timers_.erase(Entry(it->first->expired(), it->first));
		DCHECK(n == 1);

		// FIXME: no delete please
		delete it->first;

		active_timers_.erase(it);
		// DCHECK

	} else if (calling_expired_timers_) {
		canceling_timers_.insert(timer);
	}
	DCHECK(timers_.size() == active_timers_.size());
}

void TimerPool::handle_read()
{
	owner_loop_->check_in_own_loop();

	Time curr = Time::now();
	{
		uint64_t one;
		ssize_t n = timer_socket_->read(&one, sizeof one);
		if (n != sizeof one) {
			PLOG(ERROR) << "TimerQueue::handle_read reads " << n << " bytes instead of 8";
		}
		LOG(TRACE) << "TimerQueue::handle_read() " << one << " at " ;//<< curr;
	}

	{
		// do_calling...
		// activing_timers
		std::vector<Entry> expired_timers;//= get_expired(curr);
		fill_expired_timers(curr, expired_timers);

		calling_expired_timers_ = true;
		canceling_timers_.clear();

		// safe to callback outside critical section
		for (const Entry& it : expired_timers) {
			it.second->run();
		}
		calling_expired_timers_ = false;

		// delete or update expired
		reset(curr, expired_timers);
	}
}

void TimerPool::fill_expired_timers(Time tm, std::vector<Entry>& timers)
{
	owner_loop_->check_in_own_loop();
	DCHECK(timers_.size() == active_timers_.size());

	Entry sentry(tm, reinterpret_cast<Timer*>(UINTPTR_MAX));
	TimerList::iterator it = timers_.lower_bound(sentry);
	DCHECK(it == timers_.end() || it->first > tm);

	std::copy(timers_.begin(), it, std::back_inserter(timers));
	
	timers_.erase(timers_.begin(), it);
	for (const Entry& it : timers) {
		ActiveTimer timer(it.second, it.second->sequence());
		size_t n = active_timers_.erase(timer);
		DCHECK(n == 1);
	}
	DCHECK(timers_.size() == active_timers_.size());
}

void TimerPool::reset(Time tm, const std::vector<Entry>& expired_timers)
{
	Time next_expired;

	for (const Entry& it : expired_timers) {
		ActiveTimer timer(it.second, it.second->sequence());
		if (it.second->repeat() && 
			canceling_timers_.find(timer) == canceling_timers_.end())
		{
			it.second->restart(tm);
			save(it.second);
		} else {
			// FIXME move to a free list
			// FIXME: no delete please
			delete it.second;
		}
	}

	if (!timers_.empty()) {
		next_expired = timers_.begin()->second->expired();
	}

	if (next_expired.is_valid()) {
		// FIXME: implicit_cast<>
		TimerFD* ts = static_cast<TimerFD*>(timer_socket_.get());
		ts->reset(next_expired - Time::now());

		// resetTimerfd(timerfd_, next_expired);
	}
}

bool TimerPool::save(Timer* timer)
{
	owner_loop_->check_in_own_loop();
	DCHECK(timers_.size() == active_timers_.size());

	Time when = timer->expired();

	bool earliest_changed = false;
	if (timers_.empty() || timers_.begin()->first > when) {
		earliest_changed = true;
	}

	{
		// std::pair<TimerList::iterator, bool> result;
		auto result = timers_.insert(Entry(when, timer));
		DCHECK(result.second);
	}

	{
		// std::pair<ActiveTimerSet::iterator, bool> result;
		auto result = active_timers_.insert(ActiveTimer(timer, timer->sequence()));
		DCHECK(result.second);
	}

	DCHECK(timers_.size() == active_timers_.size());
	return earliest_changed;
}

}	// namespace annety
