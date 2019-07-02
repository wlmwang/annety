// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "Time.h"
#include "MutexLock.h"
#include "CallbackForward.h"

#include <vector>
#include <memory>
#include <utility>
#include <functional>

namespace annety
{
class ThreadRef;
class Channel;
class Poller;

// Reactor, one loop per thread.
class EventLoop
{
public:
	using ChannelList = std::vector<Channel*>;
	using Functor = std::function<void()>;

	EventLoop();
	~EventLoop();
	
	// Must called in own loop thread
	void loop();

	// Not 100% thread safe if you call through a raw pointer,
	// better to call through shared_ptr<EventLoop> for 100% safety
	void quit();
	
	// Internal method---------------------------------

	// *Not thread safe* , but run in own loop thread aways.
	void update_channel(Channel* channel);
	void remove_channel(Channel* channel);
	bool has_channel(Channel *channel);

	// Run callback immediately in the own loop thread.
	// Maybe it wakeup the own loop thread and run the cb.
	// If in the own loop thread, cb is run within the call-function.
	// *Thread safe*
	void run_in_own_loop(Functor cb);

	// Queue callback in the own loop.
	// Run after finish looping.
	// *Thread safe*
	void queue_in_own_loop(Functor cb);
	
	void check_in_own_loop() const;
	bool is_in_own_loop() const;

private:
	// *Not thread safe* , but run in own loop thread aways.
	void handle_read();
	void do_calling_wakeup_functors();
	void wakeup();
	
	void print_active_channels() const;

private:
	// atomic
	bool looping_{false};
	bool event_handling_{false};
	uint64_t looping_times_{0};

	std::unique_ptr<ThreadRef> owning_thread_;
	std::unique_ptr<Poller> poller_;
	
	SelectableFDPtr wakeup_socket_;
	std::unique_ptr<Channel> wakeup_channel_;

	Time poll_tm_;
	ChannelList active_channels_;
	Channel* current_channel_{nullptr};
	
	// atomic
	bool calling_wakeup_functors_{false};

	mutable MutexLock lock_;
	std::vector<Functor> wakeup_functors_;

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
