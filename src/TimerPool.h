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
#include <memory>
#include <functional>

namespace annety
{
class EventLoop;
class Channel;
class Timer;
class TimerId;

// timer pool, sorted by expired.
// no guarantee that the callback will be emitted on time.
class TimerPool
{
public:
	explicit TimerPool(EventLoop* loop);
	~TimerPool();
	
	// Must be thread safe. Usually be called from other threads.
	TimerId add_timer(TimerCallback cb, TimeStamp expired, double interval_s);
	void cancel_timer(TimerId timer_id);

#if !defined(OS_LINUX)
	void check_timer(TimeStamp expired);
#endif

private:
	// FIXME: use unique_ptr<Timer> instead of raw pointers.
	// This requires heterogeneous comparison lookup (N3465) from C++14

	// Timer list. sorted by expired
	using EntryTimer = std::pair<TimeStamp, Timer*>;
	using EntryTimerSet = std::set<EntryTimer>;
	using EntryTimerList = std::vector<EntryTimer>;

	// active timer list, sorted by TimeId. for cancel
	using ActiveTimer = std::pair<Timer*, int64_t>;
	using ActiveTimerSet = std::set<ActiveTimer>;

	void add_timer_in_own_loop(Timer* timer);
	void cancel_timer_in_own_loop(TimerId timer_id);

#if !defined(OS_LINUX)
	void wakeup();
	void check_timer_in_own_loop(TimeStamp expired);
#endif

	bool save(Timer* timer);
	void update(TimeStamp time, const EntryTimerList& expired_timers);
	void reset(TimeStamp expired);

	void fill_expired_timers(TimeStamp time, EntryTimerList& expired_timers);
	void handle_read();

private:
	EventLoop* owner_loop_{nullptr};

	SelectableFDPtr timer_socket_;
	std::unique_ptr<Channel> timer_channel_;

	// Timer list. sorted by expired
	EntryTimerSet timers_;

	bool calling_expired_timers_{false};

	// active timer list, sorted by TimeId. for cancel
	ActiveTimerSet active_timers_;
	ActiveTimerSet canceling_timers_;
};

}	// namespace annety

#endif	// ANT_TIMER_POOL_H_
