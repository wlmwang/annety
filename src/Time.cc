// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#include "Time.h"
#include "StringPrintf.h"
#include "Logging.h"

#include <algorithm>
#include <cmath>	// isnan
#include <ostream>

namespace annety
{
namespace
{
Time TimeNowIgnoreTZ()
{
	struct timeval tv;
	struct timezone tz = {0, 0};  // UTC
	CHECK(::gettimeofday(&tv, &tz) == 0);
	// microseconds since the epoch.
	return Time() + TimeDelta::from_microseconds(
					tv.tv_sec * Time::kMicrosecondsPerSecond + 
					tv.tv_usec);
}

}	// namespace anonymous

// Time --------------------------------------------------------------------

// static
Time Time::now()
{
	return TimeNowIgnoreTZ();
}

// static
Time Time::from_time_t(time_t tt)
{
	if (tt == 0) {
		return Time();
	} else if (tt == std::numeric_limits<time_t>::max()) {
		return max();
	}
	return Time(tt * kMicrosecondsPerSecond);
}

time_t Time::to_time_t() const
{
	if (is_null()) {
		return 0;
	} else if (is_max()) {
		return std::numeric_limits<time_t>::max();
	}
	return us_ / kMicrosecondsPerSecond;
}

// static
Time Time::from_timeval(struct timeval t)
{
	DCHECK_LT(t.tv_usec, static_cast<int>(Time::kMicrosecondsPerSecond));
	DCHECK_GE(t.tv_usec, 0);
	if (t.tv_usec == 0 && t.tv_sec == 0) {
		return Time();
	} else if (t.tv_usec == static_cast<suseconds_t>(kMicrosecondsPerSecond) - 1 
							&& t.tv_sec == std::numeric_limits<time_t>::max()) {
		return max();
	}
	return Time((static_cast<int64_t>(t.tv_sec) * kMicrosecondsPerSecond) + t.tv_usec);
}

struct timeval Time::to_timeval() const
{
	struct timeval result;
	if (is_null()) {
		result.tv_sec = 0;
		result.tv_usec = 0;
		return result;
	} else if (is_max()) {
		result.tv_sec = std::numeric_limits<time_t>::max();
		result.tv_usec = static_cast<suseconds_t>(kMicrosecondsPerSecond) - 1;
		return result;
	}
	result.tv_sec = us_ / Time::kMicrosecondsPerSecond;
	result.tv_usec = us_ % Time::kMicrosecondsPerSecond;
	return result;
}

// static
Time Time::from_double_ts(double dt)
{
	if (dt == 0 || std::isnan(dt)) {
		return Time();
	}
	return Time(static_cast<int64_t>(dt * kMicrosecondsPerSecond));
}

double Time::to_double_ts() const
{
	if (is_null()) {
		return 0;
	} else if (is_max()) {
		return std::numeric_limits<double>::infinity();
	}
	return static_cast<double>(us_) / kMicrosecondsPerSecond;
}

Time Time::midnight(bool is_local) const
{
	Exploded exploded;
	to_explode(is_local, &exploded);
	exploded.hour = 0;
	exploded.minute = 0;
	exploded.second = 0;
	exploded.millisecond = 0;
	Time out_time;
	if (from_exploded(is_local, exploded, &out_time)) {
		return out_time;
	}

	// This function must not fail.
	NOTREACHED();
	return Time();
}

// static
bool Time::exploded_mostly_equals(const Exploded& lhs, const Exploded& rhs)
{
	return lhs.year == rhs.year && lhs.month == rhs.month &&
		   lhs.day_of_month == rhs.day_of_month && lhs.hour == rhs.hour &&
		   lhs.minute == rhs.minute && lhs.second == rhs.second &&
		   lhs.millisecond == rhs.millisecond;
}

std::ostream& operator<<(std::ostream& os, TimeDelta delta)
{
	return os << delta.in_seconds_f() << " s";
}

std::ostream& operator<<(std::ostream& os, Time time)
{
	Time::Exploded exploded;
	time.to_utc_explode(&exploded);
	return os << string_printf("%04d-%02d-%02d %02d:%02d:%02d.%06d UTC",
								exploded.year,
								exploded.month,
								exploded.day_of_month,
								exploded.hour,
								exploded.minute,
								exploded.second,
								exploded.millisecond);
}

// Time::Exploded -------------------------------------------------------------
namespace
{
// This prevents a crash on traversing the environment global and looking up
// the 'TZ' variable in libc. See: crbug.com/390567.
time_t time_t_from_tm(struct tm* timestruct, bool is_local)
{
	return is_local ? ::mktime(timestruct) : ::timegm(timestruct);
}
struct tm* time_t_to_tm(time_t t, struct tm* timestruct, bool is_local)
{
	if (is_local) {
		::localtime_r(&t, timestruct);
	} else {
		::gmtime_r(&t, timestruct);
	}
	return timestruct;
}

}	// namespace anonymous


Time::Exploded* Time::to_explode(bool is_local, Exploded* exploded) const
{
	// The following values are all rounded towards -infinity.
	int64_t milliseconds;  // Milliseconds since epoch.
	time_t seconds;       // Seconds since epoch.
	int millisecond;       // Exploded millisecond value (0-999).
	if (us_ >= 0) {
		// Rounding towards -infinity <=> rounding towards 0, in this case.
		milliseconds = us_ / kMicrosecondsPerMillisecond;
		seconds = milliseconds / kMillisecondsPerSecond;
		millisecond = milliseconds % kMillisecondsPerSecond;
	} else {
		// Round these *down* (towards -infinity).
		milliseconds = (us_ - kMicrosecondsPerMillisecond + 1) /
						kMicrosecondsPerMillisecond;
		seconds = (milliseconds - kMillisecondsPerSecond + 1) / 
						kMillisecondsPerSecond;
		// Make this nonnegative (and between 0 and 999 inclusive).
		millisecond = milliseconds % kMillisecondsPerSecond;
		if (millisecond < 0) {
			millisecond += kMillisecondsPerSecond;
		}
	}

	struct tm timestruct;
	time_t_to_tm(seconds, &timestruct, is_local);

	exploded->year = timestruct.tm_year + 1900;
	exploded->month = timestruct.tm_mon + 1;
	exploded->day_of_week = timestruct.tm_wday;
	exploded->day_of_month = timestruct.tm_mday;
	exploded->hour = timestruct.tm_hour;
	exploded->minute = timestruct.tm_min;
	exploded->second = timestruct.tm_sec;
	exploded->millisecond = millisecond;
	return exploded;
}

// static
bool Time::from_exploded(bool is_local, const Exploded& exploded, Time* time)
{
	int month = exploded.month - 1;
	int year = exploded.year - 1900;

	struct tm timestruct;
	timestruct.tm_sec = exploded.second;
	timestruct.tm_min = exploded.minute;
	timestruct.tm_hour = exploded.hour;
	timestruct.tm_mday = exploded.day_of_month;

	timestruct.tm_mon = month;
	timestruct.tm_year = year;

	timestruct.tm_wday = exploded.day_of_week;  // mktime/timegm ignore this
	timestruct.tm_yday = 0;                     // mktime/timegm ignore this
	timestruct.tm_isdst = -1;                   // attempt to figure it out

	time_t seconds;

	// Certain exploded dates do not really exist due to daylight saving times,
	// and this causes mktime() to return implementation-defined values when
	// tm_isdst is set to -1. On Android, the function will return -1, while the
	// C libraries of other platforms typically return a liberally-chosen value.
	// Handling this requires the special code below.

	// time_t_from_tm() modifies the input structure, save current value.
	struct tm timestruct0 = timestruct;

  seconds = time_t_from_tm(&timestruct, is_local);
	if (seconds == -1) {
		// Get the time values with tm_isdst == 0 and 1, then select the closest one
		// to UTC 00:00:00 that isn't -1.
		timestruct = timestruct0;
		timestruct.tm_isdst = 0;
		int64_t seconds_isdst0 = time_t_from_tm(&timestruct, is_local);

		timestruct = timestruct0;
		timestruct.tm_isdst = 1;
		int64_t seconds_isdst1 = time_t_from_tm(&timestruct, is_local);

		// seconds_isdst0 or seconds_isdst1 can be -1 for some timezones.
		// E.g. "CLST" (Chile Summer Time) returns -1 for 'tm_isdt == 1'.
		if (seconds_isdst0 < 0) {
			seconds = seconds_isdst1;
		} else if (seconds_isdst1 < 0) {
			seconds = seconds_isdst0;
		} else {
			seconds = std::min(seconds_isdst0, seconds_isdst1);
		}
	}

	// Handle overflow.  Clamping the range to what mktime and timegm might
	// return is the best that can be done here.  It's not ideal, but it's better
	// than failing here or ignoring the overflow case and treating each time
	// overflow as one second prior to the epoch.
	int64_t milliseconds = 0;
	if (seconds == -1 && (exploded.year < 1969 || exploded.year > 1970)) {
		// If exploded.year is 1969 or 1970, take -1 as correct, with the
		// time indicating 1 second prior to the epoch.  (1970 is allowed to handle
		// time zone and DST offsets.)  Otherwise, return the most future or past
		// time representable.  Assumes the time_t epoch is 1970-01-01 00:00:00 UTC.
		//
		// The minimum and maximum representible times that mktime and timegm could
		// return are used here instead of values outside that range to allow for
		// proper round-tripping between exploded and counter-type time
		// representations in the presence of possible truncation to time_t by
		// division and use with other functions that accept time_t.
		//
		// When representing the most distant time in the future, add in an extra
		// 999ms to avoid the time being less than any other possible value that
		// this function can return.

		// On Android, SysTime is int64_t, special care must be taken to avoid
		// overflows.
		const int64_t min_seconds = (sizeof(time_t) < sizeof(int64_t))
									? std::numeric_limits<time_t>::min()
									: std::numeric_limits<int32_t>::min();
		const int64_t max_seconds = (sizeof(time_t) < sizeof(int64_t))
								? std::numeric_limits<time_t>::max()
								: std::numeric_limits<int32_t>::max();
		if (exploded.year < 1969) {
			milliseconds = min_seconds * kMillisecondsPerSecond;
		} else {
			milliseconds = max_seconds * kMillisecondsPerSecond;
			milliseconds += (kMillisecondsPerSecond - 1);
		}
	} else {
		int64_t checked_millis = seconds;
		checked_millis *= kMillisecondsPerSecond;
		checked_millis += exploded.millisecond;
		milliseconds = checked_millis;
	}

	int64_t checked_microseconds = milliseconds;
	checked_microseconds *= kMicrosecondsPerMillisecond;

	Time converted_time(checked_microseconds);

	// If |exploded.day_of_month| is set to 31 on a 28-30 day month, it will
	// return the first day of the next month. Thus round-trip the time and
	// compare the initial |exploded| with |utc_to_exploded| time.
	Time::Exploded to_exploded;
	if (!is_local) {
		converted_time.to_utc_explode(&to_exploded);
	} else {
		converted_time.to_local_explode(&to_exploded);
	}
    
	if (exploded_mostly_equals(to_exploded, exploded)) {
		*time = converted_time;
		return true;
	}

	*time = Time(0);
	return false;
}

inline bool is_in_range(int value, int lo, int hi)
{
	return lo <= value && value <= hi;
}

bool Time::Exploded::is_valid() const
{
	return is_in_range(month, 1, 12) &&
		   is_in_range(day_of_week, 0, 6) &&
		   is_in_range(day_of_month, 1, 31) &&
		   is_in_range(hour, 0, 23) &&
		   is_in_range(minute, 0, 59) &&
		   is_in_range(second, 0, 60) &&
		   is_in_range(millisecond, 0, 999);
}

}	// namespace annety
