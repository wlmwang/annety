// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "Time.h"
#include "ThreadForward.h"

#include <vector>
#include <memory>
#include <utility>

namespace annety
{
class Channel;
class Poller;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();
	
	void loop();

	// internal interface---------------------------------

	void update_channel(Channel* channel);
	void remove_channel(Channel* channel);
	bool has_channel(Channel *channel);

	void check_in_own_thread() const;
	bool is_in_own_thread() const;

private:
	using ChannelList = std::vector<Channel*>;

	bool quit_{false};
	bool looping_{false};
	bool event_handling_;

	ThreadRef owning_thread_;

	std::unique_ptr<Poller> poller_;
	
	// scratch variables
	Time poll_tm_;
	ChannelList active_channels_;
	Channel* current_active_channel_{nullptr};

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
