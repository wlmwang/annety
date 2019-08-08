
#include "threading/ThreadLocal.h"
#include "threading/ThreadLocalSingleton.h"
#include "threading/Thread.h"
#include "threading/ThreadPool.h"
#include "PlatformThread.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	// ThreadRef
	ThreadRef ref;
	PlatformThread::create([]() {
		for (int i = 0; i < 10; i++) {
			PLOG(INFO) << 1234.5678 << " <<<message<<<";
		}
	}, &ref);
	PlatformThread::join(ref);

	// Thread
	Thread([]() {
		for (int i = 0; i < 10; i++) {
			PLOG(INFO) << 1234.5678 << " <<<message<<<" << i;
		}
	}, "annety").start().join();

	// ThreadPool
	ThreadPool tp(5, "annetys");
	tp.start();

	for (int i = 0; i < 10; i++) {
		tp.run_task([]() {
			LOG(INFO) << "ThreadPool(for):" << pthread_self();
		});
	}

	tp.run_task([]() {
		LOG(INFO) << "ThreadPool(batch):" << pthread_self();
	}, 10);
	tp.joinall();

	tp.start();
	tp.run_task([](){
		LOG(INFO) << "ThreadPool(restart):" << pthread_self();
	}, 10);
	tp.joinall();
	
	// tp.stop();
}
