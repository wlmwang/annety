// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpConnection.h"
#include "Channel.h"
#include "EndPoint.h"
#include "EventLoop.h"
#include "SocketFD.h"
#include "Logging.h"
#include "SocketsUtil.h"
#include "ScopedClearLastError.h"

#include <iostream>

namespace annety {
TcpConnection::TcpConnection(EventLoop* loop,
							const std::string& name,
							const SocketFD& sfd,
							const EndPoint& localaddr,
							const EndPoint& peeraddr)
  : owner_loop_(loop),
    name_(name),
    state_(kConnecting),
    socket_(new SocketFD(sfd.internal_fd())),
    channel_(new Channel(owner_loop_, socket_.get()))
{
	channel_->set_read_callback(
		std::bind(&TcpConnection::handle_read, this, _1));

	LOG(TRACE) << "TcpConnection::ctor[" <<  name_ << "] at " << this
		<< " fd=" << sfd.internal_fd();
}

TcpConnection::~TcpConnection()
{
	LOG(TRACE) << "TcpConnection::dtor[" <<  name_ << "] at " << this
		<< " fd=" << channel_->fd();
}

void TcpConnection::connect_established()
{
	owner_loop_->check_in_own_thread(true);
	
	DCHECK(state_ == kConnecting);
	state_ = kConnected;

	// channel_->enable_read_event();
}

void TcpConnection::handle_read(Time receive_tm)
{
	owner_loop_->check_in_own_thread(true);

	ScopedClearLastError last_err;

	char buf[65535]{0};
	ssize_t n = sockets::read(channel_->fd(), buf, sizeof buf);
	if (n > 0) {
		std::cout << buf << std::endl;
		// message_cb_(shared_from_this(), &inputBuffer_, receiveTime);
	} else if (n == 0) {
		// handle_close();
	} else {
		LOG(ERROR) << "TcpConnection::handleRead";
		
		// handleError();
	}
}

}	// namespace annety
