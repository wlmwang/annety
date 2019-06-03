

#include <iostream>
#include <string>

#include "StringPrintf.h"
#include "StringPiece.h"
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
    //     int a = 15, b = 20;
    //     AtExitManager exit_manager;
    //     exit_manager.RegisterCallback(std::bind(&func, &a));
    //     exit_manager.RegisterCallback(std::bind(&func, &b));
    // }

	// // StringPrintf
	// string sp = string_printf("ddd %3.1f", 4555.33);
	// cout << sp << endl;

	// // StringPiece
	// StringPiece s("asdfghsjkl");
	// std::cout << s << "|" << s.rfind("s") << "|" << s.substr(2) << std::endl;

	// // SafeStrerror
	// std::cout << safe_strerror(10) << std::endl;
	// std::cout << fast_safe_strerror(11) << std::endl;

	// // ByteBuffer
	// ByteBuffer<10> bb;

	// bb.append("123456");
	// bb.append("xx");
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
	// stream << -12345.345 << "|" << string_printf("ddd %3d", 4555) << true;

	// cout << stream << endl;

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
