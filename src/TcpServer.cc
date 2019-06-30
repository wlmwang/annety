// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpServer.h"
#include "Logging.h"
#include "StringPrintf.h"
#include "SocketsUtil.h"
#include "SocketFD.h"
#include "EndPoint.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <utility>

namespace annety
{
TcpServer::TcpServer(EventLoop* loop,
					 const EndPoint& addr,
					 const std::string& name,
					 Option option)
	: owner_loop_(loop),
	  name_(name),
	  ip_port_(addr.to_ip_port()),
	  acceptor_(new Acceptor(loop, addr, option == kReusePort)),
	  thread_pool_(new EventLoopThreadPool(loop, name)),
	  connect_cb_(default_connect_callback),
	  message_cb_(default_message_callback)
{
	acceptor_->set_new_connect_callback(
		std::bind(&TcpServer::new_connection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	owner_loop_->check_in_own_loop();
	LOG(TRACE) << "TcpServer::~TcpServer [" << name_ << "] is destructing";

	for (auto& item : connections_) {
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->get_owner_loop()->run_in_own_loop(
			std::bind(&TcpConnection::connect_destroyed, conn));
	}
}

void TcpServer::set_thread_num(int num_threads)
{
	CHECK(0 <= num_threads);
	thread_pool_->set_thread_num(num_threads);
}

void TcpServer::start()
{
	if (!started_.test_and_set()) {
		// starting all thread
		thread_pool_->start(thread_init_cb_);

		// setting current thread listen
		CHECK(!acceptor_->is_listen());
		acceptor_->listen();
	}
}

void TcpServer::new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	EventLoop* loop = thread_pool_->get_next_loop();
  	
	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] accept new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	EndPoint localaddr(sockets::get_local_addr(sockfd->internal_fd()));
	TcpConnectionPtr conn(new TcpConnection(loop, name, std::move(sockfd),
											localaddr, peeraddr));

	connections_[name] = conn;
	
	// transfer register user callbacks to TcpConnection
	conn->set_connect_callback(connect_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);
	conn->set_close_callback(
		std::bind(&TcpServer::remove_connection, this, _1));

	loop->run_in_own_loop(std::bind(&TcpConnection::connect_established, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn)
{
	owner_loop_->run_in_own_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_loop();

	LOG(INFO) << "TcpServer::remove_connection_in_loop [" << name_
		<< "] - connection [" << conn->name() << "]";

	size_t n = connections_.erase(conn->name());
	DCHECK(n == 1);

	conn->get_owner_loop()->queue_in_own_loop(
		std::bind(&TcpConnection::connect_destroyed, conn));
}

}	// namespace annety
