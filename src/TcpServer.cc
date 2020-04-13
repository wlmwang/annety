// By: wlmwang
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
#include "containers/Bind.h"

#include <utility>

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

TcpServer::TcpServer(EventLoop* loop, const EndPoint& addr, const std::string& name, bool reuse_port)
	: owner_loop_(loop)
	, name_(name)
	, ip_port_(addr.to_ip_port())
	, acceptor_(new Acceptor(loop, addr, reuse_port))
	, loop_pool_(new EventLoopPool(loop, name))
	, connect_cb_(default_connect_callback)
	, message_cb_(default_message_callback)
{
	LOG(DEBUG) << "TcpServer::TcpServer [" << name_ 
		<< "] server is constructing which listening on " << ip_port_;
}

TcpServer::~TcpServer()
{
	DCHECK(initilize_);

	owner_loop_->check_in_own_loop();
	LOG(DEBUG) << "TcpServer::~TcpServer [" << name_ 
		<< "] server is destructing which listening on" << ip_port_;

	for (auto& item : connections_) {
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->get_owner_loop()->run_in_own_loop(
			std::bind(&TcpConnection::connect_destroyed, conn));
	}
}

void TcpServer::initialize()
{
	CHECK(!initilize_);
	initilize_ = true;

	acceptor_->set_new_connect_callback(
		std::bind(&TcpServer::new_connection, shared_from_this(), _1, _2));
}

void TcpServer::set_thread_num(int num_threads)
{
	CHECK(initilize_);

	CHECK(0 <= num_threads);
	loop_pool_->set_thread_num(num_threads);
}

void TcpServer::start()
{
	CHECK(initilize_);
	
	if (!started_.test_and_set()) {
		// starting all thread pool
		loop_pool_->start(thread_init_cb_);

		// setting listening socket in current thread
		CHECK(!acceptor_->is_listen());
		acceptor_->listen();
	}
}

void TcpServer::new_connection(SelectableFDPtr&& sockfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	// Get one EventLoop thread for NIO
	EventLoop* loop = loop_pool_->get_next_loop();
  	
	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	EndPoint localaddr(internal::get_local_addr(*sockfd));

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] accept new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	TcpConnectionPtr conn = make_tcp_connection(
								loop, 
								name,
								std::move(sockfd),
								localaddr,
								peeraddr);
	// copy user callbacks into TcpConnection.
	conn->set_connect_callback(connect_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);

	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer.
	using containers::_1;
	using containers::make_weak_bind;
	conn->set_close_callback(
			make_weak_bind(&TcpServer::remove_connection, shared_from_this(), _1));

	connections_[name] = conn;

	loop->run_in_own_loop(std::bind(&TcpConnection::connect_established, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn)
{
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&TcpServer::remove_connection_in_loop, shared_from_this(), conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_loop();

	LOG(INFO) << "TcpServer::remove_connection_in_loop [" << name_
		<< "] remove the connection [" << conn->name() << "]";

	size_t n = connections_.erase(conn->name());
	DCHECK(n == 1);

	// Can't remove conn here immediately, because we are inside Channel::handle_event now.
	// The purpose is not to let the channel's iterator fail.
	conn->get_owner_loop()->queue_in_own_loop(
		std::bind(&TcpConnection::connect_destroyed, conn));
}

// constructs an object of type TcpServerPtr and wraps it
TcpServerPtr make_tcp_server(EventLoop* loop, const EndPoint& addr, const std::string& name, bool reuse_port)
{
	CHECK(loop);
	
	TcpServerPtr srv(new TcpServer(loop, addr, name, reuse_port));
	srv->initialize();
	return srv;
}

}	// namespace annety
