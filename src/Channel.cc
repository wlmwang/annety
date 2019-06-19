// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "Channel.h"
#include "SelectableFD.h"
#include "Time.h"
#include "EventLoop.h"
#include "Logging.h"

#include <sstream>
#include <utility>
#include <poll.h>

namespace annety
{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, SelectableFD* sfd)
	: owner_loop_(loop),
	  select_fd_(sfd) {}

Channel::~Channel()
{
	DCHECK(!event_handling_);
	DCHECK(!added_to_loop_);
	if (owner_loop_->is_in_own_thread()) {
		// must has removed
		DCHECK(!owner_loop_->has_channel(this));
	}
}

int Channel::fd() const
{
	return select_fd_->internal_fd();
}

void Channel::handle_event(Time receive_tm)
{
	event_handling_ = true;

	LOG(TRACE) << revents_to_string();
	
	// hup event
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
		LOG_IF(WARNING, logging_hup_) << "fd = " << fd() 
			<< " Channel::handle_event() POLLHUP";
		if (close_cb_) {
			close_cb_();
		}
	}
	// error
	if (revents_ & (POLLERR | POLLNVAL)) {
		LOG_IF(WARNING, revents_ & POLLNVAL) << "fd = " << fd() 
			<< " Channel::handle_event() POLLNVAL";
		if (error_cb_) {
			error_cb_();
		}
	}
	// readable
	if (revents_ & (POLLIN | POLLPRI | POLLHUP)) {
		if (read_cb_) {
			read_cb_(receive_tm);
		}
	}
	// writeable
	if (revents_ & POLLOUT) {
		if (write_cb_) {
			write_cb_();
		}
	}
	event_handling_ = false;
}

void Channel::remove()
{
	// must has disable_all_event
	DCHECK(is_none_event());
	
	added_to_loop_ = false;
	owner_loop_->remove_channel(this);
}

void Channel::update()
{
	added_to_loop_ = true;
	owner_loop_->update_channel(this);
}

std::string Channel::revents_to_string() const
{
	return events_to_string(fd(), revents_);
}

std::string Channel::events_to_string() const
{
	return events_to_string(fd(), events_);
}

std::string Channel::events_to_string(int fd, int ev)
{
	std::ostringstream oss;

	oss << fd << ": ";
	if (ev & POLLIN) {
		oss << "IN ";
	}
	if (ev & POLLPRI) {
		oss << "PRI ";
	}
	if (ev & POLLOUT) {
		oss << "OUT ";
	}
	if (ev & POLLHUP) {
		oss << "HUP ";
	}
#if !defined(OS_MACOSX)
	if (ev & POLLRDHUP) {
		oss << "RDHUP ";
	}
#endif	// OS_MACOSX
	if (ev & POLLERR) {
		oss << "ERR ";
	}
	if (ev & POLLNVAL) {
		oss << "NVAL ";
	}
	return oss.str();
}

}	// namespace annety
