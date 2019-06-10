
// Refactoring: Anny Wang
// Date: Jun 03 2019

#ifndef ANT_SINGLETON_H
#define ANT_SINGLETON_H

#include "Macros.h"
#include "AtExit.h"
#include "Logging.h"

#include <functional>
#include <pthread.h>

namespace annety {
// Default traits for Singleton<Type>. Calls operator new and operator delete on
// the object. Registers automatic deletion at process exit.
// Overload if you need arguments or another memory allocation function.
template<typename Type>
struct DefaultSingletonTraits {
	// Set to true to automatically register deletion of the object on process
	// exit. See below for the required call that makes this happen.
	static const bool kRegisterAtExit = true;

	// new the object.
	static Type* create() {
		// The parenthesis is very important here; it forces POD type
		// initialization.
		return new Type();
	}

	// delete the object.
	static void destory(Type* x) {
		delete x;
	}
};

// Alternate traits for use with the Singleton<Type>.  Identical to
// DefaultSingletonTraits except that the Singleton will not be cleaned up
// at exit.
template<typename Type>
struct LeakySingletonTraits : public DefaultSingletonTraits<Type> {
	static const bool kRegisterAtExit = false;
};

// Example:
//
// In your header:
//   namespace annety {
//   template <typename T>
//   struct DefaultSingletonTraits;
//   }
//   class FooClass {
//    public:
//     static FooClass* instance();  <-- See comment below on this.
//     void Bar() { ... }
//    private:
//     FooClass() { ... }
//     friend struct annety::DefaultSingletonTraits<FooClass>;
//
//     DISALLOW_COPY_AND_ASSIGN(FooClass);
//   };
//
// In your source file:
//  #include "Singleton.h"
//  FooClass* FooClass::GetInstance() {
//    return annety::Singleton<FooClass>::get();
//  }
//
// Or for leaky singletons:
//  #include "Singleton.h"
//  FooClass* FooClass::instance() {
//    return annety::Singleton<
//        FooClass, annety::LeakySingletonTraits<FooClass>>::get();
//  }
//
// And to call methods on FooClass:
//   FooClass::instance()->Bar();
//
// NOTE: The method accessing Singleton<T>::get() has to be named as instance
// and it is important that FooClass::instance() is not inlined in the
// header. This makes sure that when source files from multiple targets include
// this header they don't end up with different copies of the inlined code
// creating multiple copies of the singleton.
//
// Singleton<> has no non-static members and doesn't need to actually be
// instantiated.
//
// This class is itself thread-safe. The underlying Type must of course be
// thread-safe if you want to use it concurrently. Two parameters may be tuned
// depending on the user's requirements.
//
// Glossary:
//   RAE = kRegisterAtExit
//
// On every platform, if Traits::RAE is true, the singleton will be destroyed at
// process exit. More precisely it uses AtExitManager which requires an
// object of this type to be instantiated. AtExitManager mimics the semantics
// of atexit() such as LIFO order but under Windows is safer to call. For more
// information see at_exit.h.
//
// If Traits::RAE is false, the singleton will not be freed at process exit,
// thus the singleton will be leaked if it is ever accessed. Traits::RAE
// shouldn't be false unless absolutely necessary. Remember that the heap where
// the object is allocated may be destroyed by the CRT anyway.
//
// Caveats:
// (a) Every call to get(), operator->() and operator*() incurs some overhead
//     (16ns on my P4/2.8GHz) to check whether the object has already been
//     initialized.  You may wish to cache the result of get(); it will not
//     change.
//
// (b) Your factory function must never throw an exception. This class is not
//     exception-safe.

template<typename Type, typename Traits = DefaultSingletonTraits<Type>>
class Singleton {
 public:
	// Classes using the Singleton<T> pattern should declare a GetInstance()
	// method and call Singleton::get() from within that.
	friend Type* Type::instance();

	static Type* get() {
		DPCHECK(::pthread_once(&once_, &Singleton::create) == 0);
		DCHECK(instance_ != nullptr);
		return instance_;
	}

private:
	static void create() {
		DCHECK(instance_ == nullptr);

		instance_ = Traits::create();
		if (Traits::kRegisterAtExit) {
			AtExitManager::RegisterCallback(std::bind(&Singleton::on_exit, instance_));
		}
	}

	// Adapter function for use with AtExit().  This should be called single
	// threaded, so don't use atomic operations.
	// Calling OnExit while singleton is in use by other threads is a mistake.
	static void on_exit(void* ptr) {
		DCHECK(instance_ == ptr);

		typedef char T_must_be_complete_type[sizeof(Type) == 0 ? -1 : 1];
		T_must_be_complete_type dummy; (void) dummy;

		// AtExit should only ever be register after the singleton instance was
		// created.  We should only ever get here with a valid instance_ pointer.
		Traits::destory(instance_);
		instance_ = nullptr;
	}

private:
	static Type* instance_;
	static pthread_once_t once_;

	DISALLOW_IMPLICIT_CONSTRUCTORS(Singleton);
};

template<typename Type, typename Traits>
pthread_once_t Singleton<Type, Traits>::once_ = PTHREAD_ONCE_INIT;

template<typename Type, typename Traits>
Type* Singleton<Type, Traits>::instance_ = nullptr;

}	// namespace annety

#endif	// ANT_SINGLETON_H
