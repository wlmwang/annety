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
namespace internal
{
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

static const int kMaxRetryDelayMs = 30*1000;
static const int kInitRetryDelayMs = 500;	// every retry() is doubles delay

Connector::Connector(EventLoop* loop, const EndPoint& addr)
	: owner_loop_(loop)
	, server_addr_(addr)
	, retry_delay_ms_(kInitRetryDelayMs)
{
	LOG(DEBUG) << "Connector::Connector [" << server_addr_.to_ip_port() 
		<< "] Connector is constructing and the retry is " << retry_delay_ms_;
}

Connector::~Connector()
{
	LOG(DEBUG) << "Connector::~Connector [" << server_addr_.to_ip_port() 
		<< "] Connector is destructing and the retry is " << retry_delay_ms_;

	DCHECK(!connect_socket_) << " Please stop() the Connector before destruct";
	DCHECK(!connect_channel_) << " Please stop() the Connector before destruct";
}

void Connector::start()
{
	connect_ = true;

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));
}

void Connector::stop()
{
	connect_ = false;

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::stop_in_own_loop, shared_from_this()));
	
	// cancel timer
	if (time_id_.is_valid()) {
		owner_loop_->cancel(time_id_);
	}
}

void Connector::retry()
{
	connect_ = true;

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::retry_in_own_loop, shared_from_this()));
}

void Connector::restart()
{
	state_ = kDisconnected;
	retry_delay_ms_ = kInitRetryDelayMs;

	connect_ = true;
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));
}

void Connector::start_in_own_loop()
{
	owner_loop_->check_in_own_loop();
	DCHECK(state_ == kDisconnected);
  
	if (connect_) {
		connect();
	} else {
		LOG(DEBUG) << "Connector::start_in_own_loop do not connect, maybe has connected";
	}
}

void Connector::stop_in_own_loop()
{
	owner_loop_->check_in_own_loop();

	state_ = kDisconnected;
	
	remove_and_reset_channel();
	connect_socket_.reset();
}

void Connector::connect()
{
	owner_loop_->check_in_own_loop();

	DCHECK(!connect_socket_);
	connect_socket_.reset(new SocketFD(server_addr_.family(), true, true));

	ScopedClearLastError last_error;

	// non-block ::connect()
	internal::connect(*connect_socket_, server_addr_);
	switch (errno)
	{
	case 0:
	case EINPROGRESS:	// Operation now in progress (errno=115)
	case EINTR:
	case EISCONN:
		connecting();
		break;

	case EAGAIN:
	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:	// Connection refused
	case ENETUNREACH:
		retry_in_own_loop();
		break;

	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		PLOG(ERROR) << "Connector::connect has error";
		connect_socket_.reset();
		break;

	default:
		PLOG(ERROR) << "Connector::connect has unexpected error";
		connect_socket_.reset();
		break;
	}
}

void Connector::connecting()
{
	owner_loop_->check_in_own_loop();

	DCHECK(connect_socket_);
	DCHECK(!connect_channel_);

	state_ = kConnecting;
	connect_channel_.reset(new Channel(owner_loop_, connect_socket_.get()));

	// When the socket becomes writable, the connection is successfully established
	// But if the socket becomes readable or readable & writable, the connection fails
	// ON LINUX, connect() failure, the multiplexer trigger event is [IN OUT HUP ERR]
	// ON MACOS, connect() failure, the multiplexer trigger event is [IN PRI HUP]
	using containers::make_weak_bind;
	connect_channel_->set_error_callback(
		make_weak_bind(&Connector::handle_error, shared_from_this()));
	connect_channel_->set_write_callback(
		make_weak_bind(&Connector::handle_write, shared_from_this()));
	connect_channel_->set_read_callback(
		make_weak_bind(&Connector::handle_read, shared_from_this()));

	connect_channel_->enable_read_event();
	connect_channel_->enable_write_event();
}

void Connector::handle_write()
{
	owner_loop_->check_in_own_loop();

	LOG(TRACE) << "Connector::handle_write the state=" << state_;

	// When the socket becomes writable, the connection is successfully established
	// But if the socket becomes readable or readable & writable, the connection fails
	// ON LINUX, connect() failure, the multiplexer trigger event is [IN OUT HUP ERR]
	// ON MACOS, connect() failure, the multiplexer trigger event is [IN PRI HUP]
	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		int err = internal::get_sock_error(*connect_socket_);
		if (err) {
			ScopedClearLastError last_error;
			errno = err;
			PLOG(WARNING) << "Connector::handle_write connect has failed";

			connect_socket_.reset();
			state_ = kDisconnected;

			// connect failure
			if (error_connect_cb_) {
				error_connect_cb_();
			}
		} else {
			state_ = kConnected;
			if (connect_) {
				if (new_connect_cb_) {
					new_connect_cb_(std::move(connect_socket_), server_addr_);
				} else {
					// discard connection
					connect_socket_.reset();
				}
			} else {
				// discard connection
				connect_socket_.reset();
			}
		}
	}
}

void Connector::handle_read()
{
	owner_loop_->check_in_own_loop();

	LOG(WARNING) << "Connector::handle_read the state=" << state_;
	
	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		int err = internal::get_sock_error(*connect_socket_);
		{
			ScopedClearLastError last_error;
			errno = err;
			PLOG(WARNING) << "Connector::handle_read has failed";
		}

		connect_socket_.reset();
		state_ = kDisconnected;

		// connect failure
		if (err) {
			if (error_connect_cb_) {
				error_connect_cb_();
			}
		}
	}
}

void Connector::handle_error()
{
	owner_loop_->check_in_own_loop();

	LOG(WARNING) << "Connector::handle_error the state=" << state_;
	
	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		int err = internal::get_sock_error(*connect_socket_);
		{
			ScopedClearLastError last_error;
			errno = err;
			PLOG(WARNING) << "Connector::handle_error has failed";
		}
		
		connect_socket_.reset();
		state_ = kDisconnected;

		// connect failure
		if (err) {
			if (error_connect_cb_) {
				error_connect_cb_();
			}
		}
	}
}

void Connector::retry_in_own_loop()
{
	owner_loop_->check_in_own_loop();

	state_ = kDisconnected;
	connect_socket_.reset();

	if (connect_) {
		LOG(DEBUG) << "Connector::retry connecting to " << server_addr_.to_ip_port()
			<< " in " << retry_delay_ms_ << " milliseconds";

		// retry after
		using containers::make_weak_bind;
		time_id_ = owner_loop_->run_after(
			TimeDelta::from_milliseconds(retry_delay_ms_),
			make_weak_bind(&Connector::start_in_own_loop, shared_from_this()));

		// double delay
		retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);
	} else {
		LOG(TRACE) << "Connector::retry do not connect";
	}
}

void Connector::remove_and_reset_channel()
{
	owner_loop_->check_in_own_loop();

	if (connect_channel_ && !connect_channel_->is_none_event()) {
		connect_channel_->disable_all_event();
		connect_channel_->remove();
	}

	// can't reset Channel here, because we are inside Channel::handle_event
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
