// Modify: Anny Wang
// Date: May 29 2019

#include "BuildConfig.h"
#include "CompilerSpecific.h"
#include "MutexLock.h"
#include "ConditionVariable.h"
#include "Thread.h"

#include <functional>
#include <string>
#include <vector>
#include <deque>
#include <memory>

namespace annety {
// You just call AddWork() to add a delegate to the list of work to be done.
// JoinAll() will make sure that all outstanding work is processed, and wait
// for everything to finish.  You can reuse a pool, so you can call Start()
// again after you've called JoinAll().
class ThreadPool {
public:
  typedef std::function<void()> Tasker;

  explicit ThreadPool(const std::string& name_prefix, int num_threads = 0);
  ~ThreadPool();

  // Start up all of the underlying threads, and start processing work if we
  // have any.
  void start();

  void stop();

  // Make sure all outstanding work is finished, and wait for and destroy all
  // of the underlying threads in the pool.
  // void join_all();

  // It is safe to AddWork() any time, before or after Start().
  // Delegate* should always be a valid pointer, NULL is reserved internally.
  void run_tasker(Tasker tasker);

  size_t get_tasker_size() const;

  // Must be called before start().
  void set_max_tasker_size(size_t max_tasker_size) {
    max_tasker_size_ = max_tasker_size;
  }
  void set_thread_init_cb(const Tasker& cb) {
    thread_init_cb_ = cb;
  }

private:
  Tasker pop();
  bool is_full() const;
  void thread_loop();

private:
  const std::string name_prefix_;
  int num_threads_{0};
  size_t max_tasker_size_{0};
  bool running_{false};
  Tasker thread_init_cb_{};

  mutable MutexLock lock_;
  ConditionVariable empty_ev_;
  ConditionVariable full_ev_;
  
  std::deque<Tasker> taskers_;
  std::vector<std::unique_ptr<Thread>> threads_;
};

} // namespace annety

