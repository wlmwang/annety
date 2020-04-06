// By: wlmwang
// Date: Jul 02 2019

#ifndef ANT_LOG_FILE_H_
#define ANT_LOG_FILE_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "files/FilePath.h"
#include "strings/StringPiece.h"
#include "synchronization/MutexLock.h"

#include <atomic>
#include <memory>
#include <string>
#include <stdint.h>		// uint32_t,int64_t
#include <sys/types.h>	// size_t,off_t

namespace annety
{
// Example:
// // LogFile
// LogFile logf("logging", 1024);
// logf.append("12345", strlen("12345"));
// logf.append("text", strlen("text"));
// logf.flush();
// ...


class File;

class LogFile
{
public:
	LogFile(const FilePath& path /*basename*/,
			off_t rotate_size_b = 100*1024*1024,
			int check_every_n = 1024);

	~LogFile();

	void append(const StringPiece& message);
	void append(const char* message, int len);

	void flush();
	void rotate(bool force = false);

private:
	void append_unlocked(const char* message, int len);

private:
	const FilePath path_;
	const off_t rotate_size_b_;
	const int check_every_n_;
	const TimeDelta flush_interval_s_;

	std::atomic<int> count_{0};
	std::atomic<off_t> written_{0};

	MutexLock lock_;
	std::unique_ptr<File> file_;
	TimeStamp last_rotate_;
};

}	// namespace annety

#endif	// ANT_LOG_FILE_H_
