// By: wlmwang
// Date: Jun 02 2019

#ifndef ANT_SYNCHRONIZATION_BLOCKING_QUEUE_H
#define ANT_SYNCHRONIZATION_BLOCKING_QUEUE_H

#include "Macros.h"
#include "Logging.h"
#include "synchronization/MutexLock.h"
#include "synchronization/ConditionVariable.h"

#include <deque>
#include <utility>

namespace annety
{
// Example:
// // BlockingQueue
// // BlockingQueue<int> queue;
// BlockingQueue<int, BoundedBlockingTrait<int>> queue(2);
// Thread producer([&queue]() {
//		for (int i = 0; i < 10; i++) {
//			queue.push(i);
//			LOG(INFO) << "producer:" << i << "|" << pthread_self();
//		}
// }, "annety-producer");
// producer.start();
//
// // test for BoundedBlockingTrait
// // sleep(1);
//
// Thread consumer([&queue]() {
//		while(true) {
// 			LOG(INFO) << "consumer:" << queue.pop() << "|" << pthread_self();
// 		}
// }, "annety-consumer");
// consumer.start();
//
// producer.join();
// consumer.join();	// will hold
// ...

// A blocking queue is a queue that can be used in a multi-thread, it 
// supports blocking waiting.  The difference between a blocking queue 
// and a normal queue is:
// 1. Supports multiple threads. Multiple threads can safely access 
//	  the queue.
// 2. Supports producer and consumer. When the queue is empty, the 
//	  consumer thread will block and wait for the queue to become non-empty, 
//	  when the queue is full, the producer thread will block until the queue 
//	  is non-full.
template <typename T> class UnBoundedBlockingTrait;
template <typename T> class BoundedBlockingTrait;

// {,Un}Boundedblocking queue
template <typename T, typename Traits = UnBoundedBlockingTrait<T>>
class BlockingQueue
{
private:
	struct Data : public Traits
	{
		Data() : Traits() {}
		explicit Data(size_t max_size) : Traits(max_size) {}
	};

public:
	typedef T element_type;
	typedef Traits traits_type;

	BlockingQueue() : data_() {}
	explicit BlockingQueue(size_t max_size) : data_(max_size) {}

	~BlockingQueue() = default;

	void push(const element_type& x)
	{
		data_.push(x);
	}
	void push(element_type&& x)
	{
		data_.push(std::move(x));
	}

	element_type pop()
	{
		return data_.pop();
	}

	size_t size() const
	{
		return data_.size();
	}
	size_t empty() const
	{
		return data_.empty();
	}
	bool full() const
	{
		return data_.full();
	}
	size_t capacity() const
	{
		return data_.capacity();
	}

private:
	Data data_;

	DISALLOW_COPY_AND_ASSIGN(BlockingQueue);
};

// Blocking Trait ----------------------------------------
// UnBoundedBlockingTrait
template <typename T>
class UnBoundedBlockingTrait
{
public:
	UnBoundedBlockingTrait() {}
	UnBoundedBlockingTrait(size_t max_size) = delete;

	~UnBoundedBlockingTrait() = default;

	void push(const T& x)
	{
		AutoLock locked(lock_);
		queue_.push_back(x);
		empty_cv_.signal();
	}
	void push(T&& x)
	{
		AutoLock locked(lock_);
		queue_.push_back(std::move(x));
		empty_cv_.signal();
	}

	T pop()
	{
		AutoLock locked(lock_);
		while (empty_with_locked()) {
			empty_cv_.wait();
		}
		DCHECK(!empty_with_locked());

		T front(std::move(queue_.front()));
		queue_.pop_front();
		return front;
	}

	size_t size() const
	{
		AutoLock locked(lock_);
		return size_with_locked();
	}

	bool empty() const
	{
		AutoLock locked(lock_);
		return empty_with_locked();
	}

	bool full() const
	{
		AutoLock locked(lock_);
		return full_with_locked();
	}

	size_t capacity() const
	{
		AutoLock locked(lock_);
		return capacity_with_locked();
	}

private:
	size_t size_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.size();
	}  
	bool empty_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.empty();
	}
	bool full_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.size() >= queue_.max_size();
	}
	size_t capacity_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.max_size() - queue_.size();
	}

private:
	mutable MutexLock lock_;
	ConditionVariable empty_cv_{lock_};

	std::deque<T> queue_;
};

// BoundedBlockingTrait
template <typename T>
class BoundedBlockingTrait
{
public:
	BoundedBlockingTrait() = delete;
	explicit BoundedBlockingTrait(size_t max_size) 
				: max_size_(max_size) {}

	~BoundedBlockingTrait() = default;

	void push(const T& x)
	{
		AutoLock locked(lock_);
		while (full_with_locked()) {
			full_cv_.wait();
		}
		DCHECK(!full_with_locked());

		queue_.push_back(x);
		empty_cv_.signal();
	}
	void push(T&& x)
	{
		AutoLock locked(lock_);
		while (full_with_locked()) {
			full_cv_.wait();
		}
		DCHECK(!full_with_locked());

		queue_.push_back(std::move(x));
		empty_cv_.signal();
	}

	T pop()
	{
		AutoLock locked(lock_);
		while (empty_with_locked()) {
			empty_cv_.wait();
		}
		DCHECK(!empty_with_locked());

		T front(std::move(queue_.front()));
		queue_.pop_front();
		full_cv_.signal();
		return front;
	}

	size_t size() const
	{
		AutoLock locked(lock_);
		return size_with_locked();
	}

	bool empty() const
	{
		AutoLock locked(lock_);
		return empty_with_locked();
	}

	bool full() const
	{
		AutoLock locked(lock_);
		return full_with_locked();
	}

	size_t capacity() const
	{
		AutoLock locked(lock_);
		return capacity_with_locked();
	}

private:
	size_t size_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.size();
	}  
	bool empty_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.empty();
	}
	bool full_with_locked() const
	{
		lock_.assert_acquired();
		return queue_.size() >= max_size_;
	}
	size_t capacity_with_locked() const
	{
		lock_.assert_acquired();
		return max_size_ - queue_.size();
	}

private:
	mutable MutexLock lock_;
	ConditionVariable empty_cv_{lock_};
	ConditionVariable full_cv_{lock_};

	std::deque<T> queue_;
	size_t max_size_;
};

}	// namespace annety

#endif  // ANT_SYNCHRONIZATION_BLOCKING_QUEUE_H
