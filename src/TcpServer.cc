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

TcpServer::TcpServer(EventLoop* loop, 
					const EndPoint& addr, 
					const std::string& name, 
					bool reuse_port)
	: owner_loop_(loop)
	, name_(name)
	, ip_port_(addr.to_ip_port())
	, acceptor_(new Acceptor(loop, addr, reuse_port))
	, workers_(new EventLoopPool(loop, name))
	, connect_cb_(default_connect_callback)
	, close_cb_(default_close_callback)
	, message_cb_(default_message_callback)
{
	LOG(DEBUG) << "TcpServer::TcpServer [" << name_ 
		<< "] server which listening on " << ip_port_ << " is constructing";
}

void TcpServer::initialize()
{
	DCHECK(!initilize_);
	initilize_ = true;

	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`acceptor_` storages this shared_ptr).
	// 1. but the server instance usually do not need to be destroyed.
	// 2. make_weak_bind() is not good for right-value yet.
	using std::placeholders::_1;
	using std::placeholders::_2;
	acceptor_->set_new_connect_callback(
		std::bind(&TcpServer::new_connection, shared_from_this(), _1, _2));
}

TcpServer::~TcpServer()
{
	DCHECK(initilize_);

	owner_loop_->check_in_own_loop();

	// Destroy all client connections.
	for (auto& item : connections_) {
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->get_owner_loop()->run_in_own_loop(
			std::bind(&TcpConnection::connect_destroyed, conn));
	}

	LOG(DEBUG) << "TcpServer::~TcpServer [" << name_ 
		<< "] server which listening on" << ip_port_ << " is destructing";
}

void TcpServer::set_thread_num(int num_threads)
{
	DCHECK(initilize_);

	CHECK(0 <= num_threads);
	workers_->set_thread_num(num_threads);
}

void TcpServer::start()
{
	DCHECK(initilize_);
	
	if (!started_.test_and_set()) {
		// Starting all event loop thread pool.
		workers_->start(thread_init_cb_);

		// Setting listening socket in current thread.
		DCHECK(!acceptor_->is_listen());
		acceptor_->listen();
	}
}

void TcpServer::new_connection(SelectableFDPtr&& peerfd, const EndPoint& peeraddr)
{
	owner_loop_->check_in_own_loop();

	// Get one EventLoop thread for NIO.
	EventLoop* worker = workers_->get_next_loop();

	EndPoint localaddr(internal::get_local_addr(*peerfd));

	std::string name = name_ + string_printf("#%s#%d", 
								ip_port_.c_str(), next_conn_id_++);

	LOG(INFO) << "TcpServer::new_connection [" << name_
		<< "] accept new connection [" << name
		<< "] from " << peeraddr.to_ip_port();

	TcpConnectionPtr conn = make_tcp_connection(
							worker, 
							name,
							std::move(peerfd),
							localaddr,
							peeraddr);
	// Copy user callbacks into TcpConnection.
	conn->set_connect_callback(connect_cb_);
	conn->set_close_callback(close_cb_);
	conn->set_message_callback(message_cb_);
	conn->set_write_complete_callback(write_complete_cb_);

	// FIXME: Please use weak_from_this() since C++17.
	// Must use weak bind. Otherwise, there is a circular reference problem 
	// of smart pointer (`connections_` storages all connections).
	using containers::_1;
	using containers::make_weak_bind;
	conn->set_finish_callback(
			make_weak_bind(&TcpServer::remove_connection, shared_from_this(), _1));

	// Storages all client connections.
	connections_[name] = conn;
	
	// Run in the `conn` own loop, because `server` and `conn` may not be the 
	// same thread. --- the `server` thread call this.
	// The `conn` will be copied by std::bind(). So conn.use_count() will 
	// become 3.
	worker->run_in_own_loop(
		std::bind(&TcpConnection::connect_established, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn)
{
	// Run in the `server` own loop, because `conn` and `server` may not be 
	// the same thread. --- the `conn` thread call this.
	// The `conn` will be copied by std::bind(). So conn.use_count() will 
	// become 4.
	using containers::make_weak_bind;
	owner_loop_->run_in_own_loop(
		make_weak_bind(&TcpServer::remove_connection_in_loop, shared_from_this(), conn));
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn)
{
	owner_loop_->check_in_own_loop();

	LOG(INFO) << "TcpServer::remove_connection_in_loop [" << name_
		<< "] remove the connection [" << conn->name() << "]";
	
	// conn.use_count() now is 2, after erase will become 1.
	// conn +1
	// ConnectionMap +1
	size_t n = connections_.erase(conn->name());
	DCHECK(n == 1);

	// Can't remove `conn` here immediately, because we are inside channel's 
	// event handling function now.  The purpose is not to let the channel's 
	// iterator fail.
	// The `conn` will be copied by std::bind(). So conn.use_count() will 
	// become 2.
	conn->get_owner_loop()->queue_in_own_loop(
		std::bind(&TcpConnection::connect_destroyed, conn));
}

// Constructs an object of type TcpServerPtr and wraps it.
TcpServerPtr make_tcp_server(EventLoop* loop, 
	const EndPoint& addr, 
	const std::string& name, 
	bool reuse_port)
{
	DCHECK(loop);
	
	TcpServerPtr srv(new TcpServer(loop, addr, name, reuse_port));
	srv->initialize();
	return srv;
}

}	// namespace annety
