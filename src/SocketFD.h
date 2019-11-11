// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_SOCKET_FD_H_
#define ANT_SOCKET_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

#include <sys/socket.h>

namespace annety
{
class SocketFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	SocketFD() = delete;
	SocketFD(sa_family_t family, bool nonblock, bool cloexec);
	
private:
	DISALLOW_COPY_AND_ASSIGN(SocketFD);
};

}	// namespace annety

#endif	// _ANT_SOCKET_FD_H_
