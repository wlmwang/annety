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
#include <atomic>
#include <memory>
#include <utility>
#include <functional>

namespace annety
{
class ThreadRef;
class Channel;
class Poller;
class TimerPool;

// Reactor mode, Event dispatcher.
// It mainly acts as a combination of IO multiplexing and channels, and 
// controls the thread safety model.  One loop per thread: Each thread 
// has at most one EventLoop instance.
//
// Thread ipc: Other threads add a wakeup task function, and then wakeup 
// the own thread to execute it.
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
	void quit();

	// Timers method ---------------------------------

	// Runs callback at time seconds.
	// *Thread safe*
	TimerId run_at(double time_s, TimerCallback cb);
	TimerId run_at(TimeStamp time, TimerCallback cb);

	// Runs callback after delay seconds.
	// *Thread safe*
	TimerId run_after(double delay_s, TimerCallback cb);
	TimerId run_after(TimeDelta delta, TimerCallback cb);

	// Runs callback every interval seconds.
	// *Thread safe*
	TimerId run_every(double interval_s, TimerCallback cb);
	TimerId run_every(TimeDelta delta, TimerCallback cb);

	// Cancel the timer
	// *Thread safe*
	void cancel(TimerId timerId);

	// On non-timefd platform, control poll timeout to 
	// implement timer.
	// *Thread safe*
	void set_poll_timeout(int64_t time_ms = kPollTimeoutMs);
	
	// Channel method ---------------------------------
	
	// *Not thread safe*, but run in the own loop.
	void update_channel(Channel* channel);
	void remove_channel(Channel* channel);
	bool has_channel(Channel *channel);

	// Internal method ---------------------------------

	// 1. If in the own loop thread, cb is executed immediately,
	// 2. otherwise the own loop thread will be wake up, then 
	// 	  cb is executed async.
	// *Thread safe*
	void run_in_own_loop(Functor cb);

	// Queue cb in the own loop, run after finish looping.
	// *Thread safe*
	void queue_in_own_loop(Functor cb);
	
	// *Thread safe*
	void check_in_own_loop() const;
	bool is_in_own_loop() const;

private:
	// wakeup the own loop thread.
	// *Thread safe*
	void wakeup();
	void handle_read();

	// *Not thread safe*, but run in own loop thread.
	void do_calling_wakeup_functors();
	
	// *Not thread safe*, but run in own loop thread.
	void print_active_channels() const;

private:
	std::atomic<bool> quit_{false};
	std::atomic<bool> looping_{false};
	std::atomic<bool> handling_event_{false};
	std::atomic<bool> calling_wakeup_functors_{false};
	std::atomic<int64_t> looping_times_{0};
	std::atomic<int64_t> poll_timeout_ms_{kPollTimeoutMs};

	// The creation thread of EventLoop
	std::unique_ptr<ThreadId> owning_thread_id_;
	std::unique_ptr<ThreadRef> owning_thread_ref_;

	// IO Multiplexing.
	std::unique_ptr<Poller> poller_;
	
	// Timer pool.
	std::unique_ptr<TimerPool> timer_;

	// client active channel.
	TimeStamp poll_active_ms_;
	ChannelList active_channels_;

	// wakeup socket/channel.
	SelectableFDPtr wakeup_socket_;
	std::unique_ptr<Channel> wakeup_channel_;

	// wakeup task function.
	MutexLock lock_;
	std::vector<Functor> wakeup_functors_;

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
