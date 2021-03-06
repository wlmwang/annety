// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_POLLER_H_
#define ANT_POLLER_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "EventLoop.h"

#include <vector>
#include <map>

namespace annety
{
namespace {
const int kChannelPollInit = -1;
const int kChannelPollAdded = 1;
const int kChannelPollDeleted = 2;

const int kPollCtlAdd = 1;
const int kPollCtlDel = 2;
const int kPollCtlMod = 3;
}	// anonymous namespace

class Channel;

// Base class of IO Multiplexing
//
// This class does not owns the EventLoop and Channels lifetime.
// *Not thread safe*, but they are all called in the own loop.
class Poller
{
public:
	using ChannelList = std::vector<Channel*>;
	using ChannelMap = std::map<int, Channel*>;

	Poller(EventLoop* loop);
	virtual ~Poller();

	// Polls the I/O events.
	// *Not thread safe*, but run in own loop thread.
	virtual TimeStamp poll(int timeout_ms, ChannelList* activeChannels) = 0;

	// Changes the interested I/O events.
	// *Not thread safe*, but run in own loop thread.
	virtual void update_channel(Channel* channel) = 0;

	// Remove the channel, when it destructs.
	// *Not thread safe*, but run in own loop thread.
	virtual void remove_channel(Channel* channel) = 0;

	// *Not thread safe*, but run in own loop thread.
	virtual bool has_channel(Channel* channel) const;

	void check_in_own_loop() const
	{
		owner_loop_->check_in_own_loop();
	}

protected:
	static const char* operation_to_string(int op);

	EventLoop* owner_loop_;
	ChannelMap channels_;

	DISALLOW_COPY_AND_ASSIGN(Poller);
	
};

}	// namespace annety

#endif	// ANT_POLLER_H_
