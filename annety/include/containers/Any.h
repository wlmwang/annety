// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jul 14 2019

#ifndef ANT_CONTAINERS_ANY_H
#define ANT_CONTAINERS_ANY_H

#include "Logging.h"

#include <memory>		// std::unique_ptr
#include <utility>		// std::move,std::forward
#include <type_traits>	// std::is_same,std::decay,std::enable_if
#include <typeinfo>		// typeid
#include <typeindex>	// std::type_index

// Suggest use std::any since C++17, defined in header <any>

namespace annety
{
namespace containers
{
// Example:
// // Any
// Any a = 1;
// Any b = string("hello, world");
// Any c;
// Any d = make_any(a);
//
// cout << std::boolalpha;
// cout << "a has value:" << a.has_value() << endl;
// cout << "b has value:" << b.has_value() << endl;
// cout << "c has value:" << c.has_value() << endl;
// cout << "d has value:" << d.has_value() << endl;
//
// cout << "a is int:" << a.type().name() << endl;
// cout << "a cast to int:" << any_cast<int>(a) << endl;
//
// any_cast<int>(a) = 2;
// cout << "a cast to int:" << any_cast<int>(a) << endl;
//
// c = a;
// cout << "c cast to int:" << any_cast<int>(c) << endl;
// cout << "c cast to string:" << any_cast<string>(c) << endl;
// ...

class Any;
template <typename T> T& any_cast(const Any&);

class Any
{
public:
	Any() : type_(std::type_index(typeid(void))) {}

	// FIXME: the following four methods do not exist in std::any.	
	// Please use annety::make_any, which is consistent with std::any.

	Any(const Any& rhs) : ptr_(rhs.clone()), type_(rhs.type_) {}
	Any(Any&& rhs) : ptr_(std::move(rhs.ptr_)), type_(rhs.type_) {}

	// The std::enable_if<> type-traits is limited to non-Any type
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
	
	template <typename T>
	T& any_cast() const
	{
		// Maybe throw std::bad_any_cast like as C++17
		if (!is<T>()) {
			LOG(ERROR) << "can not cast " << typeid(T).name() 
					<< " to " << type_.name();
			throw std::bad_cast();
		}
		// FIXME: use dynamic_cast<>
		auto derived = static_cast<Derived<T>*>(ptr_.get());
		return derived->value_;
	}

private:
	// For access private Any::any_cast if we following std::any
	template <typename T> friend T& any_cast(const Any&);

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

}	// namespace containers
}	// namespace annety

#endif	// ANT_CONTAINERS_ANY_H
