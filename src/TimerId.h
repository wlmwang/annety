// Refactoring: Anny Wang
// Date: Jul 05 2019

#ifndef ANT_TIMER_ID_H_
#define ANT_TIMER_ID_H_

namespace annety
{
class Timer;

// TimerId is standard layout class, but do not trivial class and pod.
struct TimerId
{
public:
	TimerId(Timer *timer, int64_t seq) 
		: timer_(timer), sequence_(seq) {}

public:
	Timer* timer_;
	int64_t sequence_;
};

}	// namespace annety

#endif	// ANT_TIMER_ID_H_
