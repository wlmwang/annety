// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "Time.h"
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
using ChannelList = std::vector<Channel*>;

public:
	EventLoop();
	~EventLoop();
	
	void loop();

	void quit();
	
	// internal ---------------------------------
	void wakeup();
	
	void update_channel(Channel* channel);
	void remove_channel(Channel* channel);
	bool has_channel(Channel *channel);

	void check_in_own_loop() const;
	bool is_in_own_loop() const;

private:
	void handle_read();

private:
	bool quit_{false};
	bool looping_{false};
	bool event_handling_{false};

	std::unique_ptr<ThreadRef> owning_thread_;
	std::unique_ptr<Poller> poller_;
	
	SelectableFDPtr wakeup_socket_;
	std::unique_ptr<Channel> wakeup_channel_;

	Time poll_tm_;
	ChannelList active_channels_;
	Channel* current_channel_{nullptr};

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
