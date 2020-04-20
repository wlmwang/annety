// By: wlmwang
// Date: Jun 30 2019

#include "TcpClient.h"
#include "Logging.h"
#include "SocketsUtil.h"
#include "SocketFD.h"
#include "EndPoint.h"
#include "Connector.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "strings/StringPrintf.h"
#include "containers/Bind.h"

#include <unistd.h>		// ::usleep

namespace annety
{
namespace internal {
// Consistent with the function of the same name in the TcpServer.cc file.
// Redefining the reduction of dependencies here.
static struct sockaddr_in6 get_local_addr(const SelectableFD& sfd)
{
	return sockets::get_local_addr(sfd.internal_fd());
}

}	// namespace internal

TcpClient::TcpClient(EventLoop* loop, const EndPoint& addr, const std::string& name)
	: owner_loop_(loop)
	, name_(name)
	, ip_port_(addr.to_ip_port())
	, connector_(new Connector(owner_loop_, addr))
	, connect_cb_(default_connect_callback)
	, close_cb_(default_close_callback)
	, message_cb_(default_message_callback)
{
	LOG(DEBUG) << "TcpClient::TcpClient [" << name_ 
		<< "] client is constructing which connecting to " << ip_port_;
}

void TcpClient::initialize()
{
	DCHECK(!initilize_);
	initilize_ = true;

	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`connector_` storages this shared_ptr).
	// 1. but the client instance usually do not need to be destroyed.
	// 2. make_weak_bind() is not good for right-value yet.
	using std::placeholders::_1;
	using std::placeholders::_2;
	connector_->set_new_connect_callback(
		std::bind(&TcpClient::new_connection, shared_from_this(), _1, _2));
	connector_->set_error_connect_callback(
			std::bind(&TcpClient::handle_retry, shared_from_this()));
}

TcpClient::~TcpClient()
{
	DCHECK(initilize_);

	LOG(DEBUG) << "TcpClient::~TcpClient [" << name_ 
		<< "] client is destructing which be connected to " << ip_port_;

	TcpConnectionPtr conn;
	{
		AutoLock locked(lock_);
		conn = connection_;
	}

	if (conn) {
		connect_destroyed();
	} else {
		connector_->stop();

		// FIXME: *HACK* FOR `connection` close FINISH.
		// Waiting for TCP wavehand protocol(Four-Way Wavehand) to complete,
		// so that we can have a chance to call `connect_cb_`.
		owner_loop_->run_after(1, std::bind([](ConnectorPtr& connector) {
			// remove connector.
			connector.reset();
		}, connector_));
	}
}

void TcpClient::connect()
{
	DCHECK(initilize_);

	if (!connect_) {
		connect_ = true;
		connector_->start();
	}
}

void TcpClient::disconnect()
{
	DCHECK(initilize_);

	if (connect_) {
		{
			AutoLock locked(lock_);
			if (connection_) {
				connection_->shutdown();
			}
		}
		connect_ = false;
	}
}

void TcpClient::stop()
{
	DCHECK(initilize_);

	if (connect_) {
		connector_->stop();
		connect_ = false;
	}
}

void TcpClient::new_connection(SelectableFDPtr&& sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	EndPoint localaddr(internal::get_local_addr(*sockfd));

	LOG(INFO) << "TcpClient::new_connection [" << name_
		<< "] connect new connection [" << name
		<< "] from " << localaddr.to_ip_port();
	
	TcpConnectionPtr conn = make_tcp_connection(
								owner_loop_, 
								name,
								std::move(sockfd),
								localaddr,
								peeraddr);
	// Copy user callbacks into TcpConnection.
	conn->set_connect_callback(connect_cb_);
	conn->set_close_callback(close_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);
	
	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`connection_` storages conn).
	using containers::_1;
	using containers::make_weak_bind;
	conn->set_finish_callback(
			make_weak_bind(&TcpClient::remove_connection, shared_from_this(), _1));
	
	{
		AutoLock locked(lock_);
		connection_ = conn;
	}
	conn->connect_established();
}

void TcpClient::remove_connection(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_loop();
	DCHECK(owner_loop_ == conn->get_owner_loop());
	
	// reset the `connection_` object.
	{
		AutoLock locked(lock_);
		DCHECK(connection_ == conn);
		connection_.reset();
	}

	// Can't remove `conn` here immediately, because we are inside channel's 
	// event handling function now.
	// The `conn` will be copied by std::bind().
	owner_loop_->queue_in_own_loop(
		std::bind(&TcpConnection::connect_destroyed, conn));
	
	if (retry_ && connect_) {
		LOG(DEBUG) << "TcpClient::remove_connection the [" << name_ 
			<< "] client on " << ip_port_ << ", wait for reconnect now";
		
		// retry.
		if (connector_) {
			connector_->restart();
		}
	}
}

void TcpClient::handle_retry()
{
	DCHECK(initilize_);

	connect_destroyed();

	// retry server.
	if (retry_ && connect_) {
		LOG(DEBUG) << "TcpClient::handle_retry the [" << name_ 
			<< "] client on " << ip_port_ << ", wait for reconnect now";

		if (connector_) {
			// The `connector_` will be copied by std::bind().
			owner_loop_->queue_in_own_loop(
				std::bind(&Connector::retry, connector_));
		}
	} else {
		// FIXME: exit program when connect fail??
		// exit(0);
	}
}

void TcpClient::connect_destroyed()
{
	DCHECK(initilize_);
	
	bool unique = false;
	TcpConnectionPtr conn;
	{
		AutoLock locked(lock_);
		unique = connection_.unique();
		conn = connection_;
	}
	
	LOG(DEBUG) << "TcpClient::connect_destroyed the conn address is " 
		<< conn.get() << ", unique is " << unique;

	if (conn) {
		DCHECK(owner_loop_ == conn->get_owner_loop());

		// FIXME: *Not 100% safe*, if we are in different thread.
		using std::placeholders::_1;
		CloseCallback cb = std::bind([](EventLoop* loop, const TcpConnectionPtr& conn) {
			// remove conn.
			loop->queue_in_own_loop(std::bind(&TcpConnection::connect_destroyed, conn));
		}, owner_loop_, _1);

		// close callback.
		owner_loop_->run_in_own_loop(
			std::bind(&TcpConnection::set_close_callback, conn, cb));
		
		if (unique) {
			conn->force_close();
		}
	}
}

// Constructs an object of type TcpClientPtr and wraps it.
TcpClientPtr make_tcp_client(EventLoop* loop, 
	const EndPoint& addr, 
	const std::string& name)
{
	DCHECK(loop);

	TcpClientPtr crv(new TcpClient(loop, addr, name));
	crv->initialize();
	return crv;
}

}	// namespace annety
