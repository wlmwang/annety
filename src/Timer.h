// Refactoring: Anny Wang
// Date: Jul 04 2019

#ifndef ANT_TIMER_H_
#define ANT_TIMER_H_

#include "Macros.h"
#include "Times.h"
#include "CallbackForward.h"

#include <stdint.h>

namespace annety
{
class Timer
{
public:
	Timer(TimerCallback cb, Time expired, TimeDelta interval);
	~Timer();

	Time expired() const  { return expired_;}
	int64_t sequence() const { return sequence_;}
	bool repeat() const { return !interval_.is_null();}

	void restart(Time tm);
	void run() const { cb_();}

private:
	Time expired_;
	const TimeDelta interval_;
	const TimerCallback cb_;
	const int64_t sequence_;

	DISALLOW_COPY_AND_ASSIGN(Timer);
};

}	// namespace annety

#endif	// ANT_TIMER_H_
