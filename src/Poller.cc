// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "Poller.h"
#include "Channel.h"

namespace annety
{
Poller::Poller(EventLoop* loop)
	: owner_loop_(loop) {}

Poller::~Poller() = default;

bool Poller::has_channel(Channel* channel) const
{
	check_in_own_loop();
	
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}

}	// namespace annety
