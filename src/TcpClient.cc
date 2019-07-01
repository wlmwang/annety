// Refactoring: Anny Wang
// Date: Jun 30 2019

#include "TcpClient.h"
#include "Logging.h"
#include "StringPrintf.h"
#include "SocketsUtil.h"
#include "SocketFD.h"
#include "EndPoint.h"
#include "Connector.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

namespace annety
{
namespace internal
{
// define in TcpServer.cc
struct sockaddr_in6 get_local_addr(const SelectableFD& sfd);
}	// namespace internal

TcpClient::TcpClient(EventLoop* loop,
		const EndPoint& addr,
		const std::string& name)
	: owner_loop_(loop),
	  name_(name),
	  ip_port_(addr.to_ip_port()),
	  connector_(new Connector(owner_loop_, addr)),
	  connect_cb_(default_connect_callback),
	  message_cb_(default_message_callback)
{
	connector_->set_new_connect_callback(
		std::bind(&TcpClient::new_connection, this, _1, _2));
}

TcpClient::~TcpClient()
{
	// LOG(TRACE) << "TcpClient::~TcpClient [" << name_ << "] is destructing";

	// TcpConnectionPtr conn;
	// bool unique = false;
	// {
	// 	AutoLock locked(lock_);
	// 	unique = connection_.unique();
	// 	conn = connection_;
	// }

	// if (conn)
	// {
	// 	DCHECK(owner_loop_ == conn->get_own_loop());
	// 	// FIXME: not 100% safe, if we are in different thread
	// 	CloseCallback cb = std::bind(&detail::remove_connection, owner_loop_, _1);
	// 	owner_loop_->run_in_own_loop(
	// 		std::bind(&TcpConnection::set_close_callback, conn, cb));
	// 	if (unique) {
	// 		conn->force_close();
	// 	}
	// } else {
	// 	connector_->stop();
	// 	// FIXME: HACK
	// 	// owner_loop_->runAfter(1, std::bind(&detail::remove_connector, connector_));
	// }
}

void TcpClient::connect()
{
	LOG(INFO) << "TcpClient::connect[" << name_ << "] - connecting to " << ip_port_;
	connect_ = true;
	connector_->start();
}

void TcpClient::disconnect()
{
	connect_ = false;
	{
		AutoLock locked(lock_);
		if (connection_) {
			connection_->shutdown();
		}
	}
}

void TcpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void TcpClient::new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	EndPoint localaddr(internal::get_local_addr(*sockfd));

	LOG(INFO) << "TcpClient::new_connection [" << name_
		<< "] connect new connection [" << name
		<< "] from " << localaddr.to_ip_port();

	TcpConnectionPtr conn(new TcpConnection(owner_loop_, name, std::move(sockfd),
											localaddr, peeraddr));

	// transfer register user callbacks to TcpConnection
	conn->set_connect_callback(connect_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);
	conn->set_close_callback(
		std::bind(&TcpClient::remove_connection, this, _1));
	
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
	owner_loop_->queue_in_own_loop(std::bind(&TcpConnection::connect_destroyed, conn));
	
	if (retry_ && connect_) {
		LOG(INFO) << "TcpClient::connect[" << name_ << "] - reconnecting to " << ip_port_;
		connector_->restart();
	}
}

}	// namespace annety