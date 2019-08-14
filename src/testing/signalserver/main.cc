
#include "SignalServer.h"
#include "EventLoop.h"
#include "Logging.h"
#include "threading/Thread.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	// SignalServer
	
	// // error------------------------
	// Thread([]() {
	// 	EventLoop loop;
	// 	SignalServer ssrv(&loop);
	// 	ssrv.add_signal(SIGUSR1, [&ssrv]() {
	// 		LOG(INFO) << "ssrv:" << SIGUSR1;
	// 	});
	// 	loop.loop();
	// }).start().join();
	
	// ok----------------------------
	EventLoop loop;
	SignalServer ssrv(&loop);
	ssrv.add_signal(SIGTERM, [&]() {
		LOG(INFO) << "ssrv:" << SIGTERM;
		ssrv.delete_signal(SIGQUIT);
		// ssrv.revert_signal();
	});
	
	// ssrv.add_signal(SIGTERM, [&ssrv]() {
	// 	LOG(INFO) << "ssrv1:" << SIGTERM;
	// });

	// add
	ssrv.add_signal(SIGQUIT, [&]() {
		LOG(INFO) << "ssrv:" << SIGQUIT;
	});

	// ignore
	ssrv.ignore_signal(SIGQUIT);
	
	loop.loop();
}
