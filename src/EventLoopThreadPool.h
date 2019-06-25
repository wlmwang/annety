// Refactoring: Anny Wang
// Date: Jun 25 2019

#ifndef ANT_EVENT_LOOP_THREAD_POOL_H_
#define ANT_EVENT_LOOP_THREAD_POOL_H_

#include "Macros.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace annety
{
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	EventLoopThreadPool(EventLoop* loop, const std::string& name);
	~EventLoopThreadPool();
  
	void set_thread_num(int num_threads) { num_threads_ = num_threads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	// valid after calling start()
	/// round-robin
	EventLoop* get_next_loop();

	/// with the same hash code, it will always return the same EventLoop
	EventLoop* get_loop_with_hashcode(size_t hashcode);

	std::vector<EventLoop*> get_all_loops();

	bool started() const { return started_; }

	const std::string& name() const { return name_; }

private:
	EventLoop* owner_loop_;
	std::string name_;
	bool started_{false};
	int next_{0};
	int num_threads_{0};

	std::vector<std::unique_ptr<EventLoopThread>> threads_;
	std::vector<EventLoop*> loops_;
	
	DISALLOW_COPY_AND_ASSIGN(EventLoopThreadPool);
};

}	// namespace annety

#endif	// ANT_EVENT_LOOP_THREAD_POOL_H_
