// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SocketFD.h"
#include "Logging.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>  // socket

namespace annety {
SocketFD::SocketFD(bool nonblock, bool cloexec) {
	int flags = 0;
	if (nonblock) {
		flags |= SOCK_NONBLOCK;
	}
	if (cloexec) {
		flags |= SOCK_CLOEXEC;
	}
	flags |= SOCK_STREAM;

	// PF_UNIX/PF_INET6/PF_INET=AF_INET
	fd_ = ::socket(AF_INET, flags, IPPROTO_TCP);
	PLOG_IF(ERROR, fd_ < 0) << "::socket failed";
}

SocketFD::~SocketFD() {
	int ret = ::close(fd_);
	PLOG_IF(ERROR, ret < 0) << "::close failed";
}

}	// namespace annety
