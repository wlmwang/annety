

#include <iostream>
#include <string>

#include "StringPrintf.h"
// #include "StringPiece.h"
// #include "SafeStrerror.h"
// #include "ByteBuffer.h"
// #include "LogStream.h"
// #include "StlUtil.h"
// #include "Time.h"
// #include "Logging.h"
#include "MutexLock.h"

using namespace annety;
using namespace std;

int main(int argc, char* argv[]) {
#if defined(OS_MACOSX)
	cout << "mac osx" << endl;
#elif defined(OS_LINUX)
	cout << "linux os" << endl;
#endif

// #if defined(NDEBUG)
// 	std::cout << "NDEBUG SET:" << NDEBUG << std::endl;
// #endif
	
	cout << __cplusplus << endl;

	// // StringPrintf
	// string sp = StringPrintf("ddd %3.1f", 4555.33);
	// cout << sp << endl;

	// // StringPiece
	// StringPiece s("asdfghsjkl");
	// std::cout << s << "|" << s.rfind("s") << "|" << s.substr(2) << std::endl;

	// // SafeStrerror
	// std::cout << safe_strerror(10) << std::endl;
	// std::cout << safe_fast_strerror(11) << std::endl;

	// // ByteBuffer
	// ByteBuffer<10> bb;

	// bb.append("asdf");
	// bb.append("xxxx");
	// bb.append("yy");

	// cout << bb << endl;
	// cout << bb.taken_as_string() << endl;	// taken

	// bb.append("tt");
	// bb.append("asdf");

	// cout << "append success:" << bb.append("asdff") << bb.append("T") << endl;
	// cout << bb << endl;

	// LogBuffer lb;
	// lb.append("as这个是汉子？222");
	// cout << lb << endl;

	// // LogStream
	// LogStream stream;
	// stream << -12345.345 << "|" << StringPrintf("ddd %3d", 4555) << true;

	// cout << stream << endl;

	// // for time
	// Time t = Time::now();
	// cout << t << endl;

	// t += TimeDelta::from_hours(8);
	// cout << t << endl;

	// cout << t.utc_midnight() << endl;

	// // StlUtil
	// const char* arr[] = {"asdfasd", "xxx", "vvv"};
	// cout << sizeof arr << "|" << sizeof(arr)/sizeof(void*) << endl;	// 3*8=24
	// cout << annety::size(arr) << endl;

	// // for logging
	// LOG(INFO, 10) << 1234.5123 << "xxx";

	// CHECK_EQ(1, 1);

	// DCHECK_EQ(1, 1);
	// DCHECK_LE(1, 2);
	// // NOTREACHED();	// debug abort/release ERROR
}