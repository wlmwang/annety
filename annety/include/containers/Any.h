// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jul 14 2019

#ifndef ANT_ANY_H
#define ANT_ANY_H

#include "Logging.h"

#include <memory>
#include <utility>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

namespace annety
{
class Any;
template <typename T> T& any_cast(const Any&);

// use std::any since C++17. defined in header <any>
class Any
{
public:
	Any() : type_(std::type_index(typeid(void))) {}

	// FIXME: the following four methods do not exist in std::any
	// please use annety::make_any, which is consistent with std::any
	Any(const Any& rhs) : ptr_(rhs.clone()), type_(rhs.type_) {}
	Any(Any&& rhs) : ptr_(std::move(rhs.ptr_)), type_(rhs.type_) {}

	// the std::enable_if<> type-traits is limited to non-Any type
	template <typename U, typename = typename std::enable_if<
							!std::is_same<typename std::decay<U>::type, Any>::value
							, U>::type> 
	Any(U&& value) 
		: ptr_(new Derived<typename std::decay<U>::type>(std::forward<U>(value)))
		, type_(std::type_index(typeid(typename std::decay<U>::type))) {}

	Any& operator=(const Any& other)
	{
		if (ptr_ == other.ptr_) {
			return *this;
		}
		ptr_ = other.clone();
		type_ = other.type_;
		return *this;
	}
	
	void reset()
	{
		ptr_.reset();
		type_ = std::type_index(typeid(void));
	}

	bool has_value() const
	{
		return bool(ptr_);
	}
	
	std::type_index type() const
	{
		return type_;
	}

private:
	// for access private Any::any_cast if we following std::any
	template <typename T> friend T& annety::any_cast(const Any&);

	template <typename T>
	T& any_cast() const
	{
		// throw std::bad_any_cast in C++17
		if (!is<T>()) {
			LOG(ERROR) << "can not cast " << typeid(T).name() 
					<< " to " << type_.name();
			throw std::bad_cast();
		}
		// FIXME: use dynamic_cast<>
		auto derived = static_cast<Derived<T>*>(ptr_.get());
		return derived->value_;
	}

	template <typename T>
	bool is() const
	{
		return type_ == std::type_index(typeid(T));
	}

	// impl
	struct Base;
	using BasePtr = std::unique_ptr<Base>;

	struct Base
	{
		virtual ~Base() {}
		virtual BasePtr clone() const = 0;
	};

	template <typename T>
	struct Derived : public Base
	{
		template<typename U>
        Derived(U&& value) : value_(std::forward<U>(value)) {}

		BasePtr clone() const
		{
			return BasePtr(new Derived(value_));
		}

		T value_;
	};

	BasePtr clone() const
	{
		if (ptr_) {
			return ptr_->clone();
		}
		return nullptr;
	}

private:
	BasePtr	ptr_;
	std::type_index	type_;
};

template <typename T>
T& any_cast(const Any& value)
{
	return value.any_cast<T>();
}

template <typename T>
Any make_any(T&& value)
{
	return Any(std::forward<T>(value));
}

}	// namespace annety

#endif	// ANT_ANY_H
