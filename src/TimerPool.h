// Refactoring: Anny Wang
// Date: Jul 04 2019

#ifndef ANT_TIMER_POOL_H_
#define ANT_TIMER_POOL_H_

#include "Macros.h"
#include "Time.h"
#include "TimerId.h"
#include "CallbackForward.h"

#include <string>
#include <set>
#include <vector>
#include <memory>
#include <functional>

namespace annety
{
class EventLoop;
class Channel;
class Timer;
class TimerId;

class TimerPool
{
public:
	explicit TimerPool(EventLoop* loop);
	~TimerPool();

	TimerId add_timer(TimerCallback cb, Time when, double interval);
	void cancel_timer(TimerId timer_id);

private:
	using Entry = std::pair<Time, Timer*>;	// <expired:timer*>
	using TimerList = std::set<Entry>;
	
	// TimeId. for cancel
	using ActiveTimer = std::pair<Timer*, int64_t>;
	using ActiveTimerSet = std::set<ActiveTimer>;

	void add_timer_in_own_loop(Timer* timer);
	void cancel_timer_in_own_loop(TimerId timer_id);

	bool save(Timer* timer);
	void reset(Time tm, const std::vector<Entry>& expired_timers);

	void fill_expired_timers(Time now, std::vector<Entry>& timers);
	void handle_read();

private:
	EventLoop* owner_loop_{nullptr};

	SelectableFDPtr timer_socket_;
	std::unique_ptr<Channel> timer_channel_;

	// Timer list. sorted by expired
	TimerList timers_;

	// TimeId list. for cancel
	bool calling_expired_timers_{false};
	ActiveTimerSet canceling_timers_;
	ActiveTimerSet active_timers_;
};

}	// namespace annety

#endif	// ANT_TIMER_POOL_H_
