// By: wlmwang
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
namespace internal {
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
	LOG(DEBUG) << conn->local_addr().to_ip_port() << " -> "
			   << conn->peer_addr().to_ip_port() << " is "
			   << "UP";
}
void default_close_callback(const TcpConnectionPtr& conn)
{
	LOG(DEBUG) << conn->local_addr().to_ip_port() << " -> "
			   << conn->peer_addr().to_ip_port() << " is "
			   << "DOWN";
	// Do not call conn->force_close(), because some users want 
	// to register message callback only.
}
void default_message_callback(const TcpConnectionPtr&, NetBuffer* buf, TimeStamp)
{
	// Discard all the buffer data.
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
	CHECK(loop);

	LOG(DEBUG) << "TcpConnection::TcpConnection the [" <<  name_ << "] connection of"
		<< " fd=" << connect_socket_->internal_fd() << " is constructing";

	// setting keepalive sockopt
	internal::set_keep_alive(*connect_socket_, true);

	// When necessary, manually closed by the user.
	// set_tcp_nodelay(true);
}

void TcpConnection::initialize()
{
	DCHECK(!initilize_);
	initilize_ = true;

	// Run in the `conn` own loop, because init `conn` and own loop of `conn` may 
	// not be the same thread. --- the `server` thread call this.
	owner_loop_->run_in_own_loop(
		std::bind(&TcpConnection::initialize_in_loop, shared_from_this()));
}

TcpConnection::~TcpConnection()
{
	DCHECK(initilize_);
	DCHECK(state_ == kDisconnected);
	
	LOG(DEBUG) << "TcpConnection::~TcpConnection the [" <<  name_ << "] connecting of "
		<< " fd=" << connect_socket_->internal_fd()
		<< " state=" << state_to_string() << " is destructing";
}

void TcpConnection::initialize_in_loop()
{
	owner_loop_->check_in_own_loop();

	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`connect_channel_` storages this shared_ptr).
	//
	// Register all channel's base I/O events.
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
	DCHECK(initilize_);

	int rt = internal::set_tcp_nodelay(*connect_socket_, on);
	DCHECK_GE(rt, 0);
}

void TcpConnection::send(const void* data, int len)
{
	DCHECK(initilize_);
	
	CHECK(data);
	send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const NetBuffer& message)
{
	DCHECK(initilize_);

	send(message.to_string_piece());
}

void TcpConnection::send(const NetBuffer* message)
{
	DCHECK(initilize_);

	CHECK(message);
	send(message->to_string_piece());
}

void TcpConnection::send(NetBuffer& message)
{
	DCHECK(initilize_);

	// send(NetBuffer*) function will swap the data.
	send(&message);
}

void TcpConnection::send(NetBuffer&& message)
{
	DCHECK(initilize_);

	NetBuffer buffer(std::move(message));
	send(&buffer);
}

void TcpConnection::send(const StringPiece& buffer)
{
	DCHECK(initilize_);

	// Compile-time assignment
	constexpr void(TcpConnection::*const snd)(const StringPiece&) 
		= &TcpConnection::send_in_loop;

	if (state_.load(std::memory_order_relaxed) == kConnected) {
		// FIXME: Please use weak_from_this() since C++17.
		// FIXME: as_string() is too bad.
		using containers::make_weak_bind;
		owner_loop_->run_in_own_loop(
			make_weak_bind(snd, shared_from_this(), buffer.as_string()));
	}
}

void TcpConnection::send(NetBuffer* buffer)
{
	DCHECK(initilize_);

	CHECK(buffer);

	// Compile-time assignment
	constexpr void(TcpConnection::*const snd)(const StringPiece&) 
		= &TcpConnection::send_in_loop;

	if (state_.load(std::memory_order_relaxed) == kConnected) {
		// taken_as_string() function will swap the data.
		//
		// FIXME: Please use weak_from_this() since C++17.
		// FIXME: taken_as_string() is too bad.
		using containers::make_weak_bind;
		owner_loop_->run_in_own_loop(
			make_weak_bind(snd, shared_from_this(), buffer->taken_as_string()));
	}
}

void TcpConnection::send_in_loop(const StringPiece& buffer)
{
	send_in_loop(buffer.data(), buffer.size());
}

