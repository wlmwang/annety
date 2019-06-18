// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SelectableFD.h"
#include "Logging.h"

#include <unistd.h> // close

namespace annety {
SelectableFD::~SelectableFD() {
	int ret = ::close(fd_);
	PLOG_IF(ERROR, ret < 0) << "::close failed";
}

}	// namespace annety
