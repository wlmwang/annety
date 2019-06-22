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

#include <utility>
#include <iostream>

namespace annety {
TcpConnection::TcpConnection(EventLoop* loop,
							const std::string& name,
							SelectableFDPtr sockfd,
							const EndPoint& localaddr,
							const EndPoint& peeraddr)
  : owner_loop_(loop),
    name_(name),
    state_(kConnecting),
    conn_socket_(std::move(sockfd)),
    conn_channel_(new Channel(owner_loop_, conn_socket_.get()))
{
	conn_channel_->set_read_callback(
		std::bind(&TcpConnection::handle_read, this, _1));
	// conn_channel_->set_write_callback(
	// 	std::bind(&TcpConnection::handle_write, this));
	conn_channel_->set_close_callback(
		std::bind(&TcpConnection::handle_close, this));
	conn_channel_->set_error_callback(
		std::bind(&TcpConnection::handle_error, this));

	LOG(TRACE) << "TcpConnection::construct[" <<  name_ << "] at " << this
		<< " fd=" << conn_socket_->internal_fd();
}

TcpConnection::~TcpConnection()
{
	LOG(TRACE) << "TcpConnection::destruct[" <<  name_ << "] at " << this
		<< " fd=" << conn_channel_->fd();
}

void TcpConnection::connect_established()
{
	owner_loop_->check_in_own_thread(true);
	
	DCHECK(state_ == kConnecting);
	state_ = kConnected;

	conn_channel_->enable_read_event();
}

void TcpConnection::connect_destroyed()
{
	owner_loop_->check_in_own_thread(true);

	if (state_ == kConnected) {
		state_ = kDisconnected;
		conn_channel_->disable_all_event();
	}
	conn_channel_->remove();
}

void TcpConnection::handle_read(Time receive_tm)
{
	owner_loop_->check_in_own_thread(true);

	ScopedClearLastError last_err;

	char buf[65535]{0};
	ssize_t n = sockets::read(conn_channel_->fd(), buf, sizeof buf);
	if (n > 0) {
		std::cout << buf << std::endl;

		// message_cb_(shared_from_this(), &inputBuffer_, receiveTime);
	} else if (n == 0) {
		handle_close();
	} else {
		LOG(ERROR) << "TcpConnection::handle_read";

		handle_error();
	}
}

void TcpConnection::handle_close()
{
	owner_loop_->check_in_own_thread(true);

	LOG(TRACE) << "fd = " << conn_channel_->fd() << " state = " << state_to_string();
	DCHECK(state_ == kConnected || state_ == kDisconnecting);

	state_ = kDisconnected;

	conn_channel_->disable_all_event();

	close_cb_(shared_from_this());
}

void TcpConnection::handle_error()
{
	PLOG(ERROR) << "TcpConnection::handle_error [" << name_ << "]";
}

const char* TcpConnection::state_to_string() const
{
	switch (state_) {
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

}	// namespace annety
