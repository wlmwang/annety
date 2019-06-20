// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpServer.h"
#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsUtil.h"
#include "EndPoint.h"

#include <stdio.h>  // snprintf
  
namespace annety
{
TcpServer::TcpServer(EventLoop* loop,
					 const EndPoint& listenAddr,
					 const std::string& nameArg,
					 Option option)
	: owner_loop_(loop),
	 ip_port_(listenAddr.to_ip_port()),
	 name_(nameArg),
	 acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
{
	//...
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

}	// namespace annety
