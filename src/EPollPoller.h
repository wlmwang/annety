// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_EPOLL_POLLER_H_
#define ANT_EPOLL_POLLER_H_

#include "Macros.h"
#include "Poller.h"
#include "TimeStamp.h"

#if defined(OS_LINUX)
#include <vector>
#include <poll.h>
#include <sys/epoll.h>

namespace annety
{
// IO Multiplexing wrapper of epoll(4).
//
// This class does not owns the EventLoop and Channels lifetime.
class EPollPoller : public Poller
{
public:
	EPollPoller(EventLoop* loop);
	~EPollPoller() override;

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
	using EventList = std::vector<struct epoll_event>;

	static const int kInitEventListSize = 16;
	static const char* operation_to_string(int op);
	
	// *Not thread safe*, but run in own loop thread.
	void fill_active_channels(int num, ChannelList* active_channels) const;
	int update_poll_events(int operation, Channel* channel);

private:
	int epollfd_;
	EventList active_events_;

	DISALLOW_COPY_AND_ASSIGN(EPollPoller);
};

}	// namespace annety

#endif	// OS_LINUX

#endif	// ANT_EPOLL_POLLER_H_
