// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventLoop.h"
#include "ThreadLocal.h"
#include "PlatformThread.h"

namespace annety {
namespace {
// one thread one loop
void clean_tls_eventloop(void *ptr);
ThreadLocal<EventLoop> tls_eventloop{&clean_tls_eventloop};

void clean_tls_eventloop(void *ptr) {
	LOG(INFO) << "TLS EventLoop has clean by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", EventLoop " << ptr;
}

}	// namespace anonymous

EventLoop::EventLoop() :
	looping_(false),
	owning_thread_(PlatformThread::current_ref())
{
	LOG(INFO) << "EventLoop is creating by thread " 
		<< owning_thread_.ref() 
		<< ", EventLoop " << this;
	
	CHECK(tls_eventloop.empty()) << " EventLoop has created by thread " 
		<< owning_thread_.ref() 
		<< ", Current thread " << PlatformThread::current_ref().ref();

	tls_eventloop.set(this);
}

EventLoop::~EventLoop() {
	LOG(INFO) << "~EventLoop is called by thread " 
		<< PlatformThread::current_ref().ref()
		<< ", EventLoop " << this;
}

void EventLoop::loop() {
	//
}

void EventLoop::update_channel(Channel* ch) {
	//
}

void EventLoop::check_in_own_thread() const {
	CHECK(is_in_own_thread()) << " EventLoop was created by thread " 
		<< owning_thread_.ref()
		<< ", Current thread " << PlatformThread::current_ref().ref();
}

bool EventLoop::is_in_own_thread() const {
	return owning_thread_ == PlatformThread::current_ref();
}

}	// namespace annety