void TcpConnection::send_in_loop(const void* data, size_t len)
{
	owner_loop_->check_in_own_loop();
	
	CHECK(data);

	ssize_t nwrote = 0;
	size_t remaining = len;
	bool fault_error = false;
	if (state_.load(std::memory_order_relaxed) == kDisconnected) {
		LOG(WARNING) << "TcpConnection::send_in_loop was disconnected, give up writing";
		return;
	}

	// If no thing in output buffer, try writing directly.
	if (!connect_channel_->is_write_event() && output_buffer_->readable_bytes() == 0) {
		nwrote = connect_socket_->write(data, len);
		if (nwrote >= 0) {
			remaining = len - nwrote;
			if (remaining == 0 && write_complete_cb_) {
				// Call the user write complete callback. Async callback.
				owner_loop_->queue_in_own_loop(
					std::bind(write_complete_cb_, shared_from_this()));
			}
		} else {
			// Write failed.
			nwrote = 0;
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				PLOG(ERROR) << "TcpConnection::send_in_loop has failed";
				if (errno == EPIPE || errno == ECONNRESET) {
					// The peer endpoint has closed.
					fault_error = true;
				}
			}
		}
	}

	DCHECK(remaining <= len);
	if (!fault_error && remaining > 0) {
		size_t history = output_buffer_->readable_bytes();
		if (history + remaining >= high_water_mark_ && history < high_water_mark_ 
			&& high_water_mark_cb_)
		{
			// Call the user high watermark callback. Async callback.
			owner_loop_->queue_in_own_loop(
				std::bind(high_water_mark_cb_, shared_from_this(), history + remaining));
		}

		// FIXME: Copy data to output_buffer_ and enable write event.
		output_buffer_->append(static_cast<const char*>(data) + nwrote, remaining);
		if (!connect_channel_->is_write_event()) {
			connect_channel_->enable_write_event();
		}
	}
}

void TcpConnection::shutdown()
{
	DCHECK(initilize_);

	StateE expected = kConnected;
	if (state_.compare_exchange_strong(expected, kDisconnecting, 
			std::memory_order_release, std::memory_order_relaxed))
	{	
		// Run in the `conn` own loop, because call `shutdown` and own loop of 
		// `conn` may not be the same thread. --- the users thread call this.
		using containers::make_weak_bind;
		owner_loop_->run_in_own_loop(
			make_weak_bind(&TcpConnection::shutdown_in_loop, shared_from_this()));
	}
}

void TcpConnection::shutdown_in_loop()
{
	owner_loop_->check_in_own_loop();
	
	// There is still data not sent completely.
	if (!connect_channel_->is_write_event()) {
		// Close write channel.
		internal::shutdown(*connect_socket_);
	}
}

void TcpConnection::force_close()
{
	DCHECK(initilize_);

	StateE expected = kConnected;
	if (state_.compare_exchange_strong(expected, kDisconnecting, 
			std::memory_order_release, std::memory_order_relaxed) || 
		expected == kDisconnecting)
	{
		using containers::make_weak_bind;
		owner_loop_->queue_in_own_loop(
			make_weak_bind(&TcpConnection::force_close_in_loop, shared_from_this()));
	}
}

void TcpConnection::force_close_with_delay(double delay_s)
{
	DCHECK(initilize_);

	StateE expected = kConnected;
	if (state_.compare_exchange_strong(expected, kDisconnecting, 
			std::memory_order_release, std::memory_order_relaxed) || 
		expected == kDisconnecting)
	{
		// Must use weak bind. Otherwise, the `conn` instance is always destroyed 
		// at the end of the delay. (`timer` storages this shared_ptr).
		using containers::make_weak_bind;
		owner_loop_->run_after(
			TimeDelta::from_seconds_d(delay_s), 
			make_weak_bind(&TcpConnection::force_close, shared_from_this()));
	}
}

void TcpConnection::force_close_in_loop()
{
	owner_loop_->check_in_own_loop();

	StateE state = state_.load(std::memory_order_relaxed);
	if (state == kConnected || state == kDisconnecting) {
		// As if we received 0 bytes in handle_read();
		handle_close();
	}
}

void TcpConnection::start_read()
{
	DCHECK(initilize_);

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&TcpConnection::start_read_in_loop, shared_from_this()));
}

void TcpConnection::stop_read()
{
	DCHECK(initilize_);

	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&TcpConnection::stop_read_in_loop, shared_from_this()));
}

void TcpConnection::start_read_in_loop()
{
	owner_loop_->check_in_own_loop();

	bool expected = false;
	if (reading_.compare_exchange_strong(expected, true, 
			std::memory_order_release, std::memory_order_relaxed) && 
		!connect_channel_->is_read_event())
	{
		connect_channel_->enable_read_event();
	}
}

