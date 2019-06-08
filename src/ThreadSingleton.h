// Refactoring: Anny Wang
// Date: Jun 03 2019

#ifndef ANT_THREAD_SINGLETON_H
#define ANT_THREAD_SINGLETON_H

#include "Macros.h"
#include "Logging.h"

#include <pthread.h>

namespace annety {
template<typename Type>
class ThreadSingleton {
 public:
	static Type* get() {
		if (!tls_instance_) {
			tls_instance_ = new Type();
			
			deleter_.set(tls_instance_);
		}
		DCHECK(tls_instance_ != nullptr);
		return tls_instance_;
	}

private:
	static void on_thread_exit(void* ptr) {
		// TODO(macos):Now pthread was exited, so tls_instance_ == 0 ???
		// DCHECK(tls_instance_ == ptr);
		DCHECK(ptr);

		typedef char T_must_be_complete_type[sizeof(Type) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void) dummy;
		
		delete reinterpret_cast<Type*>(ptr);
		tls_instance_ = nullptr;
	}

private:
	class Deleter {
	public:
		Deleter() {
			DPCHECK(::pthread_key_create(&tls_key_, 
						&ThreadSingleton::on_thread_exit) == 0);
		}
		~Deleter() {
			DPCHECK(::pthread_key_delete(tls_key_) == 0);
		}

		void set(Type* ptr) {
			DPCHECK(::pthread_getspecific(tls_key_) == nullptr);
			DPCHECK(::pthread_setspecific(tls_key_, ptr) == 0);
		}

	private:
		pthread_key_t tls_key_;
	};

private:
	static thread_local Type* tls_instance_;
	static Deleter deleter_;

	DISALLOW_COPY_AND_ASSIGN(ThreadSingleton);
};

template<typename Type>
thread_local Type* ThreadSingleton<Type>::tls_instance_ = nullptr;

template<typename Type>
typename ThreadSingleton<Type>::Deleter ThreadSingleton<Type>::deleter_;

}	// namespace annety

#endif	// ANT_THREAD_SINGLETON_H
