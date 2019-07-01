
#include "Connector.h"
#include "Logging.h"
#include "EndPoint.h"
#include "SocketFD.h"
#include "SocketsUtil.h"
#include "Channel.h"
#include "EventLoop.h"
#include "ScopedClearLastError.h"

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
static const int kInitRetryDelayMs = 500;

Connector::Connector(EventLoop* loop, const EndPoint& addr)
	: owner_loop_(loop),
	  server_addr_(addr),
	  retry_delay_ms_(kInitRetryDelayMs)
{
	LOG(TRACE) << "Connector::Connector [" << server_addr_.to_ip_port() << "], Connector "
		<< this << " is constructing";
}

Connector::~Connector()
{
	LOG(TRACE) << "Connector::~Connector [" << server_addr_.to_ip_port() << "], Connector "
		<< this << " is destructing";

	DCHECK(!connect_socket_);
	DCHECK(!connect_channel_);
}

void Connector::start()
{
	connect_ = true;
	owner_loop_->run_in_own_loop(std::bind(&Connector::start_in_own_loop, this));
}

void Connector::stop()
{
	connect_ = false;
	owner_loop_->queue_in_own_loop(std::bind(&Connector::stop_in_own_loop, this));
	// FIXME: cancel timer
}

void Connector::restart()
{
	owner_loop_->check_in_own_loop();

	state_ = kDisconnected;
	retry_delay_ms_ = kInitRetryDelayMs;

	connect_ = true;
	start_in_own_loop();
}

void Connector::start_in_own_loop()
{
	owner_loop_->check_in_own_loop();
	DCHECK(state_ == kDisconnected);
  
	if (connect_) {
		connect();
	} else {
		LOG(TRACE) << "do not connect, maybe has connected";
	}
}

void Connector::stop_in_own_loop()
{
	owner_loop_->check_in_own_loop();

	if (state_ == kConnecting) {
		state_ = kDisconnected;

		remove_and_reset_channel();
		retry();
	}
}

void Connector::connect()
{
	DCHECK(!connect_socket_);
	connect_socket_.reset(new SocketFD(server_addr_.family(), true, true));

	ScopedClearLastError last_error{};

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
	case ECONNREFUSED:
	case ENETUNREACH:
		retry();
		break;

	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		PLOG(ERROR) << "connect error";
		connect_socket_.reset();
		break;

	default:
		PLOG(ERROR) << "connect unexpected error";
		connect_socket_.reset();
		break;
	}
}

void Connector::connecting()
{
	DCHECK(connect_socket_);
	DCHECK(!connect_channel_);

	state_ = kConnecting;
	connect_channel_.reset(new Channel(owner_loop_, connect_socket_.get()));

	connect_channel_->set_write_callback(
		std::bind(&Connector::handle_write, this));
	connect_channel_->set_error_callback(
		std::bind(&Connector::handle_error, this));

	connect_channel_->enable_write_event();
}

void Connector::retry()
{
	state_ = kDisconnected;
	connect_socket_.reset();

	if (connect_) {
		LOG(INFO) << "Connector::retry - retry connecting to " << server_addr_.to_ip_port()
			<< " in " << retry_delay_ms_ << " milliseconds. ";

		// owner_loop_->runAfter(retry_delay_ms_/1000.0,
		// 	std::bind(&Connector::start_in_own_loop, shared_from_this()));
		// retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);

		owner_loop_->queue_in_own_loop(std::bind(&Connector::start_in_own_loop, shared_from_this()));
	} else {
		LOG(TRACE) << "do not connect";
	}
}

void Connector::handle_write()
{
	owner_loop_->check_in_own_loop();
	LOG(TRACE) << "Connector::handle_write " << state_;

	ScopedClearLastError last_error{};

	if (state_ == kConnecting) {
		DCHECK(connect_socket_);
		remove_and_reset_channel();

		errno = internal::get_sock_error(*connect_socket_);
		if (errno) {
			PLOG(WARNING) << "Connector::handle_write failed";
			retry();
		} else if (internal::is_self_connect(*connect_socket_)) {
			LOG(WARNING) << "Connector::handle_write - self connect";
			retry();
		} else {
			state_ = kConnected;
			if (connect_) {
				if (new_connect_cb_) {
					new_connect_cb_(std::move(connect_socket_), server_addr_);
				} else {
					connect_socket_.reset();
				}
			} else {
				connect_socket_.reset();
			}
		}
	} else {
		// todo. what happened?
		DCHECK(state_ == kDisconnected);
	}
}

void Connector::handle_error()
{
	DCHECK(connect_socket_);
	LOG(ERROR) << "Connector::handle_error state=" << state_;
	
	if (state_ == kConnecting) {
		ScopedClearLastError last_error{};
		errno = internal::get_sock_error(*connect_socket_);
		PLOG(TRACE) << "Connector::handle_error failed";

		remove_and_reset_channel();
		retry();
	}
}

void Connector::remove_and_reset_channel()
{
	connect_channel_->disable_all_event();
	connect_channel_->remove();

	// can't reset Channel here, because we are inside Channel::handle_event
	owner_loop_->queue_in_own_loop(std::bind(&Connector::reset_channel, this));
}

void Connector::reset_channel()
{
	owner_loop_->check_in_own_loop();
	
	connect_channel_.reset();
}

}	// namespace annety
