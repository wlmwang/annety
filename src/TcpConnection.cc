// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpConnection.h"
#include "Channel.h"
#include "NetBuffer.h"
#include "EndPoint.h"
#include "EventLoop.h"
#include "SocketFD.h"
#include "SocketsUtil.h"
#include "ScopedClearLastError.h"
#include "containers/Bind.h"

#include <utility>

namespace annety
{
namespace internal
{
// Specific connect socket related function interfaces
int set_keep_alive(const SelectableFD& sfd, bool on)
{
	return sockets::set_keep_alive(sfd.internal_fd(), on);
}
int set_tcp_nodelay(const SelectableFD& sfd, bool on)
{
	return sockets::set_tcp_nodelay(sfd.internal_fd(), on);
}
int shutdown(const SelectableFD& sfd, int how = SHUT_WR)
{
	return sockets::shutdown(sfd.internal_fd(), how);
}

}	// namespace internal

void default_connect_callback(const TcpConnectionPtr& conn)
{
	LOG(TRACE) << conn->local_addr().to_ip_port() << " -> "
			   << conn->peer_addr().to_ip_port() << " is "
			   << (conn->connected() ? "UP" : "DOWN");
	// do not call conn->force_close()
	// because some users want to register message callback only.
}

void default_message_callback(const TcpConnectionPtr&, NetBuffer* buf, TimeStamp)
{
	buf->has_read_all();
}

TcpConnection::TcpConnection(EventLoop* loop,
							const std::string& name,
							SelectableFDPtr sockfd,
							const EndPoint& localaddr,
							const EndPoint& peeraddr)
	: owner_loop_(loop)
	, name_(name)
	, local_addr_(localaddr)
	, peer_addr_(peeraddr)
	, connect_socket_(std::move(sockfd))
	, connect_channel_(new Channel(owner_loop_, connect_socket_.get()))
	, input_buffer_(new NetBuffer())
	, output_buffer_(new NetBuffer())
{
	LOG(DEBUG) << "TcpConnection::TcpConnection the [" <<  name_ << "] connection of"
		<< " fd=" << connect_socket_->internal_fd() << " is constructing";

	// setting keepalive sockopt
	internal::set_keep_alive(*connect_socket_, true);
}

TcpConnection::~TcpConnection()
{
	LOG(DEBUG) << "TcpConnection::~TcpConnection the [" <<  name_ << "] connecting of "
		<< " fd=" << connect_socket_->internal_fd()
		<< " state=" << state_to_string() << " is destructing";

	DCHECK(initilize_ == true);
	DCHECK(state_ == kDisconnected);
}

void TcpConnection::initialize()
{
	CHECK(!initilize_);
	initilize_ = true;

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(make_weak_bind(&TcpConnection::initialize_in_loop, shared_from_this()));
}

void TcpConnection::initialize_in_loop()
{
	owner_loop_->check_in_own_loop();

	// cannot use std::bind() with shared_from_this()
	// it would forms a circular reference to the connect_channel_ object
	using containers::_1;
	using containers::make_weak_bind;

	connect_channel_->set_read_callback(
		make_weak_bind(&TcpConnection::handle_read, shared_from_this(), _1));
	connect_channel_->set_write_callback(
		make_weak_bind(&TcpConnection::handle_write, shared_from_this()));
	connect_channel_->set_close_callback(
		make_weak_bind(&TcpConnection::handle_close, shared_from_this()));
	connect_channel_->set_error_callback(
		make_weak_bind(&TcpConnection::handle_error, shared_from_this()));
}

void TcpConnection::set_tcp_nodelay(bool on)
{
	CHECK(initilize_);

	DCHECK_GE(internal::set_tcp_nodelay(*connect_socket_, on), 0);
}

void TcpConnection::send(const void* data, int len)
{
	CHECK(initilize_);

	send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
	CHECK(initilize_);

	// FIXME: as string
	if (state_ == kConnected) {
		if (owner_loop_->is_in_own_loop()) {
			send_in_loop(message);
		} else {
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::send_in_loop;
			// move semantics in here, even RVO
			using containers::make_weak_bind;
			owner_loop_->run_in_own_loop(make_weak_bind(fp, shared_from_this(), message.as_string()));
		}
	}
}

void TcpConnection::send(NetBuffer&& message)
{
	CHECK(initilize_);

	NetBuffer msg(std::move(message));
	send(&msg);
}

void TcpConnection::send(NetBuffer* buf)
{
	CHECK(initilize_);

	// FIXME: as string
	if (state_ == kConnected) {
		if (owner_loop_->is_in_own_loop()) {
			send_in_loop(buf->taken_as_string());
		} else {
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::send_in_loop;
			// move semantics in here, even RVO
			using containers::make_weak_bind;
			owner_loop_->run_in_own_loop(make_weak_bind(fp, shared_from_this(), buf->taken_as_string()));
		}
	}
}

void TcpConnection::send_in_loop(const StringPiece& message)
{
	send_in_loop(message.data(), message.size());
}

void TcpConnection::send_in_loop(const void* data, size_t len)
{
	owner_loop_->check_in_own_loop();

	ssize_t nwrote = 0;
	size_t remaining = len;
	bool fault_error = false;
	if (state_ == kDisconnected) {
		LOG(WARNING) << "TcpConnection::send_in_loop was disconnected, give up writing";
		return;
	}

	// if no thing in output queue, try writing directly
	if (!connect_channel_->is_write_event() && output_buffer_->readable_bytes() == 0) {
		nwrote = connect_socket_->write(data, len);
		if (nwrote >= 0) {
			remaining = len - nwrote;
			if (remaining == 0 && write_complete_cb_) {
				owner_loop_->queue_in_own_loop(std::bind(write_complete_cb_, shared_from_this()));
			}
		} else {
			nwrote = 0;
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				PLOG(ERROR) << "TcpConnection::send_in_loop has failed";
				if (errno == EPIPE || errno == ECONNRESET) {
					// the peer endpoint has closed
					fault_error = true;
				}
			}
		}
	}

