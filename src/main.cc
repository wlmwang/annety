

#include <iostream>
#include <string>

#include "StringPrintf.h"
#include "StringPiece.h"
#include "StringUtil.h"
#include "StringSplit.h"
#include "SafeSprintf.h"
#include "SafeStrerror.h"
#include "ByteBuffer.h"
#include "LogStream.h"
#include "StlUtil.h"
#include "Time.h"
#include "Logging.h"
#include "MutexLock.h"
#include "PlatformThread.h"
#include "Thread.h"
#include "Exception.h"
#include "ThreadPool.h"
#include "BlockingQueue.h"
#include "FilePath.h"
#include "FileEnumerator.h"
#include "File.h"
#include "AtExit.h"

using namespace annety;
using namespace std;

void test22() {
	throw Exception("this is a exception!!"); 
}
void test11() {
	test22();
}
void test() {
	test11();
}

void func(void* ptr) {
	std::cout << "**" << ptr << std::endl;
}

int main(int argc, char* argv[]) {
#if defined(OS_MACOSX)
	cout << "mac osx" << endl;
#elif defined(OS_LINUX)
	cout << "linux os" << endl;
#endif

#if defined(NDEBUG)
	std::cout << "NDEBUG SET:" << NDEBUG << std::endl;
#endif
	
	cout << __cplusplus << endl;
    
    // // AtExitManager
    // {
    //     int a = 15;
    //     AtExitManager exit_manager;
    //     exit_manager.RegisterCallback(std::bind(&func, &a));
    //     exit_manager.RegisterCallback(std::bind(&func, nullptr));
    // }

	// // StringPiece
	// StringPiece s("abcdcba");
	// StringPiece s1 = s;
	// std::cout << s << ":" << s1 << '\n' 
	// 		<< s.rfind("s") << "|" << s.substr(2) << std::endl;

	// // StringUtil
	// StringPiece ts = trim_whitespace(" 12345  ", TrimPositions::TRIM_ALL);
	// cout << ts << "|" << ts.size() << endl;

	// string ss = "abcd";
	// cout << write_into(&ss, 2) << endl;

	// cout << equals_case_insensitive("ABcD", "AbcD") << endl;

	// // StringSplit
	// std::string input = "12,345;67,890";
	// std::vector<std::string> tokens = split_string(input, ",;", 
	// 						KEEP_WHITESPACE,
	// 						SPLIT_WANT_ALL);
	// for (auto &t : tokens) {
	// 	cout << t << endl;
	// }

	// // SafeSprintf
	// char buf[1024];
	// cout << strings::safe_sprintf(buf, "float %3d", 4555) << endl;
	// cout << buf << endl;

	// // StringPrintf
	// string sf = string_printf("float %3.1f", 4555.33);
	// cout << sf << endl;

	// // SafeStrerror
	// std::cout << safe_strerror(10) << std::endl;
	// std::cout << fast_safe_strerror(11) << std::endl;

	// // ByteBuffer
	// ByteBuffer<10> bb;

	// bb.append("123456");
	// bb.append("ab");
	// bb.append("cd");

	// ByteBuffer<10> bb1 = bb;

	// cout << bb << endl;
	// cout << bb.taken_as_string() << endl;	// taken
	// cout << bb << "::" << bb1 << endl;

	// bb.append("78");
	// bb.append("ef");

	// cout << "append success:" << bb.append("hijklm") << endl;
	// cout << "append success:" << bb.append("n") << endl;
	// cout << bb << endl;

	// // LogBuffer
	// LogBuffer lb;
	// lb.append("这个是中文，本质上是utf-8");
	
	// LogBuffer lb1 = lb;
	// lb.reset();
	
	// cout << lb << "::" << lb1 << endl;

	// // LogStream
	// LogStream stream;
	// stream << -12345.345 << "|" << string_printf("double %3d", 555) << true;

	// LogStream stream1 = stream;
	// stream.reset();

	// cout << stream << "::" << stream1 << endl;

	// // Time
	// Time t = Time::now();
	// cout << t << endl;

	// t += TimeDelta::from_hours(8);
	// cout << t << endl;

	// cout << t.utc_midnight() << endl;

	// // StlUtil
	// const char* arr[] = {"asdfasd", "xxx", "vvv"};
	// cout << sizeof arr << "|" << sizeof(arr)/sizeof(void*) << endl;	// 3*8=24
	// cout << annety::size(arr) << endl;

	// // Logging
	// LOG(INFO) << 1234.5123 << "xxx";

	// CHECK_EQ(1, 1);

	// DCHECK_EQ(1, 1);
	// DCHECK_LE(1, 2);
	// // // NOTREACHED();	// debug abort/release ERROR

	// // PlatformThreadHandle
	// PlatformThreadHandle handle;
	// PlatformThread::create([]() {
	// 	for (int i = 0; i < 1000; i++) {
	// 		PLOG(INFO) << 1234.5123 << "xxx";
			
	// 		// sleep(3);
	// 	}
	// }, &handle);
	// PlatformThread::join(handle);

	// // Thread
	// Thread tt([]() {
	// 	// for (int i = 0; i < 1000000; i++) {
	// 	// 	PLOG(INFO) << 1234.5123 << "xxx";
	// 	// }
	// 	test();
	// }, "annety");
	// tt.start();
	// tt.join();

	// // ThreadPool
	// ThreadPool tp("annetys", 5);
	// tp.start();

	// for (int i = 0; i < 10; i++) {
	// 	tp.run_tasker([](){
	// 		// std::cout << "ttt:" << pthread_self() << std::endl;
	// 		LOG(INFO) << "ttt:" << pthread_self();
	// 	});
	// }

	// tp.run_tasker([](){
	// 	LOG(INFO) << "ttt:" << pthread_self();
	// }, 10);
	// tp.join_all();

	// tp.start();
	// tp.run_tasker([](){
	// 	LOG(INFO) << "yyy:" << pthread_self();
	// }, 10);
	// tp.join_all();
	// // // tp.stop();


	// // BlockingQueue
	// // BlockingQueue<int> bq;
	// BlockingQueue<int, BoundedBlockingTrait<int>> bq(2);
	// Thread producer([&bq]() {
	// 	for (int i = 0; i < 10; i++) {
	// 		bq.push(i);
	// 		LOG(INFO) << "producer:" << i << "|" << pthread_self();
	// 	}
	// }, "annety-producer");
	// producer.start();

	// sleep(1);	// test for BoundedBlockingTrait
	// // one thread consumer
	// Thread consumer([&bq]() {
	// 	while(true) {
	// 		LOG(INFO) << "consumer:" << bq.pop() << "|" << pthread_self();
	// 	}
	// }, "annety-consumer");
	// consumer.start();

	// producer.join();
	// consumer.join();

	// thread pool consumer
	// ThreadPool consumers(16, "annety-consumers");
	// consumers.start();
	// consumers.run_tasker([&bq](){
	// 	while(true) {
	// 		LOG(INFO) << "consumer:" << bq.pop() << "|" << pthread_self();
	// 	}
	// });

	// producer.join();
	// consumers.join_all();

	// // FilePath
	// FilePath fp("/usr/local/annety.log.gz");

	// cout << "dirname:" << fp.dir_name() << endl;
	// cout << "basename:" << fp.base_name() << endl;
	// cout << "extension:" << fp.extension() << endl;
	// cout << "final extension:" << fp.final_extension() << endl;

	// cout << "components:" << endl;
	// std::vector<std::string> components;
	// fp.get_components(&components);
	// for (auto cc : components) {
	// 	cout << cc << endl;
	// }

	// // FileEnumerator
	// FileEnumerator enums(FilePath("."), false, FileEnumerator::FILES, "*.cc");
	// for (FilePath name = enums.next(); !name.empty(); name = enums.next()) {
	// 	cout << name << endl;
	// }

	// // File
	// File f("ggg.log", File::FLAG_OPEN_ALWAYS | File::FLAG_APPEND | File::FLAG_READ);
	// cout << "write:"<< f.write(0, "ggg", strlen("ggg")) << endl;

	// char buf[1024];
	// std::cout << "read:"<< f.read(0, buf, sizeof(buf)) << std::endl;
	// std::cout << buf << std::endl;

	// // Exception
	// try {
	// 	test();
	// } catch (const Exception& ex) {
	// 	fprintf(stderr, "reason: %s\n", ex.what());
	// 	fprintf(stderr, "stack trace: %s\n", ex.backtrace());
	// }
}
