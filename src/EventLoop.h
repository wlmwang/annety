// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "ThreadForward.h"

namespace annety {

class EventLoop {
public:
	EventLoop();
	~EventLoop();
	
	void loop();

	void check_in_owning_thread() const;
	bool is_in_owning_thread() const;

private:
	bool looping_;
	ThreadRef owning_thread_ref_;

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
