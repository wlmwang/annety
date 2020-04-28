// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_POLL_POLLER_H_
#define ANT_POLL_POLLER_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "Poller.h"

#include <vector>
#include <poll.h>

namespace annety
{
// IO Multiplexing wrapper of poll(4).
// Just for develop/debug/testing only.
//
// This class does not owns the EventLoop and Channels lifetime.
// *Not thread safe*, but they are all called in the own loop.
class PollPoller : public Poller
{
public:
	PollPoller(EventLoop* loop);
	~PollPoller() override;
	
	// Polls the I/O events.
	// *Not thread safe*, but run in own loop thread.
	TimeStamp poll(int timeout_ms, ChannelList* active_channels) override;

	// Changes the interested I/O events.
	// *Not thread safe*, but run in own loop thread.
	void update_channel(Channel* channel) override;

	// Remove the channel, when it destructs.
	// *Not thread safe*, but run in own loop thread.
	void remove_channel(Channel* channel) override;

private:
	using PollFdList = std::vector<struct pollfd>;

	// *Not thread safe*, but run in own loop thread.
	void fill_active_channels(int num, ChannelList* active_channels) const;
	void update_poll_events(int operation, Channel* channel);
	PollFdList::iterator find_poll_events(int fd);

private:
	PollFdList listen_events_;

	DISALLOW_COPY_AND_ASSIGN(PollPoller);
};

}	// namespace annety

#endif	// ANT_POLL_POLLER_H_
