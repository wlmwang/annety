// Refactoring: Anny Wang
// Date: Jul 02 2019

#ifndef ANT_LOG_FILE_H_
#define ANT_LOG_FILE_H_

#include "Macros.h"
#include "Time.h"
#include "MutexLock.h"
#include "StringPiece.h"
#include "FilePath.h"

#include <atomic>
#include <memory>
#include <string>
#include <inttypes.h>

namespace annety
{
class File;

class LogFile
{
public:
	LogFile(const FilePath& path /*base filename*/,
			off_t rotate_size_b = 100*2^20,
			int flush_interval_s = 3,
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

	Time last_rotate_;
	Time last_flush_;

	MutexLock lock_;
	std::unique_ptr<File> file_;
};

}	// namespace annety

#endif	// ANT_LOG_FILE_H_
