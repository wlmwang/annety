

#include <iostream>
#include <string>

#include "StringPrintf.h"
#include "StringPiece.h"
#include "StringUtil.h"
#include "StringSplit.h"
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
#include "FileUtil.h"
#include "AtExit.h"

using namespace annety;
using namespace std;

void test22() {
	throw Exception("<<<throw a exception>>>"); 
}
void test11() {
	test22();
}
void test() {
	test11();
}

void func(void* ptr) {
	std::cout << "AtExitManager:" << "*" << ptr << std::endl;
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
    
 //    // AtExitManager
 //    {
 //        int a = 15;
 //        AtExitManager exit_manager;
 //        exit_manager.RegisterCallback(std::bind(&func, &a));
 //        exit_manager.RegisterCallback(std::bind(&func, nullptr));
 //    }

	// // StringPiece
	// StringPiece s("abcdcba");
	// StringPiece s1 = s;
	// std::cout << s << ":" << s1 << '\n' 
	// 		<< s.rfind("s") << "|" << s.find("c") << "|" << s.substr(2) << std::endl;

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

	// // StringPrintf
	// string sf = string_printf("printf(%%3.1) %3.1f", 555.33);
	// cout << sf << endl;

	// // SafeStrerror
	// std::cout << "errno(10):" << safe_strerror(10) << std::endl;
	// std::cout << "errno(11):" << fast_safe_strerror(11) << std::endl;

	// // ByteBuffer
	// ByteBuffer<10> bb;

	// bb.append("123456");
	// bb.append("ab");
	// bb.append("cd");

	// ByteBuffer<10> bb1 = bb;

	// cout << "take before:" << bb << endl;
	// cout << "take all:" << bb.taken_as_string() << endl;	// taken
	// cout << "take before:" << bb << "|" << bb1 << endl;

	// bb.append("01");
	// bb.append("23");

	// cout << "append success:" << bb.append("4567890") << endl;
	// cout << "append success:" << bb.append("a") << endl;
	// cout << "append rt:" << bb << endl;

	// // LogStream
	// LogStream stream;
	// stream << -12345.345 << "#" << string_printf("printf(%%3d) %3d", 555) << true;

	// LogStream stream1 = stream;
	// stream.reset();

	// cout << "stream:" << stream << "|" << stream1 << endl;

	// // LogBuffer
	// LogBuffer lb;
	// lb.append("这个是中文，本质上是utf-8");
	
	// LogBuffer lb1 = lb;
	// lb.reset();
	
	// cout << "LogBuffer:" << lb << "|" << lb1 << endl;

	// // Time
	// Time t = Time::now();
	// cout << "now(utc):" << t << endl;

	// t += TimeDelta::from_hours(8);
	// cout << "+8:" << t << endl;

	// cout << "midnight:" << t.utc_midnight() << endl;

	// // StlUtil @todo
	// const char* arr[] = {"asdfasd", "xxx", "vvv"};
	// cout << "sizeof:" << sizeof arr << "|" << sizeof(arr)/sizeof(void*) << endl;	// 3*8=24
	// cout << "annety::size:" << size(arr) << "|" << size("12345") << endl;

	// // error: no matching function for call to 'size'
	// // const char *str = "12345";
	// // cout << "annety::size:" << arraysize(str) << endl;
	
	// // Logging
	// LOG(INFO) << 1234.5123 << "xxx";

	// CHECK_EQ(1, 1);
	// DCHECK_EQ(1, 1);
	// DCHECK_LE(1, 2);
	// // NOTREACHED();	// debug abort/release ERROR

	// // PlatformThreadHandle
	// PlatformThreadHandle handle;
	// PlatformThread::create([]() {
	// 	for (int i = 0; i < 10; i++) {
	// 		// errno = 10;
	// 		PLOG(INFO) << 1234.5678 << " <<<message<<<";
	// 	}
	// }, &handle);
	// PlatformThread::join(handle);

	// // Thread
	// Thread tt([]() {
	// 	for (int i = 0; i < 10; i++) {
	// 		PLOG(INFO) << 1234.5678 << " <<<message<<<";
	// 	}
	// }, "annety");
	// tt.start();
	// tt.join();

	// // ThreadPool
	// ThreadPool tp(5, "annetys");
	// tp.start();

	// for (int i = 0; i < 10; i++) {
	// 	tp.run_tasker([]() {
	// 		LOG(INFO) << "ThreadPool(for):" << pthread_self();
	// 	});
	// }
	// tp.run_tasker([]() {
	// 	LOG(INFO) << "ThreadPool(batch):" << pthread_self();
	// }, 10);

	// tp.joinall();

	// tp.start();
	// tp.run_tasker([](){
	// 	LOG(INFO) << "ThreadPool(restart):" << pthread_self();
	// }, 10);
	// tp.join_all();
	// // tp.stop();


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
	// consumer.join();	// will hold

	// // ThreadPool consumer
	// ThreadPool consumers(16, "annety-consumers");
	// consumers.start();
	// consumers.run_tasker([&bq](){
	// 	while(true) {
	// 		LOG(INFO) << "consumer:" << bq.pop() << "|" << pthread_self();
	// 	}
	// });

	// producer.join();
	// consumers.joinall();	// will hold

	// // FilePath
	// FilePath fp("/usr/local/annety.log.gz");
	// cout << "full:" << fp << endl;
	// cout << "dirname:" << fp.dirname() << endl;
	// cout << "basename:" << fp.basename() << endl;
	// cout << "extension:" << fp.extension() << endl;
	// cout << "final extension:" << fp.final_extension() << endl;

	// cout << "components:" << endl;
	// std::vector<std::string> components;
	// fp.get_components(&components);
	// for (auto cc : components) {
	// 	cout << "|" << cc << "|";
	// }
	// cout << endl;

	// // FileEnumerator
	// cout << "all (*.cc) file:" << endl;
	// FileEnumerator enums(FilePath("."), false, FileEnumerator::FILES, "*.cc");
	// for (FilePath name = enums.next(); !name.empty(); name = enums.next()) {
	// 	cout << name << endl;
	// }

	// // File
	// FilePath path("annety-text-file.log");
	// File f(path, File::FLAG_OPEN_ALWAYS | File::FLAG_APPEND | File::FLAG_READ);
	// cout << "write(annety-file.log):"<< f.write(0, "test text", strlen("test text")) << endl;

	// char buf[1024];
	// std::cout << "read len:"<< f.read(0, buf, sizeof(buf)) << std::endl;
	// std::cout << "read content:" << buf << std::endl;

	// // FileUtil
	// std::string cc;
	// cout << "read status:" << read_file_to_string(path, &cc) << endl;
	// cout << "read content:" << cc << endl;

	// cout << "delete file:" << delete_file(path, false) << endl;

	// // Exception
	// try {
	// 	test();
	// } catch (const Exception& ex) {
	// 	fprintf(stderr, "reason: %s\n", ex.what());
	// 	fprintf(stderr, "stack trace: %s\n", ex.backtrace());
	// }

	// // Exception Thread
	// Thread tt([]() {
	// 	test();
	// }, "annety");
	// tt.start();
	// tt.join();
}
