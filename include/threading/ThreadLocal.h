// By: wlmwang
// Date: Jun 09 2019

#ifndef ANT_THREADING_THREAD_LOCAL_H
#define ANT_THREADING_THREAD_LOCAL_H

#include "Macros.h"
#include "Logging.h"

#include <pthread.h>

namespace annety
{
// Example:
// // ThreadLocal
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
// ThreadLocal<thread_local_test> tls;
// tls.get()->setfoo("main");
// tls.get()->getfoo();				// main
// Thread([&tls]() {
//		tls.get()->getfoo();		// nochage
//		{
//			ThreadLocal<thread_local_test> tls;
//			tls.get()->setfoo("pthread1");
//			tls.get()->getfoo();	// pthread1
//		}
//		tls.get()->getfoo();		// nochage
//
// }, "thread local").start().join();
//
// cout << "thread local ~destruct before" << endl;
//
// tls.get()->getfoo();				// main
// ...

template <typename Type>
class ThreadLocalSingleton;

template <typename Type>
class ThreadLocal
{
public:
	using DeleteFuncType = void(*)(void*);

	ThreadLocal(const DeleteFuncType func = &ThreadLocal::local_dtor)
	{
		CHECK(func);
		PCHECK(::pthread_key_create(&tls_key_, local_dtor_func_ = func) == 0);
	}
	
	// no virtual even ThreadLocalSingleton extends this
	~ThreadLocal()
	{
		// 1. ThreadLocal<> and tls_key_ have the same lifetime
		// 2. TODO: pthread_key_delete could not call local_dtor_func_() when
		// pthread_key_create called by one thread
		Type* ptr = static_cast<Type*>(::pthread_getspecific(tls_key_));
		PCHECK(::pthread_key_delete(tls_key_) == 0);
		if (ptr) {
			local_dtor_func_(ptr);
		}
	}
	
	bool empty()
	{
		return ::pthread_getspecific(tls_key_) == nullptr;
	}

	Type* get()
	{
		Type* ptr = static_cast<Type*>(::pthread_getspecific(tls_key_));
		return ptr? ptr: set(new Type());
	}
	
	Type* set(Type* ptr)
	{
		PCHECK(::pthread_getspecific(tls_key_) == nullptr);
		PCHECK(::pthread_setspecific(tls_key_, ptr) == 0);
		return ptr;
	}

private:
	friend class ThreadLocalSingleton<Type>;

	// 1. Scoped ThreadLocal end of lifetime will run local_dtor(). [auto]
	// 2. A thread(getspecific ThreadLocal) exit will run local_dtor() [auto]
	// 3. A thread(created ThreadLocal) exit will run local_dtor()
	static void local_dtor(void *ptr)
	{
		CHECK(ptr);

		typedef char T_must_be_complete_type[sizeof(Type) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void) dummy;
		
		delete static_cast<Type*>(ptr);
	}

private:
	pthread_key_t tls_key_;
	DeleteFuncType local_dtor_func_;

	DISALLOW_COPY_AND_ASSIGN(ThreadLocal<Type>);
};

}	// namespace annety

#endif	// ANT_THREADING_THREAD_LOCAL_H