	DCHECK(remaining <= len);
	if (!fault_error && remaining > 0) {
		size_t oldlen = output_buffer_->readable_bytes();

		if (oldlen + remaining >= high_water_mark_ && oldlen < high_water_mark_ 
			&& high_water_mark_cb_)
		{
			owner_loop_->queue_in_own_loop(std::bind(high_water_mark_cb_, shared_from_this(), 
										oldlen + remaining));
		}

		// copy data to output_buffer_
		output_buffer_->append(static_cast<const char*>(data) + nwrote, remaining);
		if (!connect_channel_->is_write_event()) {
			connect_channel_->enable_write_event();
		}
	}
}

void TcpConnection::shutdown()
{
	CHECK(initilize_);

	if (state_ == kConnected) {
		state_ = kDisconnecting;
		using containers::make_weak_bind;
		owner_loop_->run_in_own_loop(make_weak_bind(&TcpConnection::shutdown_in_loop, shared_from_this()));
	}
}

void TcpConnection::shutdown_in_loop()
{
	owner_loop_->check_in_own_loop();

	if (!connect_channel_->is_write_event()) {
		internal::shutdown(*connect_socket_);
	}
}

void TcpConnection::force_close()
{
	CHECK(initilize_);

	// FIXME: use compare and swap
	if (state_ == kConnected || state_ == kDisconnecting) {
		state_ = kDisconnecting;
		owner_loop_->queue_in_own_loop(std::bind(&TcpConnection::force_close_in_loop, shared_from_this()));
	}
}

void TcpConnection::force_close_with_delay(double delay_s)
{
	CHECK(initilize_);

	if (state_ == kConnected || state_ == kDisconnecting) {
		state_ = kDisconnecting;
		// not force_close_in_loop to avoid race condition
		using containers::make_weak_bind;
		owner_loop_->run_after(TimeDelta::from_seconds_d(delay_s), 
							make_weak_bind(&TcpConnection::force_close, shared_from_this()));
	}
}

void TcpConnection::force_close_in_loop()
{
	owner_loop_->check_in_own_loop();

	if (state_ == kConnected || state_ == kDisconnecting) {
		// as if we received 0 byte in handle_read();
		handle_close();
	}
}

void TcpConnection::start_read()
{
	CHECK(initilize_);

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(make_weak_bind(&TcpConnection::start_read_in_loop, shared_from_this()));
}

