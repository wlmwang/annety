// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 03 2019

#ifndef ANT_AT_EXIT_H
#define ANT_AT_EXIT_H

#include "Macros.h"
#include "synchronization/MutexLock.h"

#include <stack>
#include <functional>

namespace annety
{
// This class provides a facility similar to the CRT atexit(), except that
// we control when the callbacks are executed.
// This facility is mostly used by Singleton. It is *Not 100% thread safe*
//
// The usage is simple. Early in the main() scope create an AtExitManager 
// instance on the stack:
// int main(...)
// {
//	  {
//    	  AtExitManager exit_manager;
//    	  exit_manager.register_callback(std::bind(&func));
//	  }
// }
// When the exit_manager instance goes out of scope, all the registered
// callbacks and singleton destructors will be called.
class AtExitManager
{
public:
	using AtExitCallback = std::function<void()>;

	AtExitManager();

	// The dtor calls all the registered callbacks. Do not try to register more
	// callbacks after this point.
	~AtExitManager();

	// Registers the specified function to be called at exit.
	static void register_callback(AtExitCallback cb);

	// Calls the functions registered with register_callback in LIFO order. It
	// is possible to register new callbacks after calling this function.
	static void process_callbacks();

protected:
	// This constructor will allow this instance of AtExitManager to be created
	// even if one already exists.
	// AtExitManagers are kept on a global stack, and it will be removed during
	// destruction.  This allows you to shadow another AtExitManager.
	explicit AtExitManager(bool shadow);

private:
	MutexLock lock_;
	std::stack<AtExitCallback> stack_cb_;
	
	AtExitManager* next_manager_;  // Stack of managers to allow shadowing.

	DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

}	// namespace annety

#endif	// ANT_AT_EXIT_H
