// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 03 2019

#include "AtExit.h"
#include "Logging.h"

#include <utility>

namespace annety
{
// Keep a stack of registered AtExitManagers.  We always operate on the most
// recent, and we should never have more than one outside of testing (for a
// statically linked version of this library).  Testing may use the shadow
// version of the constructor, and if we are building a dynamic library we may
// end up with multiple AtExitManagers on the same process.  We don't protect
// this for thread-safe access, since it will only be modified in testing.
static AtExitManager* g_top_manager = nullptr;

AtExitManager::AtExitManager() : next_manager_(g_top_manager)
{
	// If multiple modules instantiate AtExitManagers they'll end up living in this
	// module... they have to coexist.
	DCHECK(!g_top_manager);
	g_top_manager = this;
}

AtExitManager::~AtExitManager()
{
	if (!g_top_manager) {
		NOTREACHED() << "Tried to ~AtExitManager without an AtExitManager";
		return;
	}
	DCHECK_EQ(this, g_top_manager);

	process_callbacks();
	g_top_manager = next_manager_;
}

// static
void AtExitManager::register_callback(AtExitCallback cb)
{
	DCHECK(cb);
	if (!g_top_manager) {
		NOTREACHED() << "Tried to register_callback without an AtExitManager";
		return;
	}

	AutoLock locked(g_top_manager->lock_);
	g_top_manager->stack_cb_.push(std::move(cb));
}

// static
void AtExitManager::process_callbacks()
{
	if (!g_top_manager) {
		NOTREACHED() << "Tried to process_callbacks without an AtExitManager";
		return;
	}

	std::stack<AtExitCallback> cbs;
	{
		AutoLock locked(g_top_manager->lock_);
		cbs.swap(g_top_manager->stack_cb_);
	}

	while (!cbs.empty()) {
		auto task = cbs.top();
		cbs.pop();
		task();
	}
}

AtExitManager::AtExitManager(bool shadow) : next_manager_(g_top_manager)
{
	DCHECK(shadow || !g_top_manager);
	g_top_manager = this;
}

}	// namespace annety