void TcpConnection::stop_read_in_loop()
{
	owner_loop_->check_in_own_loop();

	bool expected = true;
	if (reading_.compare_exchange_strong(expected, false, 
			std::memory_order_release, std::memory_order_relaxed) && 
		connect_channel_->is_read_event())
	{
		connect_channel_->disable_read_event();
	}
}

void TcpConnection::connect_established()
{
	// cout << (shared_from_this().use_count()==4); // true
	// shared_from_this() +1
	// new_connection() +3

	owner_loop_->check_in_own_loop();

	// Change the Connection state.
	DCHECK(state_ == kConnecting);
	state_.store(kConnected, std::memory_order_relaxed);

	// Enable the readable event.
	connect_channel_->enable_read_event();

	// Call the user connect callback.
	connect_cb_(shared_from_this());

	DLOG(TRACE) << "TcpConnection::connect_established is called";
}

void TcpConnection::connect_destroyed()
{
	// cout << (shared_from_this().use_count()==2); // true
	// shared_from_this() +1
	// std::bind() called by remove_connection_in_loop() +1

	owner_loop_->check_in_own_loop();

	StateE expected = kConnected;
	if (state_.compare_exchange_strong(expected, kDisconnected,
			std::memory_order_release, std::memory_order_relaxed)) {
		connect_channel_->disable_all_event();

		// Call the user close callback.
		close_cb_(shared_from_this());
	}
	connect_channel_->remove();

	DLOG(TRACE) << "TcpConnection::connect_destroyed is called";
}

void TcpConnection::handle_close()
{
	// cout << (shared_from_this().use_count()==3); // true
	// shared_from_this() +1
	// WBindFuctor +1
	// ConnectionMap +1

	owner_loop_->check_in_own_loop();

	DLOG(TRACE) << "TcpConnection::handle_close the fd = " 
		<< connect_socket_->internal_fd() 
		<< " state = " << state_to_string() << " is closing";
	
	DCHECK(state_ == kConnected || state_ == kDisconnecting);
	state_.store(kDisconnected, std::memory_order_relaxed);

	connect_channel_->disable_all_event();

	// Call the user close callback.
	close_cb_(shared_from_this());

	// Call the Server finish callback.
	finish_cb_(shared_from_this());
}

void TcpConnection::handle_read(TimeStamp received_ms)
{
	owner_loop_->check_in_own_loop();

	ScopedClearLastError last_error;

	// Wrapper the ::read() system call.
	ssize_t n = input_buffer_->read_fd(connect_socket_->internal_fd());
	if (n > 0) {
		// Call the user message callback.
		message_cb_(shared_from_this(), input_buffer_.get(), received_ms);
	} else if (n == 0) {
		LOG(DEBUG) << "TcpConnection::handle_read the conntion fd=" 
			<< connect_socket_->internal_fd() 
			<< " is going to closing";

		handle_close();
	} else {
		PLOG(ERROR) << "TcpConnection::handle_read the conntion fd=" 
			<< connect_socket_->internal_fd() 
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
				// Disable the writable event. Otherwise the file descriptor will 
				// have a busy loop with writable event.
				connect_channel_->disable_write_event();

				if (write_complete_cb_) {
					// Call the user write complete callback. Async callback.
					owner_loop_->queue_in_own_loop(
						std::bind(write_complete_cb_, shared_from_this()));
				}
				if (state_.load(std::memory_order_relaxed) == kDisconnecting) {
					// After the output buffer is sent, then handling shutdown.
					shutdown_in_loop();
				}
			}
		} else {
			PLOG(ERROR) << "TcpConnection::handle_write has failed";
		}
	} else {
		LOG(DEBUG) << "TcpConnection::handle_write the conntion fd=" 
			<< connect_socket_->internal_fd()
			<< " is down, no more writing";
	}
}

void TcpConnection::handle_error()
{
	PLOG(ERROR) << "TcpConnection::handle_error the connection " 
		<< name_ << " has a error";
}

const char* TcpConnection::state_to_string() const
{
	switch (state_.load(std::memory_order_relaxed)) {
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

// Constructs an object of type TcpConnectionPtr and wraps it.
TcpConnectionPtr make_tcp_connection(EventLoop* loop,
	const std::string& name,
	SelectableFDPtr&& sockfd,
	const EndPoint& localaddr,
	const EndPoint& peeraddr)
{
	CHECK(loop && sockfd);

	TcpConnectionPtr conn(new TcpConnection(loop, 
						name, 
						std::move(sockfd), 
						localaddr, 
						peeraddr));
	conn->initialize();
	return conn;
}

}	// namespace annety
