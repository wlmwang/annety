// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpServer.h"
#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsUtil.h"
#include "EndPoint.h"
#include "SocketFD.h"
#include "TcpConnection.h"

#include <utility>
#include <stdio.h>  // snprintf

namespace annety
{
TcpServer::TcpServer(EventLoop* loop,
					 const EndPoint& addr,
					 const std::string& name,
					 Option option)
	: owner_loop_(loop), name_(name),
	  ip_port_(addr.to_ip_port()),
	  acceptor_(new Acceptor(loop, addr, option == kReusePort)),
	  connection_cb_(default_connection_callback),
	  message_cb_(default_message_callback)
{
	acceptor_->set_new_connect_callback(
		std::bind(&TcpServer::new_connection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	owner_loop_->check_in_own_thread(true);
	LOG(TRACE) << "TcpServer::~TcpServer [" << name_ << "] destructing";

	for (auto& item : connections_) {
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		// conn->get_owner_oop()->runInLoop(
		// 	std::bind(&TcpConnection::connect_destroyed, conn));
		conn->connect_destroyed();
	}
}

void TcpServer::start()
{
	if (!started_.test_and_set()) {
		// thread_pool_->start(thread_init_cb_);
		
		CHECK(!acceptor_->is_listen());
		acceptor_->listen();
	}
}

void TcpServer::new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_thread(true);

	// EventLoop* loop = thread_pool_->get_next_loop();
	EventLoop* loop = owner_loop_;
  
	char buf[128];
	::snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), ++next_conn_id_);
	std::string name = name_ + buf;

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] - new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	EndPoint localaddr(sockets::get_local_addr(sockfd->internal_fd()));
	TcpConnectionPtr conn(new TcpConnection(loop, name, std::move(sockfd),
											localaddr, peeraddr));

	connections_[name] = conn;
	
	// forward register user callback to TcpConnection
	conn->set_connection_callback(connection_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);
	conn->set_close_callback(
		std::bind(&TcpServer::remove_connection, this, _1));

	// loop->runInLoop(std::bind(&TcpConnection::connect_established, conn));
	conn->connect_established();
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn)
{
	// loop_->runInLoop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
	remove_connection_in_loop(conn);
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_thread(true);

	LOG(INFO) << "TcpServer::remove_connection_in_loop [" << name_
		<< "] - connection " << conn->name();

	size_t n = connections_.erase(conn->name());
	DCHECK(n == 1);

	// EventLoop* loop = conn->get_loop();
	// loop->queueInLoop(
	// 	std::bind(&TcpConnection::connect_destroyed, conn));
	conn->connect_destroyed();
}


}	// namespace annety
