// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_POLLER_H_
#define ANT_POLLER_H_

#include "Macros.h"
#include "Time.h"
#include "EventLoop.h"

#include <vector>
#include <map>

namespace annety
{
class Channel;

class Poller
{
public:
	using ChannelList = std::vector<Channel*>;
	using ChannelMap = std::map<int, Channel*>;

	Poller(EventLoop* loop);
	virtual ~Poller();

	// Polls the I/O events.
	// Must be called in the loop thread.
	virtual Time poll(int timeout_ms, ChannelList* activeChannels) = 0;

	// Changes the interested I/O events.
	// Must be called in the loop thread.
	virtual void update_channel(Channel* channel) = 0;

	// Remove the channel, when it destructs.
	// Must be called in the loop thread.
	virtual void remove_channel(Channel* channel) = 0;

	virtual bool has_channel(Channel* channel) const;

	void check_in_own_loop() const
	{
		owner_loop_->check_in_own_loop(true);
	}

protected:
	ChannelMap channels_;

private:
	EventLoop* owner_loop_;

	DISALLOW_COPY_AND_ASSIGN(Poller);
};

}	// namespace annety

#endif	// ANT_POLLER_H_