void TcpConnection::start_read_in_loop()
{
	owner_loop_->check_in_own_loop();

	if (!reading_ || !connect_channel_->is_read_event()) {
		connect_channel_->enable_read_event();
		reading_ = true;
	}
}

void TcpConnection::stop_read()
{
	CHECK(initilize_);

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(make_weak_bind(&TcpConnection::stop_read_in_loop, shared_from_this()));
}

void TcpConnection::stop_read_in_loop()
{
	owner_loop_->check_in_own_loop();

	if (reading_ || connect_channel_->is_read_event()) {
		connect_channel_->disable_read_event();
		reading_ = false;
	}
}

void TcpConnection::connect_established()
{
	owner_loop_->check_in_own_loop();

	DCHECK(state_ == kConnecting);
	state_ = kConnected;

	connect_channel_->enable_read_event();

	connect_cb_(shared_from_this());

	LOG(TRACE) << "TcpConnection::connect_established is called";
}

void TcpConnection::connect_destroyed()
{
	owner_loop_->check_in_own_loop();

	if (state_ == kConnected) {
		state_ = kDisconnected;
		connect_channel_->disable_all_event();

		connect_cb_(shared_from_this());
	}
	connect_channel_->remove();

	LOG(TRACE) << "TcpConnection::connect_destroyed is called";
}

void TcpConnection::handle_read(TimeStamp recv_tm)
{
	owner_loop_->check_in_own_loop();

	ScopedClearLastError last_error;

	// wrapper ::read() call
	ssize_t n = input_buffer_->read_fd(connect_socket_->internal_fd());
	if (n > 0) {
		message_cb_(shared_from_this(), input_buffer_.get(), recv_tm);
	} else if (n == 0) {
		LOG(TRACE) << "TcpConnection::handle_read the conntion fd=" << connect_socket_->internal_fd() 
				<< " is going to closing";
		handle_close();
	} else {
		PLOG(ERROR) << "TcpConnection::handle_read the conntion fd=" << connect_socket_->internal_fd() 
				<< " has been occurred error";
		handle_error();
	}
}

void TcpConnection::handle_write()
{
	owner_loop_->check_in_own_loop();

	if (connect_channel_->is_write_event()) {
		ssize_t n = connect_socket_->write(output_buffer_->begin_read(),
										   output_buffer_->readable_bytes());
		if (n > 0) {
			output_buffer_->has_read(n);
			if (output_buffer_->readable_bytes() == 0) {
				connect_channel_->disable_write_event();
				if (write_complete_cb_) {
					owner_loop_->queue_in_own_loop(std::bind(write_complete_cb_, shared_from_this()));
				}
				if (state_ == kDisconnecting) {
					shutdown_in_loop();
				}
			}
		} else {
			PLOG(ERROR) << "TcpConnection::handle_write has failed";
		}
	} else {
		LOG(DEBUG) << "TcpConnection::handle_write the conntion fd=" << connect_socket_->internal_fd()
			<< " is down, no more writing";
	}
}

void TcpConnection::handle_close()
{
	owner_loop_->check_in_own_loop();

	LOG(TRACE) << "TcpConnection::handle_close the fd = " << connect_socket_->internal_fd() 
			<< " state = " << state_to_string() << " is closing";
	DCHECK(state_ == kConnected || state_ == kDisconnecting);

	state_ = kDisconnected;
	connect_channel_->disable_all_event();

	// backup the connection(shared_ptr<>)
	TcpConnectionPtr backup(shared_from_this());
	connect_cb_(backup);

	close_cb_(backup);
}

void TcpConnection::handle_error()
{
	PLOG(ERROR) << "TcpConnection::handle_error the connection " << name_ << " has a error";
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

// constructs an object of type TcpConnectionPtr and wraps it
TcpConnectionPtr make_tcp_connection(EventLoop* loop,
				const std::string& name,
				SelectableFDPtr&& sockfd,
				const EndPoint& localaddr,
				const EndPoint& peeraddr)
{
	CHECK(loop && sockfd);

	TcpConnectionPtr conn(new TcpConnection(loop, name, std::move(sockfd), localaddr, peeraddr));
	conn->initialize();
	return conn;
}

}	// namespace annety
