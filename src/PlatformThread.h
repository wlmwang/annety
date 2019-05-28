// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 28 2019

#ifndef ANT_PLATFORM_THREAD_H_
#define ANT_PLATFORM_THREAD_H_

#include "BuildConfig.h"
#include "CompilerSpecific.h"
#include "Time.h"

#include <functional>
#include <string>
#include <stddef.h>

#if defined(OS_POSIX)
#include <pthread.h>
#include <unistd.h>
#endif

namespace annety {
// Used for logging. Always an integer value.
#if defined(OS_MACOSX)
typedef mach_port_t PlatformThreadId;
#elif defined(OS_POSIX)
typedef pid_t PlatformThreadId;
#endif

const PlatformThreadId kInvalidThreadId{0};

// Used for thread checking and debugging.
// Meant to be as fast as possible.
// These are produced by PlatformThread::CurrentRef(), and used to later
// check if we are on the same thread or not by using ==. These are safe
// to copy between threads, but can't be copied to another process as they
// have no meaning there. Also, the internal identifier can be re-used
// after a thread dies, so a PlatformThreadRef cannot be reliably used
// to distinguish a new thread from an old, dead thread.
class PlatformThreadRef {
public:
  typedef pthread_t RefType;
  
  constexpr PlatformThreadRef() : id_(0) {}

  explicit constexpr PlatformThreadRef(RefType id) : id_(id) {}

  bool operator==(PlatformThreadRef other) const {
    return id_ == other.id_;
  }

  bool operator!=(PlatformThreadRef other) const { return id_ != other.id_; }

  bool is_null() const {
    return id_ == 0;
  }
  
private:
  RefType id_;
};

// Used to operate on threads.
class PlatformThreadHandle {
public:
#if defined(OS_POSIX)
  typedef pthread_t Handle;
#endif

  constexpr PlatformThreadHandle() : handle_(0) {}

  explicit constexpr PlatformThreadHandle(Handle handle) : handle_(handle) {}

  bool is_equal(const PlatformThreadHandle& other) const {
    return handle_ == other.handle_;
  }

  bool is_null() const {
    return !handle_;
  }

  Handle platform_handle() const {
    return handle_;
  }

private:
  Handle handle_;
};

// A namespace for low-level thread functions.
class PlatformThread {
public:
  typedef std::function<void()> ThreadMainFunc;

  // Gets the current thread id, which may be useful for logging purposes.
  static PlatformThreadId current_id();

  // Gets the current thread reference, which can be used to check if
  // we're on the right thread quickly.
  static PlatformThreadRef current_ref();

  // Get the handle representing the current thread. On Windows, this is a
  // pseudo handle constant which will always represent the thread using it and
  // hence should not be shared with other threads nor be used to differentiate
  // the current thread from another.
  static PlatformThreadHandle current_handle();

  // Yield the current thread so another thread can be scheduled.
  static void yield_current_thread();

  // Sleeps for the specified duration.
  static void sleep(TimeDelta duration);
  
  // Sets the thread name visible to debuggers/tools. This will try to
  // initialize the context for current thread unless it's a WorkerThread.
  static void set_name(const std::string& name);

  // Creates a new thread.  Upon success,
  // |*thread_handle| will be assigned a handle to the newly created thread,
  // and |fuc|'s ThreadMainFunc method will be executed on the newly created
  // thread.
  // NOTE: When you are done with the thread handle, you must call Join to
  // release system resources associated with the thread.  You must ensure that
  // the Delegate object outlives the thread.
  static bool create(ThreadMainFunc func, PlatformThreadHandle* thread_handle);

  // CreateNonJoinable() does the same thing as Create() except the thread
  // cannot be Join()'d.  Therefore, it also does not output a
  // PlatformThreadHandle.
  static bool create_non_joinable(ThreadMainFunc func);

  // Joins with a thread created via the Create function.  This function blocks
  // the caller until the designated thread exits.  This will invalidate
  // |thread_handle|.
  static void join(PlatformThreadHandle thread_handle);

  // Detaches and releases the thread handle. The thread is no longer joinable
  // and |thread_handle| is invalidated after this call.
  static void detach(PlatformThreadHandle thread_handle);
};

}  // namespace annety

#endif  // ANT_PLATFORM_THREAD_H_
