// Refactoring: Anny Wang
// Date: Jul 07 2019

#ifndef ANT_WEAK_CALLBACK_H_
#define ANT_WEAK_CALLBACK_H_

#include <functional>
#include <memory>

namespace annety
{
// a barely usable weak-callback of class
template <typename CLASS, typename... ARGS>
class WeakCallback
{
public:
	using WeakCallbackRef = std::weak_ptr<CLASS>;
	using WeakCallbackType = std::function<void(CLASS*, ARGS...)>;

	WeakCallback(const WeakCallbackRef& ref, const WeakCallbackType& cb)
		: ref_(ref), cb_(cb) {}

	WeakCallback(const WeakCallback&) = default;
	WeakCallback& operator=(const WeakCallback&) = default;
	~WeakCallback() = default;
	
	void operator()(ARGS&&... args) const
	{
		std::shared_ptr<CLASS> ptr(ref_.lock());
		if (ptr) {
			cb_(ptr.get(), std::forward<ARGS>(args)...);
		}
	}

private:
	WeakCallbackRef ref_;
	WeakCallbackType cb_;
};

// template function for make WeakCall ----------------------------------
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> make_weak_callback(const std::shared_ptr<CLASS>& obj,
											    void(CLASS::*func)(ARGS...))
{
	return WeakCallback<CLASS, ARGS...>(obj, func);
}

// const class-method
template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> make_weak_callback(const std::shared_ptr<CLASS>& obj,
											  void(CLASS::*func)(ARGS...) const)
{
	return WeakCallback<CLASS, ARGS...>(obj, func);
}

}	// namespace annety

#endif	// ANT_WEAK_CALLBACK_H_
