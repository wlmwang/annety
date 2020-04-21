// By: wlmwang
// Date: Jun 30 2019

#include "Connector.h"
#include "TimeStamp.h"
#include "Logging.h"
#include "EndPoint.h"
#include "SocketFD.h"
#include "SocketsUtil.h"
#include "Channel.h"
#include "EventLoop.h"
#include "ScopedClearLastError.h"
#include "containers/Bind.h"

#include <errno.h>

namespace annety
{
namespace internal {
int socket(sa_family_t family, bool nonblock, bool cloexec)
{
	return sockets::socket(family, nonblock, cloexec);
}
int connect(const SelectableFD& conn_sfd, EndPoint& server_addr)
{
	return sockets::connect(conn_sfd.internal_fd(), server_addr.get_sockaddr());
}
int get_sock_error(const SelectableFD& conn_sfd)
{
	return sockets::get_sock_error(conn_sfd.internal_fd());
}
bool is_self_connect(const SelectableFD& conn_sfd)
{
	return sockets::is_self_connect(conn_sfd.internal_fd());
}

}	// namespace internal

// Every retry() is doubles delay.
static const int kInitRetryDelayMs = 500;
static const int kMaxRetryDelayMs = 30*1000;

Connector::Connector(EventLoop* loop, const EndPoint& addr)
	: owner_loop_(loop)
	, server_addr_(addr)
	, retry_delay_ms_(kInitRetryDelayMs)
{
	DLOG(TRACE) << "Connector::Connector [" << server_addr_.to_ip_port() 
		<< "] Connector is constructing and the retry is " << retry_delay_ms_;
}

Connector::~Connector()
{
	DLOG(TRACE) << "Connector::~Connector [" << server_addr_.to_ip_port() 
		<< "] Connector is destructing and the retry is " << retry_delay_ms_;

	DCHECK(!connect_socket_) << "Please stop() the Connector before destruct";
	DCHECK(!connect_channel_) << "Please stop() the Connector before destruct";
}

void Connector::start()
{
	connect_ = true;

	// FIXME: Please use weak_from_this() since C++17.
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));
}

void Connector::stop()
{
	connect_ = false;

	// FIXME: Please use weak_from_this() since C++17.
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::stop_in_own_loop, shared_from_this()));
	
	// Cancel the timer of retry.
	if (time_id_.is_valid()) {
		owner_loop_->cancel(time_id_);
	}
}

void Connector::retry()
{
	connect_ = true;

	// FIXME: Please use weak_from_this() since C++17.
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::retry_in_own_loop, shared_from_this()));
}

void Connector::restart()
{
	retry_delay_ms_ = kInitRetryDelayMs;

	state_ = kDisconnected;
	connect_ = true;

	// FIXME: Please use weak_from_this() since C++17.
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));
}

void Connector::start_in_own_loop()
{
	owner_loop_->check_in_own_loop();
	DCHECK(state_ == kDisconnected);
  
	if (connect_) {
		do_connect();
	} else {
		DLOG(TRACE) << "Connector::start_in_own_loop do not connect, maybe has connected";
	}
}

void Connector::stop_in_own_loop()
{
	owner_loop_->check_in_own_loop();

	state_ = kDisconnected;
	
	remove_and_reset_channel();
	connect_socket_.reset();
}

void Connector::retry_in_own_loop()
{
	owner_loop_->check_in_own_loop();

	state_ = kDisconnected;
	connect_socket_.reset();

	if (connect_) {
		DLOG(TRACE) << "Connector::retry connecting to " << server_addr_.to_ip_port()
			<< " in " << retry_delay_ms_ << " milliseconds";

		// FIXME: Please use weak_from_this() since C++17.
		// retry after.
		using containers::make_weak_bind;
		time_id_ = owner_loop_->run_after(
			TimeDelta::from_milliseconds(retry_delay_ms_),
			make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));

		// double delay.
		retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);
	} else {
		DLOG(TRACE) << "Connector::retry do not connect";
	}
}

