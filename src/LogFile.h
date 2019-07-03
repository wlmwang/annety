// Refactoring: Anny Wang
// Date: Jul 02 2019

#ifndef ANT_LOG_FILE_H_
#define ANT_LOG_FILE_H_

#include "Macros.h"
#include "MutexLock.h"
#include "StringPiece.h"
#include "Time.h"

#include <memory>
#include <string>
#include <inttypes.h>

namespace annety
{
class File;

class LogFile
{
public:
	LogFile(const std::string& basename,
			off_t rotate_size_b = 10*1024*1024,
			int flush_interval_s = 3,
			int check_every_n = 1024,
			bool thread_safe = true);

	~LogFile();

	void append(const StringPiece& message);
	void append(const char* message, int len);

	void flush();
	bool rotate();

private:
	void append_unlocked(const char* message, int len);

private:
	const std::string basename_;
	const off_t rotate_size_b_;
	const int check_every_n_;
	const TimeDelta flush_interval_s_;

	int count_{0};
	off_t written_{0};

	// time_t startOfPeriod_;// 滚动日志当日 0 点时间戳
	// time_t lastRoll_;     // 上一次滚动日志文件时间戳
	// time_t lastFlush_;    // 上一次刷新磁盘时间戳
	// std::unique_ptr<FileUtil::AppendFile> file_;  // 日志文件句柄

	Time last_rotate_;
	Time last_flush_;
	Time start_period_;

	std::unique_ptr<MutexLock> lock_;

	std::unique_ptr<File> file_;
};

}	// namespace annety

#endif	// ANT_LOG_FILE_H_
