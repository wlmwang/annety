// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TIMER_FD_H_
#define ANT_TIMER_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

namespace annety
{
class TimerFD : public SelectableFD
{
public:
	explicit TimerFD(bool nonblock = true, bool cloexec = true);
	explicit TimerFD(int sckfd) : SelectableFD(sckfd) {}

	virtual ~SocketFD() override;
	
private:
	int fd_;

	DISALLOW_COPY_AND_ASSIGN(TimerFD);
};

}	// namespace annety

#endif	// _ANT_TIMER_FD_H_
