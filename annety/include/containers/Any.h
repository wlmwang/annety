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
// use std::any since C++17. defined in header <any>
class Any
{
public:
	// default ctor
	Any() : type_(std::type_index(typeid(void))) {}
	Any(const Any& rhs) : ptr_(rhs.clone()), type_(rhs.type_) {}
	Any(Any&& rhs) : ptr_(std::move(rhs.ptr_)), type_(rhs.type_) {}

	// move ctor
	template <class U, class = typename std::enable_if<
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

	bool is_null()
	{
		return !bool(ptr_);
	}

	template<typename U>
	bool is()
	{
		return type_ == std::type_index(typeid(U));
	}

	template<typename U>
	U& any_cast()
	{
		// FIXME: use CHECK() macros
		if (!is<U>()) {
			LOG(ERROR) << "can not cast " << typeid(U).name() 
					<< " to " << type_.name();
			throw std::bad_cast();
		}
		// FIXME: use static_cast<>
		auto derived = dynamic_cast<Derived<U>*>(ptr_.get());
		return derived->value_;
	}

private:
	struct Base;
	using BasePtr = std::unique_ptr<Base>;

	struct Base
	{
		virtual ~Base() {}
		virtual BasePtr clone() const = 0;
	};

	template<typename T>
	struct Derived : public Base
	{
		template<typename U>
        Derived(U&& value) : value_(std::forward<U>(value)) { }

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

}	// namespace annety

#endif	// ANT_ANY_H
