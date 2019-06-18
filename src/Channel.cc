// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "Channel.h"
#include "SelectableFD.h"
#include "Time.h"
#include "EventLoop.h"

#include <utility>
#include <poll.h>

namespace annety {
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, SelectableFD* sfd)
	: owner_loop_(loop),
	  select_fd_(sfd),
	  events_(0),
	  revents_(0),
	  eventHandling_(false),
	  addedToLoop_(false) {}

Channel::~Channel() = default;

int Channel::fd() const {
	return select_fd_->internal_fd();
}

void Channel::handle_event(Time receiveTime) {
	//
}

void Channel::update() {
	addedToLoop_ = true;
	owner_loop_->update_channel(this);
}

}	// namespace annety
