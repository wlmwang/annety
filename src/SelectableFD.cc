// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SelectableFD.h"
#include "SocketsUtil.h"

namespace annety
{
SelectableFD::~SelectableFD()
{
	sockets::close(fd_);
}

}	// namespace annety
