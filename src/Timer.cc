// Refactoring: Anny Wang
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

Timer::Timer(TimerCallback cb, Time when, double interval)
	: expired_(when),
	  cb_(std::move(cb)),
	  sequence_(globalSequence++),
	  interval_(interval),
	  repeat_(interval > 0.0)
{
	LOG(TRACE) << "Timer::Timer [" << "sequence:" << sequence_ 
		<< ", interval:" << interval_ 
		<<  ", expired:" << expired_ <<"] is constructing";
}

Timer::~Timer()
{	
	LOG(TRACE) << "Timer::~Timer [" << "sequence:" << sequence_ 
		<< ", interval:" << interval_ 
		<<  ", expired:" << expired_ <<"] is destructing";
}

void Timer::restart(Time tm)
{
	if (repeat_) {
		expired_ = tm + TimeDelta::from_seconds_d(interval_);
	} else {
		expired_ = Time();
	}
}

}	// namespace annety
