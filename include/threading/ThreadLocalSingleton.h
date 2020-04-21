// By: wlmwang
// Date: Jun 09 2019

#ifndef ANT_THREADING_THREAD_LOCAL_SINGLETON_H
#define ANT_THREADING_THREAD_LOCAL_SINGLETON_H

#include "Macros.h"
#include "Logging.h"
#include "threading/ThreadLocal.h"

#include <pthread.h>

namespace annety
{
// Example:
// // ThreadLocalSingleton
// class thread_local_test
// {
// public:
//		thread_local_test() {
//			cout << pthread_self() << "|" << "thread_local_test" << endl;
//		}
//		~thread_local_test() {
//			cout << pthread_self() << "|" << "~thread_local_test" << "|" << foo_ << endl;
//		}
//
//		void setfoo(const string& f) {
//			foo_ = f;
//		}
//		string getfoo() {
//			cout << pthread_self() << "|" << foo_ << endl;
//			return foo_;
//		}
//
// private:
//		string foo_ = "nochage";
// };
//
// ThreadLocalSingleton<thread_local_test>::get()->setfoo("main");
// ThreadLocalSingleton<thread_local_test>::get()->getfoo();		// main
// Thread([]() {
//		ThreadLocalSingleton<thread_local_test>::get()->getfoo();	// nochage
// }, "thread local singleton").start().join();
//
// cout << "thread local singleton ~destruct before" << endl;
// ...

template <typename Type>
class ThreadLocalSingleton final
{
public:
	static bool empty() { return data_.empty();}

	static Type* get() { return data_.get();}

private:
	// data
	class ThreadLocalData : public ThreadLocal<Type>
	{
	public:
		ThreadLocalData() 
			: ThreadLocal<Type>(&ThreadLocalData::on_thread_exit) {}
		
		bool empty() { return tls_instance_ == nullptr;}

		Type* get()
		{
			if (!tls_instance_) {
				tls_instance_ = ThreadLocal<Type>::get();
			}
			CHECK(tls_instance_ != nullptr);
			return tls_instance_;
		}

	private:
		// 1. A thread(getspecific thread) exit will run on_thread_exit()
		// 2. A thread(main thread) exit will run on_thread_exit()
		static void on_thread_exit(void* ptr)
		{
			// FIXME: Now, On MacOS platform the tls_instance_ == 0 ???
#if defined(OS_LINUX)
			CHECK(tls_instance_ == ptr);
#endif
			// call (Type)ptr ~destruct
			ThreadLocal<Type>::local_dtor(ptr);
			tls_instance_ = nullptr;
		}

		// thread_local have thread lifetime
		static thread_local Type* tls_instance_;
	};

private:
	// process(*note:not thread*) lifetime
	static ThreadLocalData data_;

	DISALLOW_IMPLICIT_CONSTRUCTORS(ThreadLocalSingleton<Type>);
};

template <typename Type>
thread_local Type* ThreadLocalSingleton<Type>::ThreadLocalData::tls_instance_{nullptr};

template <typename Type>
typename ThreadLocalSingleton<Type>::ThreadLocalData ThreadLocalSingleton<Type>::data_{};

}	// namespace annety

#endif	// ANT_THREADING_THREAD_LOCAL_SINGLETON_H
