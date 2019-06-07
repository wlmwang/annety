// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#ifndef ANT_TIME_H_
#define ANT_TIME_H_

#include <iosfwd>
#include <limits>
#include <algorithm>

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace annety {
// TimeDelta ------------------------------------------------------------------

class TimeDelta {
public:
	constexpr TimeDelta() : delta_(0) {}

	// Converts units of time to TimeDeltas.
	static constexpr TimeDelta from_days(int days);
	static constexpr TimeDelta from_hours(int hours);
	static constexpr TimeDelta from_minutes(int minutes);
	static constexpr TimeDelta from_seconds(int64_t secs);
	static constexpr TimeDelta from_milliseconds(int64_t ms);
	static constexpr TimeDelta from_microseconds(int64_t us);
	static constexpr TimeDelta from_seconds_d(double secs);
	static constexpr TimeDelta from_milliseconds_d(double ms);
	static constexpr TimeDelta from_microseconds_d(double us);

	static constexpr TimeDelta max();
	static constexpr TimeDelta min();

	constexpr int64_t internal_value() const {
		return delta_;
	}

	constexpr bool is_null() const {
		return delta_ == 0;
	}
	constexpr bool is_max() const {
		return delta_ == std::numeric_limits<int64_t>::max();
	}
	constexpr bool is_min() const {
		return delta_ == std::numeric_limits<int64_t>::min();
	}

	// Returns the time delta in some unit.
	constexpr int in_days() const;
	constexpr int in_hours() const;
	constexpr int in_minutes() const;
	constexpr int64_t in_seconds() const;
	constexpr int64_t in_milliseconds() const;
	constexpr int64_t in_microseconds() const;
	constexpr double in_seconds_f() const;
	constexpr double in_milliseconds_f() const;
	constexpr double in_microseconds_f() const;

	// Computations with other deltas.
	TimeDelta operator+(TimeDelta other) const {
		return TimeDelta(delta_ + other.delta_);
	}
	TimeDelta operator-(TimeDelta other) const {
		return TimeDelta(delta_ - other.delta_);
	}
	TimeDelta& operator+=(TimeDelta other) {
		return *this = (*this + other);
	}
	TimeDelta& operator-=(TimeDelta other) {
		return *this = (*this - other);
	}

	constexpr TimeDelta operator-() const {
		return TimeDelta(-delta_);
	}

	// Computations with numeric types.
	template <typename T>
	TimeDelta operator*(T a) const {
		return TimeDelta(delta_ * a);
	}
	template <typename T>
	TimeDelta operator/(T a) const {
		return TimeDelta(delta_ / a);
	}
	template <typename T>
	TimeDelta& operator*=(T a) {
		return *this = (*this * a);
	}
	template <typename T>
	TimeDelta& operator/=(T a) {
		return *this = (*this / a);
	}

	constexpr TimeDelta operator/(TimeDelta a) const {
		return TimeDelta(delta_ / a.delta_);
	}
	constexpr TimeDelta operator%(TimeDelta a) const {
		return TimeDelta(delta_ % a.delta_);
	}

	// Comparison operators.
	constexpr bool operator==(TimeDelta other) const {
		return delta_ == other.delta_;
	}
	constexpr bool operator!=(TimeDelta other) const {
		return delta_ != other.delta_;
	}
	constexpr bool operator<(TimeDelta other) const {
		return delta_ < other.delta_;
	}
	constexpr bool operator<=(TimeDelta other) const {
		return delta_ <= other.delta_;
	}
	constexpr bool operator>(TimeDelta other) const {
		return delta_ > other.delta_;
	}
	constexpr bool operator>=(TimeDelta other) const {
		return delta_ >= other.delta_;
	}

private:
	// Constructs a delta given the duration in microseconds. This is private
	// to avoid confusion by callers with an integer constructor. Use
	// FromSeconds, FromMilliseconds, etc. instead.
	constexpr explicit TimeDelta(int64_t delta_us) : delta_(delta_us) {}

	// Private method to build a delta from a double.
	static constexpr TimeDelta from_double(double value);

	// Private method to build a delta from the product of a user-provided value
	// and a known-positive value.
	static constexpr TimeDelta from_product(int64_t value, int64_t positive_value);

	// Returns |delta_| (microseconds) divided by |divisor|, or the max value
	// representable in T if is_max().
	template <typename T>
	constexpr T divide_or_max(int64_t divisor) const;

	// Delta in microseconds.
	int64_t delta_;
};

template <typename T>
TimeDelta operator*(T a, TimeDelta td) {
	return td * a;
}

// for logging use only.
std::ostream& operator<<(std::ostream& os, TimeDelta delta);

namespace internal {

// TimeBase --------------------------------------------------------------------

// a microsecond timebase
template<class TimeClass>
class TimeBase {
public:
	static constexpr int64_t kHoursPerDay = 24;
	static constexpr int64_t kMillisecondsPerSecond = 1000;
	static constexpr int64_t kMillisecondsPerDay =
								kMillisecondsPerSecond * 60 * 60 * kHoursPerDay;
	static constexpr int64_t kMicrosecondsPerMillisecond = 1000;
	static constexpr int64_t kMicrosecondsPerSecond =
								kMicrosecondsPerMillisecond * kMillisecondsPerSecond;
	static constexpr int64_t kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
	static constexpr int64_t kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
	static constexpr int64_t kMicrosecondsPerDay = kMicrosecondsPerHour * kHoursPerDay;
	static constexpr int64_t kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
	static constexpr int64_t kNanosecondsPerMicrosecond = 1000;
	static constexpr int64_t kNanosecondsPerSecond =
								kNanosecondsPerMicrosecond * kMicrosecondsPerSecond;

	// for serializing only.
	constexpr int64_t internal_value() const {
		return us_;
	}

	constexpr bool is_null() const {
		return us_ == 0;
	}
	constexpr bool is_max() const {
		return us_ == std::numeric_limits<int64_t>::max();
	}
	constexpr bool is_min() const {
		return us_ == std::numeric_limits<int64_t>::min();
	}

	static TimeClass max() {
		return TimeClass(std::numeric_limits<int64_t>::max());
	}
	static TimeClass min() {
		return TimeClass(std::numeric_limits<int64_t>::min());
	}

	TimeClass& operator=(TimeClass other) {
		us_ = other.us_;
		return *(static_cast<TimeClass*>(this));
	}

	// Compute the difference between two times.
	TimeDelta operator-(TimeClass other) const {
		return TimeDelta::from_microseconds(us_ - other.us_);
	}

	// Return a new time modified by some delta.
	TimeClass operator+(TimeDelta delta) const {
		return TimeClass(us_ + delta.internal_value());
	}
	TimeClass operator-(TimeDelta delta) const {
		return TimeClass(us_ - delta.internal_value());
	}

	// Modify by some time delta.
	TimeClass& operator+=(TimeDelta delta) {
		return static_cast<TimeClass&>(*this = (*this + delta));
	}
	TimeClass& operator-=(TimeDelta delta) {
		return static_cast<TimeClass&>(*this = (*this - delta));
	}

	// Comparison operators
	bool operator==(TimeClass other) const {
		return us_ == other.us_;
	}
	bool operator!=(TimeClass other) const {
		return us_ != other.us_;
	}
	bool operator<(TimeClass other) const {
		return us_ < other.us_;
	}
	bool operator<=(TimeClass other) const {
		return us_ <= other.us_;
	}
	bool operator>(TimeClass other) const {
		return us_ > other.us_;
	}
	bool operator>=(TimeClass other) const {
		return us_ >= other.us_;
	}

protected:
	constexpr explicit TimeBase(int64_t us) : us_(us) {}

	// Time value in a microsecond timebase.
	int64_t us_;
};

}	// namespace internal

// operator delta + Time
template<class TimeClass>
inline TimeClass operator+(TimeDelta delta, TimeClass t) {
	return t + delta;
}

// Time -----------------------------------------------------------------------

// Represents a wall clock time in UTC. Values are not guaranteed to be
// monotonically non-decreasing and are subject to large amounts of skew.
class Time : public internal::TimeBase<Time> {
public:
// kExplodedMinYear and kExplodedMaxYear define the platform-specific limits
// for values passed to FromUTCExploded() and FromLocalExploded().
#if defined(OS_MACOSX)
	static constexpr int kExplodedMinYear = 1902;
	static constexpr int kExplodedMaxYear = std::numeric_limits<int>::max();
#else
	static constexpr int kExplodedMinYear =
		(sizeof(time_t) == 4 ? 1902 : std::numeric_limits<int>::min());
	static constexpr int kExplodedMaxYear =
		(sizeof(time_t) == 4 ? 2037 : std::numeric_limits<int>::max());
#endif

	// Represents an exploded time that can be formatted nicely. This is kind of
	// like the Win32 SYSTEMTIME structure or the Unix "struct tm" with a few
	// additions and changes to prevent errors.
	// POD struct
	struct Exploded {
		int year;			// Four digit year "2007"
		int month;			// 1-based month (values 1 = January, etc.)
		int day_of_week;	// 0-based day of week (0 = Sunday, etc.)
		int day_of_month;	// 1-based day of month (1-31)
		int hour;			// Hour within the current day (0-23)
		int minute;			// Minute within the current hour (0-59)
		int second;			// Second within the current minute (0-59 plus leap
							//   seconds which may take it up to 60).
		int millisecond;	// Milliseconds within the current second (0-999)

		// A cursory test for whether the data members are within their
		// respective ranges. A 'true' return value does not guarantee the
		// Exploded value can be successfully converted to a Time value.
		bool is_valid() const;
	};

	// Contains the NULL time. Use Time::Now() to get the current time.
	constexpr Time() : TimeBase(0) {}

	// Returns the current time. Watch out, the system might adjust its clock
	// in which case time will actually go backwards. We don't guarantee that
	// times are increasing, or that two calls to now() won't be the same.
	static Time now();

	// Converts to/from time_t in UTC and a Time class.
	static Time from_time_t(time_t tt);
	time_t to_time_t() const;

	static Time from_timeval(struct timeval t);
	struct timeval to_timeval() const;

	// Converts time to/from a double which is the number of seconds since epoch
	// (Jan 1, 1970). used this format to represent time.
	static Time from_double_ts(double dt);
	double to_double_ts() const;

	// Converts an exploded structure representing either the local time or UTC
	// into a Time class. Returns false on a failure when, for example, a day of
	// month is set to 31 on a 28-30 day month. Returns Time(0) on overflow.
	static bool from_utc_exploded(const Exploded& exploded, Time* time) {
		return from_exploded(false, exploded, time);
	}
	static bool from_local_exploded(const Exploded& exploded, Time* time) {
		return from_exploded(true, exploded, time);
	}

	// Fills the given exploded structure with either the local time or UTC from
	// this time structure (containing UTC).
	Exploded* to_utc_explode(Exploded* exploded) const {
		return to_explode(false, exploded);
	}
	Exploded* to_local_explode(Exploded* exploded) const {
		return to_explode(true, exploded);
	}

	// The following two functions round down the time to the nearest day in
	// either UTC or local time. It will represent midnight on that day.
	Time utc_midnight() const {
		return midnight(false);
	}
	Time local_midnight() const {
		return midnight(true);
	}

private:
	friend class internal::TimeBase<Time>;

	constexpr explicit Time(int64_t us) : TimeBase(us) {}

	// Explodes the given time to either local time |is_local = true| or UTC
	// |is_local = false|.
	Exploded* to_explode(bool is_local, Exploded* exploded) const;

	// Unexplodes a given time assuming the source is either local time
	// |is_local = true| or UTC |is_local = false|. Function returns false on
	// failure and sets |time| to Time(0). Otherwise returns true and sets |time|
	// to non-exploded time.
	static bool from_exploded(bool is_local,
							  const Exploded& exploded,
							  Time* time);

	// Rounds down the time to the nearest day in either local time
	// |is_local = true| or UTC |is_local = false|.
	Time midnight(bool is_local) const;

	// Comparison does not consider |day_of_week| when doing the operation.
	static bool exploded_mostly_equals(const Exploded& lhs,
									   const Exploded& rhs);
};

// Time ostream -----------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, Time time);

// TimeDelta ------------------------------------------------------------------

// static
constexpr TimeDelta TimeDelta::from_days(int days) {
	return days == std::numeric_limits<int>::max()
				? max()
				: TimeDelta(days * Time::kMicrosecondsPerDay);
}

// static
constexpr TimeDelta TimeDelta::from_hours(int hours) {
	return hours == std::numeric_limits<int>::max() 
					? max()
					: TimeDelta(hours * Time::kMicrosecondsPerHour);
}

// static
constexpr TimeDelta TimeDelta::from_minutes(int minutes) {
	return minutes == std::numeric_limits<int>::max()
					? max()
					: TimeDelta(minutes * Time::kMicrosecondsPerMinute);
}

// static
constexpr TimeDelta TimeDelta::from_seconds(int64_t secs) {
	return from_product(secs, Time::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::from_milliseconds(int64_t ms) {
	return from_product(ms, Time::kMicrosecondsPerMillisecond);
}

// static
constexpr TimeDelta TimeDelta::from_microseconds(int64_t us) {
	return TimeDelta(us);
}

// static
constexpr TimeDelta TimeDelta::from_seconds_d(double secs) {
	return from_double(secs * Time::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::from_milliseconds_d(double ms) {
	return from_double(ms * Time::kMicrosecondsPerMillisecond);
}

// static
constexpr TimeDelta TimeDelta::from_microseconds_d(double us) {
	return from_double(us);
}

// static
constexpr TimeDelta TimeDelta::max() {
	return TimeDelta(std::numeric_limits<int64_t>::max());
}

// static
constexpr TimeDelta TimeDelta::min() {
	return TimeDelta(std::numeric_limits<int64_t>::min());
}

// Must be defined before use below.
template <typename T>
constexpr T TimeDelta::divide_or_max(int64_t divisor) const {
	return is_max() ? std::numeric_limits<T>::max()
					: static_cast<T>(delta_ / divisor);
}

// Must be defined before use below.
template <>
constexpr double TimeDelta::divide_or_max<double>(int64_t divisor) const {
	return is_max() ? std::numeric_limits<double>::infinity()
					: static_cast<double>(delta_) / divisor;
}

constexpr int TimeDelta::in_days() const {
	return divide_or_max<int>(Time::kMicrosecondsPerDay);
}

constexpr int TimeDelta::in_hours() const {
	return divide_or_max<int>(Time::kMicrosecondsPerHour);
}

constexpr int TimeDelta::in_minutes() const {
	return divide_or_max<int>(Time::kMicrosecondsPerMinute);
}

constexpr double TimeDelta::in_seconds_f() const {
	return divide_or_max<double>(Time::kMicrosecondsPerSecond);
}

constexpr int64_t TimeDelta::in_seconds() const {
	return divide_or_max<int64_t>(Time::kMicrosecondsPerSecond);
}

constexpr double TimeDelta::in_milliseconds_f() const {
	return divide_or_max<double>(Time::kMicrosecondsPerMillisecond);
}

constexpr int64_t TimeDelta::in_milliseconds() const {
	return divide_or_max<int64_t>(Time::kMicrosecondsPerMillisecond);
}

constexpr int64_t TimeDelta::in_microseconds() const {
	return divide_or_max<int64_t>(1);
}

constexpr double TimeDelta::in_microseconds_f() const {
	return divide_or_max<double>(1);
}

// static
constexpr TimeDelta TimeDelta::from_double(double value) {
	return value > std::numeric_limits<int64_t>::max()
				? max()
				: value < std::numeric_limits<int64_t>::min()
						? min()
						: TimeDelta(static_cast<int64_t>(value));
}

// static
constexpr TimeDelta TimeDelta::from_product(int64_t value, int64_t positive_value) {
  // DCHECK(positive_value > 0);
	return value > std::numeric_limits<int64_t>::max() / positive_value
				? max()
				: value < std::numeric_limits<int64_t>::min() / positive_value
						? min()
						: TimeDelta(value * positive_value);
}

}	// namespace annety

#endif	// ANT_TIME_H_
