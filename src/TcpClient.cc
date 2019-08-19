// Refactoring: Anny Wang
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

#include <unistd.h>	// usleep

namespace annety
{
namespace internal
{
// Consistent with the function of the same name in the TcpServer.cc file.
// Redefining the reduction of dependencies here
static struct sockaddr_in6 get_local_addr(const SelectableFD& sfd)
{
	return sockets::get_local_addr(sfd.internal_fd());
}

void remove_connection(EventLoop* loop, const TcpConnectionPtr& conn)
{
	loop->queue_in_own_loop(std::bind(&TcpConnection::connect_destroyed, conn));
}
void remove_connector(ConnectorPtr& connector)
{
	// connector.reset();
}

}	// namespace internal

TcpClient::TcpClient(EventLoop* loop,
		const EndPoint& addr,
		const std::string& name)
	: owner_loop_(loop)
	, name_(name)
	, ip_port_(addr.to_ip_port())
	, connector_(new Connector(owner_loop_, addr))
	, connect_cb_(default_connect_callback)
	, message_cb_(default_message_callback)
{
	LOG(DEBUG) << "TcpClient::TcpClient [" << name_ 
		<< "] client is constructing which connecting to " << ip_port_;
}

TcpClient::~TcpClient()
{
	DCHECK(initilize_);

	LOG(DEBUG) << "TcpClient::~TcpClient [" << name_ 
		<< "] client is destructing which be connected to " << ip_port_;

	// FIXME: Never run the code, because the conn have circular reference with 
	// TcpClient. --- shared_from_this() by bind() in initialize()
	// And the stop() method use usleep to wait remove_connection() finish.
	close_connection();

	if (connector_) {
		LOG(DEBUG) << "TcpClient::~TcpClient [" << name_ 
			<< "] connector is destructing which address is " << connector_.get();

		connector_->stop();
		owner_loop_->queue_in_own_loop(
			std::bind(&internal::remove_connector, connector_));
	}
}

void TcpClient::initialize()
{
	CHECK(!initilize_);
	initilize_ = true;

	// FIXME: make_weak_bind() have a bug with right reference
	// using containers::_1;
	// using containers::_2;
	// using containers::make_weak_bind;
	// connector_->set_new_connect_callback(
	// 	make_weak_bind(&TcpClient::new_connection, shared_from_this(), _1, _2));

	// FIXME: unsafe
	// It is best to manually manage the lifecycle of connector_
	connector_->set_new_connect_callback(
		std::bind(&TcpClient::new_connection, this, _1, _2));
	connector_->set_error_connect_callback(
			std::bind(&TcpClient::error_connect, this));
}

void TcpClient::connect()
{
	CHECK(initilize_);

	// FIXME: check state
	connect_ = true;
	connector_->start();
}

void TcpClient::disconnect()
{
	CHECK(initilize_);

	connect_ = false;
	{
		AutoLock locked(lock_);
		if (connection_) {
			connection_->shutdown();
		}
	}
}

void TcpClient::error_connect()
{
	CHECK(initilize_);

	if (error_cb_) {
		error_cb_();
	}

	close_connection();

	if (retry_ && connect_) {
		LOG(DEBUG) << "TcpClient::error_connect the [" << name_ 
			<< "] client on " << ip_port_ << ", wait for reconnect now";

		if (connector_) {
			owner_loop_->queue_in_own_loop(
				std::bind(&Connector::retry, connector_));
		}
	} else {
		// FIXME: exit program when connect failure??
		// exit(0);
	}
}

void TcpClient::stop()
{
	CHECK(initilize_);

	if (connect_) {
		disconnect();
	}
	connector_->stop();

	// FIXME: HACK FOR disconnect() FINISH
	// Waiting for TCP wavehand protocol(Four-Way Wavehand) to complete,
	// so that we can have a chance to call connect_cb_
	if (!owner_loop_->is_in_own_loop()) {
		usleep(100 * 1000);
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
	
	// FIXME: poll with zero timeout to double confirm the new connection
	TcpConnectionPtr conn = make_tcp_connection(
								owner_loop_, 
								name,
								std::move(sockfd),
								localaddr,
								peeraddr
							);
	// transfer register user callbacks to TcpConnection
	conn->set_connect_callback(connect_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);
	
	// must be manually release conn object (@see TcpClient::remove_connection())
	using containers::_1;
	using containers::make_bind;
	using containers::make_weak_bind;
	conn->set_close_callback(
			make_bind(&TcpClient::remove_connection, shared_from_this(), _1));
	
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
	
	{
		AutoLock locked(lock_);
		DCHECK(connection_ == conn);
		connection_.reset();
	}

	// can't remove conn here immediately, because we are inside Channel::handle_event
	owner_loop_->queue_in_own_loop(std::bind(&TcpConnection::connect_destroyed, conn));
	
	if (retry_ && connect_) {
		LOG(DEBUG) << "TcpClient::remove_connection the [" << name_ 
			<< "] client on " << ip_port_ << ", wait for reconnect now";
		
		if (connector_) {
			connector_->restart();
		}
	}
}

void TcpClient::close_connection()
{
	CHECK(initilize_);
	
	bool unique = false;
	TcpConnectionPtr conn;
	{
		AutoLock locked(lock_);
		conn = connection_;
		unique = connection_.unique();
	}
	
	LOG(DEBUG) << "TcpClient::close_connection the conn address is " 
		<< conn.get() << ", unique is " << unique;

	if (conn) {
		DCHECK(owner_loop_ == conn->get_owner_loop());

		// *Not 100% safe*, if we are in different thread
		CloseCallback cb = std::bind(&internal::remove_connection, owner_loop_, _1);
		owner_loop_->run_in_own_loop(
			std::bind(&TcpConnection::set_close_callback, conn, cb));
		
		if (unique) {
			conn->force_close();
		}
	}
}

// constructs an object of type TcpClientPtr and wraps it
TcpClientPtr make_tcp_client(EventLoop* loop, const EndPoint& addr, const std::string& name)
{
	CHECK(loop);

	TcpClientPtr crv(new TcpClient(loop, addr, name));
	crv->initialize();
	return crv;
}

}	// namespace annety
