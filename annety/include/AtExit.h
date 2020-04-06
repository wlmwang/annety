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
// Example:
// // AtExitManager
// void func(void) {}
// Early in the main() scope create an AtExitManager instance on the stack:
// AtExitManager g_exit_manager;
// int main(...)
// {
// 		g_exit_manager.register_callback(std::bind(&func));
//		
//		// Important: for testing only!!!
//		{
//			AtExitManager scoped_exit_manager;
//			AtExitManager::RegisterCallback(std::bind(&func));
//		}
// }
// When the *_exit_manager instance goes out of scope, all the registered
// callbacks and the registered singleton destructors will be called.

// This class provides a facility similar to the CRT atexit(), except that
// we control when the callbacks are executed.
//
// This facility is *Not 100% thread safe*, it is mostly used by Singleton.
class AtExitManager
{
public:
	using AtExitCallback = std::function<void()>;

	AtExitManager();

	// The dtor calls all the registered callbacks.
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
	//
	// Important: for testing only!!!
	explicit AtExitManager(bool shadow);

private:
	MutexLock lock_;
	std::stack<AtExitCallback> stack_cb_;
	
	// Stack of managers to allow shadowing.
	AtExitManager* next_manager_;

	DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

}	// namespace annety

#endif	// ANT_AT_EXIT_H
