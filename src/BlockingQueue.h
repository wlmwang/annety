// Modify: Anny Wang
// Date: Jun 02 2019

#ifndef ANT_BLOCKING_QUEUE_H
#define ANT_BLOCKING_QUEUE_H

#include "BuildConfig.h"
#include "CompilerSpecific.h"
#include "MutexLock.h"
#include "ConditionVariable.h"
#include "Logging.h"

#include <deque>

namespace annety {
template<typename T>
class UnBoundedBlockingTrait {
public:
  UnBoundedBlockingTrait() {}
  UnBoundedBlockingTrait(size_t max_size) = delete;

void push(const T& x) {
    AutoLock l(lock_);
    queue_.push_back(x);
    empty_cv_.signal();
  }

  void push(T&& x) {
    AutoLock l(lock_);
    queue_.push_back(std::move(x));
    empty_cv_.signal();
  }

  T pop() {
    AutoLock l(lock_);
    while (empty_with_locked()) {
      empty_cv_.wait();
    }
    DCHECK(!empty_with_locked());

    T front(std::move(queue_.front()));
    queue_.pop_front();
    return std::move(front);
  }

  size_t size() const {
    AutoLock l(lock_);
    return size_with_locked();
  }

  bool empty() const {
    AutoLock l(lock_);
    return empty_with_locked();
  }
  
  bool full() const {
    AutoLock l(lock_);
    return full_with_locked();
  }

  size_t capacity() const {
    AutoLock l(lock_);
    return capacity_with_locked();
  }

private:
  size_t size_with_locked() const {
    lock_.assert_acquired();
    return queue_.size();
  }  
  bool empty_with_locked() const {
    lock_.assert_acquired();
    return queue_.empty();
  }
  bool full_with_locked() const {
    lock_.assert_acquired();
    return queue_.size() >= queue_.max_size();
  }
  size_t capacity_with_locked() const {
    lock_.assert_acquired();
    return queue_.max_size() - queue_.size();
  }

private:
  mutable MutexLock lock_{};
  ConditionVariable empty_cv_{lock_};
  std::deque<T> queue_{};
};

template<typename T>
class BoundedBlockingTrait {
public:
  BoundedBlockingTrait() = delete;
  explicit BoundedBlockingTrait(size_t max_size)
    : max_size_(max_size) {}

  void push(const T& x) {
    AutoLock l(lock_);
    while (full_with_locked()) {
      full_cv_.wait();
    }
    DCHECK(!full_with_locked());

    queue_.push_back(x);
    empty_cv_.signal();
  }

  void push(T&& x) {
    AutoLock l(lock_);
    while (full_with_locked()) {
      full_cv_.wait();
    }
    DCHECK(!full_with_locked());

    queue_.push_back(std::move(x));
    empty_cv_.signal();
  }

  T pop() {
    AutoLock l(lock_);
    while (empty_with_locked()) {
      empty_cv_.wait();
    }
    DCHECK(!empty_with_locked());

    T front(std::move(queue_.front()));
    queue_.pop_front();
    full_cv_.signal();
    return std::move(front);
  }

  size_t size() const {
    AutoLock l(lock_);
    return size_with_locked();
  }

  bool empty() const {
    AutoLock l(lock_);
    return empty_with_locked();
  }

  bool full() const {
    AutoLock l(lock_);
    return full_with_locked();
  }

  size_t capacity() const {
    AutoLock l(lock_);
    return capacity_with_locked();
  }

private:
  size_t size_with_locked() const {
    lock_.assert_acquired();
    return queue_.size();
  }  
  bool empty_with_locked() const {
    lock_.assert_acquired();
    return queue_.empty();
  }
  bool full_with_locked() const {
    lock_.assert_acquired();
    return queue_.size() >= max_size_;
  }
  size_t capacity_with_locked() const {
    lock_.assert_acquired();
    return max_size_ - queue_.size();
  }

private:
  mutable MutexLock lock_{};
  ConditionVariable empty_cv_{lock_};
  ConditionVariable full_cv_{lock_};
  std::deque<T> queue_{};
  size_t max_size_{0};
};

// {,Un}Boundedblocking queue
template<typename T, typename Traits = UnBoundedBlockingTrait<T>>
class BlockingQueue {
public:
  struct Data : public Traits {
    Data() : Traits() {}
    explicit Data(size_t max_size) : Traits(max_size) {}
  };

  BlockingQueue() : data_() {}
  explicit BlockingQueue(size_t max_size) : data_(max_size) {}

  void push(const T& x) {
    data_.push(x);
  }

  void push(T&& x) {
    data_.push(std::move(x));
  }

  T pop() {
    return data_.pop();
  }

  size_t size() const {
    return data_.size();
  }
  size_t empty() const {
    return data_.empty();
  }
  bool full() const {
    return data_.full();
  }
  size_t capacity() const {
    return data_.capacity();
  }

private:
  Data data_;
};

}  // namespace annety

#endif  // ANT_BLOCKING_QUEUE_H
