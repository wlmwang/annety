// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TcpServer.h"
#include "Logging.h"
#include "SocketsUtil.h"
#include "SocketFD.h"
#include "EndPoint.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "EventLoopPool.h"
#include "strings/StringPrintf.h"

#include <utility>

namespace annety
{
namespace internal
{
struct sockaddr_in6 get_local_addr(const SelectableFD& sfd)
{
	return sockets::get_local_addr(sfd.internal_fd());
}

}	// namespace internal

TcpServer::TcpServer(EventLoop* loop,
					 const EndPoint& addr,
					 const std::string& name,
					 Option option)
	: owner_loop_(loop)
	, name_(name)
	, ip_port_(addr.to_ip_port())
	, acceptor_(new Acceptor(loop, addr, option == kReusePort))
	, loop_pool_(new EventLoopPool(loop, name))
	, connect_cb_(default_connect_callback)
	, message_cb_(default_message_callback)
{
	LOG(DEBUG) << "TcpServer::TcpServer [" << name_ 
		<< "] is constructing which listening on " << ip_port_;

	acceptor_->set_new_connect_callback(
		std::bind(&TcpServer::new_connection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	owner_loop_->check_in_own_loop();
	LOG(DEBUG) << "TcpServer::~TcpServer " << name_ 
		<< " is destructing which listening on" << ip_port_;

	for (auto& item : connections_) {
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->get_owner_loop()->run_in_own_loop(
			std::bind(&TcpConnection::connect_destroyed, conn));
	}
	// FIXME: there should be a join_all() called here
}

void TcpServer::set_thread_num(int num_threads)
{
	CHECK(0 <= num_threads);
	loop_pool_->set_thread_num(num_threads);
}

void TcpServer::start()
{
	if (!started_.test_and_set()) {
		// 1. starting all thread pool
		loop_pool_->start(thread_init_cb_);

		// 2. setting listen-socket on current thread
		CHECK(!acceptor_->is_listen());
		acceptor_->listen();
	}
}

void TcpServer::new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	EventLoop* loop = loop_pool_->get_next_loop();
  	
	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	EndPoint localaddr(internal::get_local_addr(*sockfd));

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] accept new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	TcpConnectionPtr conn(new TcpConnection(loop, name, std::move(sockfd),
											localaddr, peeraddr));

	connections_[name] = conn;

	// transfer register user callbacks to TcpConnection
	conn->initialize();
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
		<< "] remove the connection [" << conn->name() << "]";

	size_t n = connections_.erase(conn->name());
	DCHECK(n == 1);

	// Can't remove conn here, because we are inside Channel::handle_event
	conn->get_owner_loop()->queue_in_own_loop(
		std::bind(&TcpConnection::connect_destroyed, conn));
}

}	// namespace annety
