// Refactoring: Anny Wang
// Date: Jun 09 2019

#ifndef ANT_THREAD_LOCAL_H
#define ANT_THREAD_LOCAL_H

#include "BuildConfig.h"
#include "Macros.h"
#include "Logging.h"

#include <pthread.h>

namespace annety {
template<typename Type>
class ThreadLocalSingleton;

template<typename Type>
class ThreadLocal {
public:
	typedef void (*DeleterFuncType) (void* ptr);

	ThreadLocal(const DeleterFuncType dtor = &ThreadLocal::local_dtor) {
		DCHECK(dtor);
		DPCHECK(::pthread_key_create(&tls_key_, local_dtor_ = dtor) == 0);
	}
	
	virtual ~ThreadLocal() {
		// 1. ThreadLocal<> and tls_key_ have the same lifetime
		// 2. TODO: pthread_key_delete could not call local_dtor_() when
		// pthread_key_create called by one thread
		Type* ptr = static_cast<Type*>(::pthread_getspecific(tls_key_));
		DPCHECK(::pthread_key_delete(tls_key_) == 0);
		if (ptr) {
			local_dtor_(ptr);
		}
	}
	
	Type* get() {
		Type* ptr = static_cast<Type*>(::pthread_getspecific(tls_key_));
		return ptr? ptr: set(new Type());
	}
	
	Type* set(Type* ptr) {
		DPCHECK(::pthread_getspecific(tls_key_) == nullptr);
		DPCHECK(::pthread_setspecific(tls_key_, ptr) == 0);
		return ptr;
	}

private:
	friend class ThreadLocalSingleton<Type>;

	// 1. Scoped ThreadLocal end of lifetime will run local_dtor(). [auto]
	// 2. A thread(getspecific ThreadLocal) exit will run local_dtor() [auto]
	// 3. A thread(created ThreadLocal) exit will run local_dtor()
	static void local_dtor(void *ptr) {
		DCHECK(ptr);

		typedef char T_must_be_complete_type[sizeof(Type) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void) dummy;
		
		delete static_cast<Type*>(ptr);
	}

private:
	pthread_key_t tls_key_;
	DeleterFuncType local_dtor_;

	DISALLOW_COPY_AND_ASSIGN(ThreadLocal<Type>);
};

}	// namespace annety

#endif	// ANT_THREAD_LOCAL_H
