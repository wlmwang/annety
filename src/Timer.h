// Refactoring: Anny Wang
// Date: Jul 04 2019

#ifndef ANT_TIMER_H_
#define ANT_TIMER_H_

#include "Macros.h"
#include "Time.h"
#include "CallbackForward.h"

#include <stdint.h>

namespace annety
{
class Timer
{
public:
	Timer(TimerCallback cb, Time when, double interval);

	Time expired() const  { return expired_;}
	int64_t sequence() const { return sequence_;}
	bool repeat() const { return repeat_;}

	void restart(Time tm);
	void run() const { cb_();}

private:
	Time expired_;
	const TimerCallback cb_;
	const int64_t sequence_;
	const double interval_;	// seconds
	const bool repeat_;

	DISALLOW_COPY_AND_ASSIGN(Timer);
};

}	// namespace annety

#endif	// ANT_TIMER_H_
