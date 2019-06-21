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

void TcpServer::new_connection(const SocketFD& conn_sfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_thread(true);

	EventLoop* io_loop = owner_loop_;
  
	char buf[64];
	::snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_connId_++);
	std::string conn_name = name_ + buf;

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] - new connection [" << conn_name
		<< "] from " << peeraddr.to_ip_port();

	EndPoint localaddr(sockets::get_local_addr(conn_sfd.internal_fd()));
	TcpConnectionPtr conn(new TcpConnection(io_loop,
											conn_name,
											conn_sfd,
											localaddr,
											peeraddr));
	
	connections_[conn_name] = conn;

	LOG(INFO) << "use cout" << conn.use_count();

	conn->connect_established();
}

}	// namespace annety
