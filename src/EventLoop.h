// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_LOOP_H_
#define ANT_EVENT_LOOP_H_

#include "Macros.h"
#include "ThreadForward.h"

#include <memory>
#include <utility>

namespace annety
{
class Channel;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();
	
	void loop();


	// internal interface---------------------------------

	void update_channel(Channel* ch);
	void remove_channel(Channel* ch);
	bool has_channel(Channel *ch);

	void check_in_own_thread() const;
	bool is_in_own_thread() const;

private:
	bool looping_;
	ThreadRef owning_thread_;

	DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_H_
