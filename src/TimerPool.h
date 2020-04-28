// By: wlmwang
// Date: Jul 04 2019

#ifndef ANT_TIMER_POOL_H_
#define ANT_TIMER_POOL_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "TimerId.h"
#include "CallbackForward.h"

#include <set>
#include <vector>
#include <utility>
#include <string>
#include <atomic>
#include <memory>
#include <functional>

namespace annety
{
class EventLoop;
class Channel;
class Timer;
class TimerId;

// Timer pool, all timers share a `timerfd`.
// Cannot be sure that the timer callback will be called on time, because 
// the `timerfd` event may be blocked by other events.
//
// We only use the one-shot wakeup of `timerfd`, and the repeat timer will 
// be reset when the timer timeout.
//
// This class owns the SelectableFD and Channel lifetime.
class TimerPool
{
public:
	explicit TimerPool(EventLoop* loop);
	~TimerPool();
	
	// Usually be called from other threads.
	// *Thread safe*
	TimerId add_timer(TimerCallback cb, TimeStamp expired, double interval_s);
	void cancel_timer(TimerId timer_id);

#if !defined(OS_LINUX)
	// On non-Linux platforms, Use the traditional poller timeout to implement 
	// the timers, and provide a timer check function.
	void check_timer(TimeStamp expired);
#endif

private:
	// FIXME: use unique_ptr<Timer> instead of raw pointers.
	// This requires heterogeneous comparison lookup (N3465) from C++14

	// The EntryTimer structure is used to add multiple timers with 
	// the same expiration time to the std::set.
	//
	// Timer list. sorted by expired first, then by pointer address.
	using EntryTimer = std::pair<TimeStamp, Timer*>;
	using EntryTimerSet = std::set<EntryTimer>;
	using EntryTimerList = std::vector<EntryTimer>;

	// The ActiveTimer structure is used to quickly find a timer from
	// the std::set using TimerId.
	//
	// Active list, sorted by pointer address first, then by sequence. 
	// for cancel(), the timer fast search.
	using ActiveTimer = std::pair<Timer*, int64_t>;
	using ActiveTimerSet = std::set<ActiveTimer>;

	// *Not thread safe*, but run in own loop thread.
	void add_timer_in_own_loop(Timer* timer);
	void cancel_timer_in_own_loop(TimerId timer_id);

	void fill_expired_timers(TimeStamp time, EntryTimerList& expired_timers);
	void handle_read();

#if !defined(OS_LINUX)
	// On non-Linux platforms, pipe will be used to simulate `timerfd`.
	void wakeup();
	void check_timer_in_own_loop(TimeStamp expired);
#endif
	
	// *Not thread safe*, but run in own loop thread.
	bool save(Timer* timer);
	void update(TimeStamp time, const EntryTimerList& expired_timers);
	void reset(TimeStamp expired);

private:
	EventLoop* owner_loop_{nullptr};
	std::atomic<bool> calling_expired_timers_{false};

	SelectableFDPtr timer_socket_;
	std::unique_ptr<Channel> timer_channel_;

	// Timer list. sorted by expired first, then by pointer address.
	EntryTimerSet timers_;

	// Active list, sorted by pointer address first, then by sequence. 
	// for cancel(), the timer fast search.
	ActiveTimerSet active_timers_;

	// canceling timers.
	// for cancel()
	ActiveTimerSet canceling_timers_;
};

}	// namespace annety

#endif	// ANT_TIMER_POOL_H_
