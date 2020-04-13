// By: wlmwang
// Date: Jul 04 2019

#ifndef ANT_TIMER_H_
#define ANT_TIMER_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "CallbackForward.h"

#include <stdint.h>

namespace annety
{
// A timer instance.
class Timer
{
public:
	Timer(TimerCallback cb, TimeStamp expired, TimeDelta interval);
	~Timer();

	TimeStamp expired() const  { return expired_;}
	int64_t sequence() const { return sequence_;}
	bool repeat() const { return !interval_.is_null();}

	void restart(TimeStamp curr);
	void run() const { cb_();}

private:
	TimeStamp expired_;
	const TimeDelta interval_;
	const TimerCallback cb_;
	const int64_t sequence_;

	DISALLOW_COPY_AND_ASSIGN(Timer);
};

}	// namespace annety

#endif	// ANT_TIMER_H_
