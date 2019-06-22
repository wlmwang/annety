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
	: owner_loop_(loop),
	 ip_port_(addr.to_ip_port()),
	 name_(name),
	 acceptor_(new Acceptor(loop, addr, option == kReusePort))
{
	acceptor_->set_new_connection_callback(
		std::bind(&TcpServer::new_connection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	owner_loop_->check_in_own_thread(true);

	LOG(TRACE) << "TcpServer::~TcpServer [" << name_ << "] destructing";
}

void TcpServer::start()
{
	if (started_ == 0) {
		CHECK(!acceptor_->listenning());
		acceptor_->listen();
	}
	started_ = 1;
}

void TcpServer::new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_thread(true);

	EventLoop* loop = owner_loop_;
  
	char buf[128];
	::snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_conn_id_++);
	std::string name = name_ + buf;

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] - new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	EndPoint localaddr(sockets::get_local_addr(sockfd->internal_fd()));
	TcpConnectionPtr conn(new TcpConnection(loop, name, std::move(sockfd),
											localaddr, peeraddr));

	connections_[name] = conn;
	
	conn->set_close_callback(
		std::bind(&TcpServer::remove_connection, this, _1));

	conn->connect_established();
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn)
{
	remove_connection_in_loop(conn);

	// FIXME: unsafe
	// loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_thread(true);

	LOG(INFO) << "TcpServer::removeConnectionInLoop [" << name_
		<< "] - connection " << conn->name();

	size_t n = connections_.erase(conn->name());
	(void)n;
	DCHECK(n == 1);

	conn->connect_destroyed();

	// EventLoop* ioLoop = conn->getLoop();
	// ioLoop->queueInLoop(
	// 	std::bind(&TcpConnection::connectDestroyed, conn));
}


}	// namespace annety
