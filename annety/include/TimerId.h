// By: wlmwang
// Date: Jul 05 2019

#ifndef ANT_TIMER_ID_H_
#define ANT_TIMER_ID_H_

namespace annety
{
class Timer;

// TimerId is standard layout class, but it is neither a trivial class nor a pod.
// It is value sematics, which means that it can be copied or assigned.
class TimerId
{
public:
	TimerId() {}

	TimerId(Timer *timer, int64_t seq) 
		: timer_(timer)
		, sequence_(seq) {}

	bool is_valid() { return !!timer_; }

public:
	Timer* timer_ {nullptr};
	int64_t sequence_ {0};
};

}	// namespace annety

#endif	// ANT_TIMER_ID_H_
