// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "Acceptor.h"

#include "Logging.h"
#include "EndPoint.h"
#include "SocketsUtil.h"
#include "EventLoop.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace annety
{
namespace internal
{
int socket(const EndPoint& addr) {
	return sockets::socket(addr.family(), true, true);
}
void bind(const SelectableFD& sfd, const EndPoint& addr) {
	sockets::bind(sfd.internal_fd(), addr.get_sockaddr());
}
void listen(const SelectableFD& sfd) {
	sockets::listen(sfd.internal_fd());
}
int accept(const SelectableFD& sfd, EndPoint& peeraddr) {
	struct sockaddr_in6 addr;
	::memset(&addr, 0, sizeof addr);
	int connfd = sockets::accept(sfd.internal_fd(), &addr);
	if (connfd >= 0) {
		peeraddr.set_sockaddr_in6(addr);
	}
	return connfd;
}
void close(const SelectableFD& sfd) {
	sockets::close(sfd.internal_fd());
}
void set_reuse_addr(const SelectableFD& sfd, bool on) {
	sockets::set_reuse_addr(sfd.internal_fd(), on);
}

void set_reuse_port(const SelectableFD& sfd, bool on) {
	sockets::set_reuse_port(sfd.internal_fd(), on);
}

}	// namespace internal

Acceptor::Acceptor(EventLoop* loop, const EndPoint& addr, bool reuseport)
	: owner_loop_(loop),
	  accept_socket_(internal::socket(addr)),
	  accept_channel_(owner_loop_, &accept_socket_)
{
	internal::set_reuse_addr(accept_socket_, true);
	internal::set_reuse_port(accept_socket_, reuseport);
	
	internal::bind(accept_socket_, addr);

	accept_channel_.set_read_callback(std::bind(&Acceptor::handle_read, this));
}

Acceptor::~Acceptor()
{
	accept_channel_.disable_all_event();
	accept_channel_.remove();
}

void Acceptor::listen()
{
	owner_loop_->check_in_own_thread(true);
	
	listenning_ = true;
	internal::listen(accept_socket_);
	accept_channel_.enable_read_event();
}

void Acceptor::handle_read()
{
	owner_loop_->check_in_own_thread(true);

	EndPoint peeraddr;
	int connfd = internal::accept(accept_socket_, peeraddr);
	if (connfd >= 0) {
		LOG(TRACE) << "Accepts of " << connfd << "|" << peeraddr.to_ip_port();

		SocketFD conn_sfd(connfd);
		if (new_connection_cb_) {
			new_connection_cb_(conn_sfd, peeraddr);
		} else {
			internal::close(conn_sfd);
		}
	} else {
		LOG(ERROR) << "Acceptor::handle_read";
		if (errno == EMFILE) {
			//...
		}
	}
}

}	// namespace annety
