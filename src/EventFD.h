// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_FD_H_
#define ANT_EVENT_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

namespace annety
{
class EventFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	EventFD() = delete;
	EventFD(bool nonblock, bool cloexec);

private:
	DISALLOW_COPY_AND_ASSIGN(EventFD);
};

}	// namespace annety

#endif	// _ANT_EVENT_FD_H_
