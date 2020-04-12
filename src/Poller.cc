// By: wlmwang
// Date: Jun 17 2019

#include "Poller.h"
#include "Channel.h"
#include "Logging.h"

namespace annety
{
Poller::Poller(EventLoop* loop)
	: owner_loop_(loop)
{
	CHECK(loop);
}

Poller::~Poller() = default;

bool Poller::has_channel(Channel* channel) const
{
	check_in_own_loop();
	
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}

const char* Poller::operation_to_string(int op)
{
	switch (op) {
	case kPollCtlAdd:
		return "ADD";
	case kPollCtlDel:
		return "DEL";
	case kPollCtlMod:
		return "MOD";
	}
	
	NOTREACHED();
	return "Unknown Operation";
}

}	// namespace annety
