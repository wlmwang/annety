

#include <iostream>
#include <string>

#include "StringPrintf.h"
#include "StringPiece.h"
#include "ByteBuffer.h"
#include "LogStream.h"
#include "Logging.h"

#include "Time.h"

using namespace annety;
using namespace std;

int main(int argc, char* argv[]) {
	// StringPiece s("asdfghsjkl");
	// std::cout << s << "|" << s.rfind("s") << "|" << s.substr(2) << std::endl;

	// cout << OS_MACOSX << "|" << __cplusplus << endl;


	// Time t = Time::now();

	// cout << t << endl;

	// t += 1 * Time::kMicrosecondsPerHour;
	
	// cout << t << endl;

	// cout << t.utc_midnight() << endl;

	// //
	// ByteBuffer<10> bb;

	// bb.append("asdf");
	// bb.append("xxxx");
	// bb.append("yy");
	// cout << bb << endl;

	// cout << bb.taken_as_string() << endl;

	// bb.append("tt");
	// bb.append("asdf");
	// cout << bb.append("asdff") << endl;
	// cout << bb << endl;

	// UnboundedBuffer ub;
	// ub.append("asdfzxcvasdfqrtasdf");
	// cout << ub << endl;


	// LogBuffer lb;
	// lb.append("asdfggg");

	// LogStream stream;
	// stream << -12345.345 << "|" << StringPrintf("ddd %3d", 4555) << true;

	// cout << stream << endl;

	// SFixedBuffer buff;
	// buff.append("asdfasdf", strlen("asdfasdf"));
	// buff.append("13234", strlen("13234"));

	// LOG(ERROR) << "asdf" << "ggh" << "ghh" << 1234 << "xxx";
#if defined(NDEBUG)
	std::cout << "NDEBUG SET" << NDEBUG << std::endl;
#endif

	LOG(INFO, 10) << 1234.5123 << "xxx";

	// CHECK_EQ(1, 2);
	// DCHECK_EQ(3, 4);

	// cout << buff.data() << endl;
}