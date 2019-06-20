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
void bind(const SelectableFD& sfd, const EndPoint& addr) {
	sockets::bind(sfd.internal_fd(), addr.get_sock_addr());
}

void listen(const SelectableFD& sfd) {
	sockets::listen(sfd.internal_fd());
}

int accept(const SelectableFD& sfd, EndPoint& peeraddr) {
	struct sockaddr_in6 addr;
	::memset(&addr, 0, sizeof addr);

	int connfd = sockets::accept(sfd.internal_fd(), &addr);
	if (connfd >= 0) {
		peeraddr.set_sock_addr_inet6(addr);
	}
	return connfd;
}

}	// namespace internal

Acceptor::Acceptor(EventLoop* loop, const EndPoint& listenAddr, bool reuseport)
	: owner_loop_(loop),
	  accept_socket_(sockets::socket(listenAddr.family(), true, true)),
	  accept_channel_(owner_loop_, &accept_socket_),
	  listenning_(false)
{
	// listen socket
	internal::bind(accept_socket_, listenAddr);
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

	EndPoint peerAddr;
	int connfd = internal::accept(accept_socket_, peerAddr);
	if (connfd >= 0) {
		LOG(TRACE) << "Accepts of " << peerAddr.to_ip_port();

		// connect handler
		if (new_connect_cb_) {
			new_connect_cb_(connfd, peerAddr);
		} else {
			sockets::close(connfd);
		}
	} else {
		LOG(ERROR) << "in Acceptor::handle_read";

		// Read the section named "The special problem of
		// accept()ing when you can't" in libev's doc.
		// By Marc Lehmann, author of libev.
		if (errno == EMFILE) {
			// ::close(idleFd_);
			// idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
			// ::close(idleFd_);
			// idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
		}
	}
}

}	// namespace annety
