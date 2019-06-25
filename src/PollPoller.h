// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_POLL_POLLER_H_
#define ANT_POLL_POLLER_H_

#include "Poller.h"
#include "Time.h"

#include <vector>
#include <poll.h>

namespace annety
{
class PollPoller : public Poller
{
public:
	PollPoller(EventLoop* loop);
	~PollPoller() override;
	
	Time poll(int timeout_ms, ChannelList* active_channels) override;

	void update_channel(Channel* channel) override;
	void remove_channel(Channel* channel) override;

private:
	void fill_active_channels(int num, ChannelList* active_channels) const;

	using PollFdList = std::vector<struct pollfd>;
	
	PollFdList pollfds_;
};

}	// namespace annety

#endif	// ANT_POLL_POLLER_H_