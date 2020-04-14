// By: wlmwang
// Date: Jun 17 2019

#include "Acceptor.h"
#include "Logging.h"
#include "EndPoint.h"
#include "SocketFD.h"
#include "SocketsUtil.h"
#include "Channel.h"
#include "EventLoop.h"

#include <utility>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace annety
{
namespace internal
{
// specific listen socket related function interfaces
int bind(const SelectableFD& sfd, const EndPoint& addr)
{
	return sockets::bind(sfd.internal_fd(), addr.get_sockaddr());
}
int listen(const SelectableFD& sfd)
{
	return sockets::listen(sfd.internal_fd());
}
int accept(const SelectableFD& sfd, EndPoint& peeraddr)
{
	struct sockaddr_in6 addr;
	::memset(&addr, 0, sizeof addr);
	int connfd = sockets::accept(sfd.internal_fd(), &addr);
	if (connfd >= 0) {
		peeraddr.set_sockaddr_in(addr);
	}
	return connfd;
}
void set_reuse_addr(const SelectableFD& sfd, bool on)
{
	sockets::set_reuse_addr(sfd.internal_fd(), on);
}
void set_reuse_port(const SelectableFD& sfd, bool on)
{
	sockets::set_reuse_port(sfd.internal_fd(), on);
}

}	// namespace internal

Acceptor::Acceptor(EventLoop* loop, const EndPoint& addr, bool reuseport)
	: owner_loop_(loop)
	, listen_socket_(new SocketFD(addr.family(), true, true))
	, listen_channel_(new Channel(owner_loop_, listen_socket_.get()))
	, idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
	DLOG(TRACE) << "Acceptor::Acceptor the [" <<  addr.to_ip_port() << "] of"
		<< " fd=" << listen_socket_->internal_fd() << " is constructing";

	// socket bind().
	internal::set_reuse_addr(*listen_socket_, true);
	internal::set_reuse_port(*listen_socket_, reuseport);
	internal::bind(*listen_socket_, addr);

	listen_channel_->set_read_callback(std::bind(&Acceptor::handle_read, this));
}

Acceptor::~Acceptor()
{
	DLOG(TRACE) << "Acceptor::~Acceptor" << " fd=" 
		<< listen_socket_->internal_fd() << " is destructing";
	
	listen_channel_->disable_all_event();
	listen_channel_->remove();
}

void Acceptor::listen()
{
	owner_loop_->check_in_own_loop();
	
	// socket listen().
	listen_ = true;
	internal::listen(*listen_socket_);
	listen_channel_->enable_read_event();
}

void Acceptor::handle_read()
{
	owner_loop_->check_in_own_loop();

	EndPoint peeraddr;	// client's EndPoint
	int connfd = internal::accept(*listen_socket_, peeraddr);
	if (connfd >= 0) {
		DLOG(TRACE) << "Acceptor::handle_read the peer " << peeraddr.to_ip_port() 
				<< ", fd " << connfd << " is accepting";

		// Make a new connection sock of socketFD.  `sockfd` is a smart point 
		// of std::unique_ptr.
		SelectableFDPtr sockfd(new SocketFD(connfd));
		if (new_connect_cb_) {
			new_connect_cb_(std::move(sockfd), peeraddr);
		} else {
			// `sockfd` is std::unique_ptr, so no need to delete or close(fd) here.
		}
	} else {
		PLOG(ERROR) << "Acceptor::handle_read was failed";

		// Read the section named "The special problem of accept()ing when you 
		// can't" in libev's doc.
		// By Marc Lehmann, author of libev.
		if (errno == EMFILE) {
			// the file descriptor is exhausted.
			DPCHECK(::close(idle_fd_) == 0);
			DPCHECK(::close(::accept(listen_socket_->internal_fd(), NULL, NULL)) == 0);
			idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
		}
	}
}

}	// namespace annety
