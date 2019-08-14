
#include "Logging.h"
#include "LogFile.h"
#include "strings/StringPrintf.h"

#include <memory>
#include <iostream>

using namespace annety;
using namespace std;

// Logging
std::unique_ptr<LogFile> g_log_file;

void output_func(const char* msg, int len)
{
	g_log_file->append(msg, len);
}

void flush_func()
{
	g_log_file->flush();
}

int main(int argc, char* argv[])
{
	// LogStream
	LogStream stream;
	stream << -12345.345 << "#" << string_printf("printf(%%3d) %3d", 555) << true;

	LogStream stream1 = stream;
	stream.reset();

	cout << "stream:" << stream << "|" << stream1 << endl;

	// Logging
	LOG(TRACE) << 1234.5123 << " xxx";
	LOG(INFO) << "vvv";

	CHECK_EQ(1, 1);
	DCHECK_EQ(1, 1);
	DCHECK_LE(1, 2);
	// NOTREACHED();	// debug abort/release ERROR

	// // LogFile
	// LogFile ll(FilePath("logging"), 1024);
	// ll.append("test1", ::strlen("test1"));
	// ll.append("test2", ::strlen("test2"));

	// LogFile for Logging
	char name[256]{0};
	::strncpy(name, argv[0], sizeof name - 1);
	g_log_file.reset(new LogFile(FilePath("logging"), 5*1024*1024, 1024));
	set_log_output_handler(output_func);
	set_log_fflush_handler(flush_func);

	for (int i = 0; i < 50000; ++i) {
		LOG(INFO) << "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
}
