// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "TimerId.h"
#include "CallbackForward.h"
#include "synchronization/MutexLock.h"

#include <vector>
#include <memory>
#include <utility>
#include <functional>

namespace annety
{
class ThreadRef;
class Channel;
class Poller;
class TimerPool;

// Event dispatcher(Reactor mode implementation), one loop per thread.
//
// Usually, it is a stack instance in the main() function.
// It may also be created in the EventLoopThread class.
class EventLoop
{
public:
	using ChannelList = std::vector<Channel*>;
	using Functor = std::function<void()>;

	static const int kPollTimeoutMs = 30*1000; // -1

	EventLoop();
	~EventLoop();
	
	// One loop per thread.
	// *Not thread safe*, but run in the own loop.
	void loop();

	// When you call with a native pointer, there may be a 
	// segmentation fault, better to call through shared_ptr 
	// for 100% safety.
	// *Not 100% thread safe*
	void terminate();
	void quit();

	// Timers method ---------------------------------

	// Runs cb at time seconds.
	// *Thread safe*
	TimerId run_at(double time_s, TimerCallback cb);
	TimerId run_at(TimeStamp time, TimerCallback cb);

	// Runs cb after delay seconds.
	// *Thread safe*
	TimerId run_after(double delay_s, TimerCallback cb);
	TimerId run_after(TimeDelta delta, TimerCallback cb);

	// Runs cb every interval seconds.
	// *Thread safe*
	TimerId run_every(double interval_s, TimerCallback cb);
	TimerId run_every(TimeDelta delta, TimerCallback cb);

	// Cancel the timer
	// *Thread safe*
	void cancel(TimerId timerId);

	// On non-timefd platform, control poll timeout to 
	// implement timer.
	void set_poll_timeout(int64_t ms = kPollTimeoutMs);
	
	// Channel method ---------------------------------
	
	// *Not thread safe*, but run in the own loop.
	void update_channel(Channel* channel);
	void remove_channel(Channel* channel);
	bool has_channel(Channel *channel);

	// Internal method ---------------------------------

	// 1. If in the own loop thread, cb is executed immediately,
	// 2. otherwise the own loop thread will be wake up, then 
	// cb is executed async.
	// *Thread safe*
	void run_in_own_loop(Functor cb);

	// Queue cb in the own loop, run after finish looping.
	// *Thread safe*
	void queue_in_own_loop(Functor cb);
	
	// *Thread safe*
	void check_in_own_loop() const;
	bool is_in_own_loop() const;

private:
	// wake up the own loop thread.
	// *Thread safe*
	void wakeup();

	// *Not thread safe*, but run in own loop thread aways.
	void handle_read();
	void do_calling_wakeup_functors();
	
	// *Not thread safe*, but run in own loop thread aways.
	void print_active_channels() const;

private:
	// atomic
	bool quit_{false};
	bool looping_{false};
	uint64_t looping_times_{0};
	bool event_handling_{false};
	int32_t poll_timeout_ms_{-1};
	// atomic
	bool calling_wakeup_functors_{false};
	
	// create EventLoop threadref
	std::unique_ptr<ThreadRef> owning_thread_;

	std::unique_ptr<Poller> poller_;
	std::unique_ptr<TimerPool> timer_pool_;

	SelectableFDPtr wakeup_socket_;
	std::unique_ptr<Channel> wakeup_channel_;

	TimeStamp poll_tm_;
	ChannelList active_channels_;
	Channel* current_channel_{nullptr};

	mutable MutexLock lock_;
	std::vector<Functor> wakeup_functors_;

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