void Connector::do_connect()
{
	owner_loop_->check_in_own_loop();

	DCHECK(!connect_socket_);
	connect_socket_.reset(new SocketFD(server_addr_.family(), true, true));

	// Non-block socket to call ::connect().
	ScopedClearLastError last_error;
	internal::connect(*connect_socket_, server_addr_);
	switch (errno)
	{
	case 0:				// connection is established.
	case EINTR:			// 4:	Interrupted system call.
	case EISCONN:		// 106:	Transport endpoint is already connected.
	case EINPROGRESS:	// 115:	Operation now in progress.
						//		- The connection could not be established immediately.
		in_connect();
		break;

	case EAGAIN:		// 11:	Resource temporarily unavailable.
						//		- Not enough free local ports...
	case EADDRNOTAVAIL:	// 99:	Address not available.
						//		- Not enough free local ports...
	case EADDRINUSE:	// 98:	Address already in use.
						//		- Local address is in use...
	case ENETUNREACH:	// 101:	Network is unreachable.
						//		- Network issue.
	case ECONNREFUSED:	// 111:	Connection refused.
						//		- The address is not listening...
		retry_in_own_loop();
		break;

	case EPERM:			// 1.	Operation not permitted.
	case EACCES:		// 13:	Permission denied.
	case EFAULT:		// 14:	Bad address.
	case EBADF:			// 77:	File descriptor in bad state.
	case ENOTSOCK:		// 88:	Not a socket.  (non-socket).
	case EAFNOSUPPORT:	// 97:	Address family not supported by protocol.
	case EALREADY:		// 114:	Connection already in progress.
						//		- Before the connection is completely established, 
						//		will fail with EALREADY.
		PLOG(ERROR) << "Connector::do_connect has error";
		connect_socket_.reset();
		break;

	default:
		PLOG(ERROR) << "Connector::do_connect has unexpected error";
		connect_socket_.reset();
		break;
	}
}

void Connector::in_connect()
{
	owner_loop_->check_in_own_loop();

	DCHECK(connect_socket_);
	DCHECK(!connect_channel_);

	state_ = kConnecting;
	connect_channel_.reset(new Channel(owner_loop_, connect_socket_.get()));

	// IO Multiplexing Event: 
	// 1. If the connection is established and no data arrives, then 
	//	  sockfd is writable.
	// 2. If the connection is established before poll and some data 
	// 	  has arrived, then sockfd is readable and writable.
	// 3. If the connection fails, sockfd is also readable and writable.
	//
	// To distinguish between 2 and 3 (both readable and writable), you 
	// can call getsockopt() to check for errors.
	//
	// LINUX: connect() fail, the multiplexer event may is [IN OUT HUP ERR]

	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`connect_channel_` storages this shared_ptr).
	using containers::make_weak_bind;
	connect_channel_->set_error_callback(
		make_weak_bind(&Connector::handle_error, shared_from_this()));
	connect_channel_->set_write_callback(
		make_weak_bind(&Connector::handle_write, shared_from_this()));

	connect_channel_->enable_write_event();

	// FIXME: Compatible with MACOS platform??
	// connect_channel_->set_read_callback(
	// 	make_weak_bind(&Connector::handle_write, shared_from_this()));
	// connect_channel_->enable_read_event();
}

void Connector::handle_write()
{
	owner_loop_->check_in_own_loop();

	LOG(DEBUG) << "Connector::handle_write the state=" << state_;

	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		ScopedClearLastError last_error;
		int err = internal::get_sock_error(*connect_socket_);
		if (err) {
			errno = err;
			PLOG(WARNING) << "Connector::handle_write connect has failed";

			connect_socket_.reset();
			state_ = kDisconnected;

			// connect fail. --- retry()
			if (error_connect_cb_) {
				error_connect_cb_();
			}
		} else {
			state_ = kConnected;
			if (connect_) {
				if (new_connect_cb_) {
					new_connect_cb_(std::move(connect_socket_), server_addr_);
				} else {
					// discard connection.
					connect_socket_.reset();
				}
			} else {
				// discard connection.
				connect_socket_.reset();
			}
		}
	} else {
		DCHECK(state_ == kDisconnected);
	}
}

void Connector::handle_error()
{
	owner_loop_->check_in_own_loop();

	LOG(WARNING) << "Connector::handle_error the state=" << state_;
	
	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		ScopedClearLastError last_error;
		int err = internal::get_sock_error(*connect_socket_);
		{
			errno = err;
			PLOG(WARNING) << "Connector::handle_error has failed";
		}
		
		connect_socket_.reset();
		state_ = kDisconnected;

		// connect fail. --- retry()
		if (err) {
			if (error_connect_cb_) {
				error_connect_cb_();
			}
		}
	}
}

void Connector::remove_and_reset_channel()
{
	owner_loop_->check_in_own_loop();

	if (connect_channel_ && !connect_channel_->is_none_event()) {
		connect_channel_->disable_all_event();
		connect_channel_->remove();
	}

	// Can't remove Channel here immediately, because we are inside channel's 
	// event handling function now.  The purpose is not to let the channel's 
	// iterator fail.
	using containers::make_weak_bind;
	owner_loop_->queue_in_own_loop(
		make_weak_bind(&Connector::reset_channel, shared_from_this()));
}

void Connector::reset_channel()
{
	owner_loop_->check_in_own_loop();
	connect_channel_.reset();
}

}	// namespace annety
