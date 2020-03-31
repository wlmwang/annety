// By: wlmwang
// Date: Jul 04 2019

#include "Timer.h"
#include "Logging.h"

#include <atomic>
#include <utility>

namespace annety
{
namespace {
std::atomic<int64_t> globalSequence{0};
}	// namespace anonymous

Timer::Timer(TimerCallback cb, TimeStamp expired, TimeDelta interval)
	: expired_(expired)
	, interval_(interval)
	, cb_(std::move(cb))
	, sequence_(globalSequence++)
{
	DLOG(TRACE) << "Timer::Timer [" << "sequence:" << sequence_ 
		<< ", interval:" << interval_ 
		<<  ", expired:" << expired_ <<"] is constructing";
}

Timer::~Timer()
{
	DLOG(TRACE) << "Timer::~Timer [" << "sequence:" << sequence_ 
		<< ", interval:" << interval_ 
		<<  ", expired:" << expired_ <<"] is destructing";
}

void Timer::restart(TimeStamp tm)
{
	if (repeat()) {
		expired_ = tm + interval_;
	} else {
		// repeat() should be called by user
		// expired_ = TimeStamp();
		NOTREACHED();
	}
}

}	// namespace annety
