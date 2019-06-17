// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_SOCKET_FD_H_
#define ANT_SOCKET_FD_H_

#include "Macros.h"

namespace annety {
class SocketFD {
public:
	explicit SocketFD(bool nonblock = true, bool cloexec = true);
	explicit SocketFD(int sckfd) : fd_(sckfd) {}

	~SocketFD();

	int internal_fd() const {
		return fd_;
	}

private:
	int fd_;

	DISALLOW_COPY_AND_ASSIGN(SocketFD);
};

}	// namespace annety

#endif	// _ANT_SOCKET_FD_H_
