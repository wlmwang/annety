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
	using SelectableFD::SelectableFD;

	TimerFD() = delete;
	TimerFD(bool nonblock, bool cloexec);

	virtual int close() override;
	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

	void reset(const TimeDelta& delta_ms);

private:
	DISALLOW_COPY_AND_ASSIGN(TimerFD);
};

}	// namespace annety

#endif	// _ANT_TIMER_FD_H_
