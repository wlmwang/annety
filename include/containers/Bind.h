// By: wlmwang
// Date: Jul 28 2019

#ifndef ANT_CONTAINERS_BIND_H
#define ANT_CONTAINERS_BIND_H

#include "Logging.h"

#include <type_traits>	// std::is_pointer,std::remove_reference
#include <tuple>		// std::tuple,std::get
#include <memory>		// std::shared_ptr,std::weak_ptr
#include <utility>		// std::forward,std::forward_as_tuple

// FIXME: Do not support binding lambda expressions yet, and it is 
// not good for right-value!!!

// FIXME: Suggest use weak_from_this() since C++17, it is defined 
// in std::enable_shared_from_this<T>::weak_from_this().

namespace annety
{
// Example:
// // WBindFuctor
// void TestFun(int a, int b, int c) {}
// class WapperCall {
// 		void test(int c) {}
// };
//
// std::function<void(int)> xx;
// {
//		xx = containers::make_bind(TestFun, 1, 2, containers::_1);
//		xx(3);
//
//		std::shared_ptr<WapperCall> call(new WapperCall());
//		xx = containers::make_bind(&WapperCall::test, call, containers::_1);
//		cout << "use cout:" << call.use_count() << endl;
//		xx(10);
//
//		xx = containers::make_weak_bind(&WapperCall::test, call, containers::_1);
//		cout << "use cout:" << call.use_count() << endl;
//		xx(11);
//
//		std::weak_ptr<WapperCall> wcall = call;
//		xx = containers::make_weak_bind(&WapperCall::test, wcall, containers::_1);
//		cout << "use cout:" << call.use_count() << endl;
//		xx(12);
// }
// if (xx) xx(20);
// ...

namespace containers
{
// Placeholder --------------------------------------------------------------------------
template <int N>
struct Placeholder {};

static Placeholder<1> ALLOW_UNUSED_TYPE _1; static Placeholder<11> ALLOW_UNUSED_TYPE _11;
static Placeholder<2> ALLOW_UNUSED_TYPE _2; static Placeholder<12> ALLOW_UNUSED_TYPE _12;
static Placeholder<3> ALLOW_UNUSED_TYPE _3; static Placeholder<13> ALLOW_UNUSED_TYPE _13;  
static Placeholder<4> ALLOW_UNUSED_TYPE _4; static Placeholder<14> ALLOW_UNUSED_TYPE _14;
static Placeholder<5> ALLOW_UNUSED_TYPE _5; static Placeholder<15> ALLOW_UNUSED_TYPE _15;  
static Placeholder<6> ALLOW_UNUSED_TYPE _6; static Placeholder<16> ALLOW_UNUSED_TYPE _16;
static Placeholder<7> ALLOW_UNUSED_TYPE _7; static Placeholder<17> ALLOW_UNUSED_TYPE _17;
static Placeholder<8> ALLOW_UNUSED_TYPE _8; static Placeholder<18> ALLOW_UNUSED_TYPE _18;
static Placeholder<9> ALLOW_UNUSED_TYPE _9; static Placeholder<19> ALLOW_UNUSED_TYPE _19; 
static Placeholder<10> ALLOW_UNUSED_TYPE _10; static Placeholder<20> ALLOW_UNUSED_TYPE _20;

// Sequence --------------------------------------------------------------------------
// see: std::integer_sequence/std::make_index_sequence() in C++14
template <int...>
struct IndexSequence {};

template <int N, int... Indexes>
struct MakeIndexSequence : MakeIndexSequence<N - 1, N - 1, Indexes...> {};

template <int... indexes>
struct MakeIndexSequence<0, indexes...>
{
	using type = IndexSequence<indexes...>;
};

// return type --------------------------------------------------------------------------
// see: std::result_of in C++11 is too limited. That C++17 is become better
template <typename T>
struct ResultOf;

template <typename T>
struct ResultOf<T*> : ResultOf<T> {};

// function
template <typename R, typename... P>
struct ResultOf<R(*)(P...)> { typedef R type; };

// class method
template <typename R, typename C, typename... P> 
struct ResultOf<R(C::*)(P...)> { typedef R type; };

// callable
template <typename F>
struct ResultOf : ResultOf<decltype(&F::operator())> {};

// Type trait --------------------------------------------------------------------------
template <typename T> struct IsWrapper : std::false_type {};
template <typename T> struct IsWrapper<std::reference_wrapper<T>> : std::true_type {};
template <typename T> using IsWrapperNoref = IsWrapper<typename std::remove_reference<T>::type>;

template<typename T> struct IsUniquePtr : std::false_type {};
template<typename T> struct IsUniquePtr<std::unique_ptr<T>> : std::true_type {};

template<typename T> struct IsSharedPtr : std::false_type {};
template<typename T> struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

template<typename T> struct IsWeakPtr : std::false_type {};
template<typename T> struct IsWeakPtr<std::weak_ptr<T>> : std::true_type {};

template <typename T>
using IsPointerNoref = std::integral_constant<bool, 
			std::is_pointer<typename std::remove_reference<T>::type>::value ||
			IsUniquePtr<typename std::remove_reference<T>::type>::value ||
			IsSharedPtr<typename std::remove_reference<T>::type>::value
			// || IsWeakPtr<typename std::remove_reference<T>::type>::value>;
		>;

template <typename T>
using IsMemfuncNoref = std::is_member_function_pointer<typename std::remove_reference<T>::type>;

// Select the value of tuple --------------------------------------------------------------
template <typename T, class Tuple>
inline auto select(T&& val, Tuple&) -> T&&
{
	return std::forward<T>(val);
}

template <int I, class Tuple>
inline auto select(Placeholder<I>&, Tuple& tp) -> decltype(std::get<I-1>(tp))
{
	return std::get<I-1>(tp);
}

// The invoker for call a callable --------------------------------------------------------
template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
	-> typename std::enable_if<IsPointerNoref<F>::value,
		typename std::result_of<decltype(*std::declval<F&&>())(P&&...)>::type
	>::type
{
	return (*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
	-> typename std::enable_if<!IsPointerNoref<F>::value && !IsMemfuncNoref<F>::value,
		typename std::result_of<F&&(P&&...)>::type
	>::type
{
	return std::forward<F>(f)(std::forward<P>(args)...);
}

template <typename F, typename C, typename... P>
inline auto invoke(F&& f, C&& this_ptr, P&&... args)
	-> typename std::enable_if<IsMemfuncNoref<F>::value && IsPointerNoref<C>::value,
		typename std::result_of<F&&(C&&, P&&...)>::type
	>::type
{
	return (*std::forward<C>(this_ptr).*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename C, typename... P>
inline auto invoke(F&& f, C&& this_obj, P&&... args)
	-> typename std::enable_if<IsMemfuncNoref<F>::value && !IsPointerNoref<C>::value && !IsWrapperNoref<C>::value,
		typename std::result_of<F&&(C&&, P&&...)>::type
	>::type
{
	return (std::forward<C>(this_obj).*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename C, typename... P>
inline auto invoke(F&& f, C&& this_wrp, P&&... args)
	-> typename std::enable_if<IsMemfuncNoref<F>::value && !IsPointerNoref<C>::value && IsWrapperNoref<C>::value,
		typename std::result_of<F&&(typename std::remove_reference<C>::type::type, P&&...)>::type
	>::type
{
	typedef typename std::remove_reference<C>::type wrapper_t;
	typedef typename wrapper_t::type this_t;
	this_t& this_obj(std::forward<C>(this_wrp));
	return (this_obj.*std::forward<F>(f))(std::forward<P>(args)...);
}

// functor for bind callable type and arguments ---------------------------------------------
template <typename FuncT, typename... ParsT>
class BindFuctor
{
private:
	using FuncType  = typename std::decay<FuncT>::type;
	using ParsType  = std::tuple<typename std::decay<ParsT>::type...>;
	using ResfType  = typename ResultOf<FuncType>::type;

	template <class ArgTuple, int... Indexes>
	auto do_call(ArgTuple&& tp, IndexSequence<Indexes...>) -> ResfType
	{
		return invoke(std::forward<FuncType>(call_), select(std::get<Indexes>(args_), tp)...);
	}

public:
	BindFuctor(FuncT&& f, ParsT&&... pars)
		: call_(std::forward<FuncT>(f))
		, args_(std::forward<ParsT>(pars)...) {}

	template <typename... ARGS>
	auto operator()(ARGS&&... args) -> ResfType
	{
		using IndexType = typename MakeIndexSequence<std::tuple_size<ParsType>::value>::type;
		return do_call(std::forward_as_tuple(std::forward<ARGS>(args)...), IndexType());
	}

private:
	FuncType call_;
	ParsType args_;
};

// functor for weak bind callable type and arguments
template <typename FuncT, typename CLASS, typename... ParsT>
class WBindFuctor
{
private:
	using FuncType  = typename std::decay<FuncT>::type;
	using ParsType  = std::tuple<typename std::decay<ParsT>::type...>;
	using ResfType  = typename ResultOf<FuncType>::type;
	using WeakType  = std::weak_ptr<CLASS>;

	template <class ArgTuple, int... Indexes>
	auto do_call(ArgTuple&& tp, IndexSequence<Indexes...>) -> ResfType
	{
		std::shared_ptr<CLASS> locked = weak_.lock();
		if (locked) {
			return invoke(call_, locked, select(std::get<Indexes>(args_), tp)...);
		}
		LOG(WARNING) << "the shared_ptr has destructed";
	}

public:
	WBindFuctor(FuncT&& f, WeakType weak, ParsT&&... pars)
		: call_(std::forward<FuncT>(f))
		, args_(std::forward<ParsT>(pars)...)
		, weak_(weak) {}

	template <typename U>
	WBindFuctor(FuncT&&, U&&, ParsT&&...) = delete;

	template <typename... ARGS>
	auto operator()(ARGS&&... args) -> ResfType
	{
		using IndexType = typename MakeIndexSequence<std::tuple_size<ParsType>::value>::type;
		return do_call(std::forward_as_tuple(std::forward<ARGS>(args)...), IndexType());
	}

private:
	FuncType call_;
	ParsType args_;
	WeakType weak_;
};

// make callable object ----------------------------------------------------------------
template <typename F, typename... P>
inline BindFuctor<F&&, P&&...> make_bind(F&& fuctor, P&&... pars)
{
	return {std::forward<F>(fuctor), std::forward<P>(pars)...};
}

// make callable object (weak callback) ------------------------------------------------
// |ptr| do not is reference parameter
template <typename F, typename C, typename... P>
inline WBindFuctor<F&&, C, P&&...> make_weak_bind(F&& fuctor, std::shared_ptr<C> ptr, P&&... pars)
{
	std::weak_ptr<C> weak{ptr};
	return {std::forward<F>(fuctor), weak, std::forward<P>(pars)...};
}

template <typename F, typename C, typename... P>
inline WBindFuctor<F&&, C, P&&...> make_weak_bind(F&& fuctor, std::weak_ptr<C> ptr, P&&... pars)
{
    return {std::forward<F>(fuctor), ptr, std::forward<P>(pars)...};
}

}   // namespace containers
}	// namespace annety

#endif  // ANT_CONTAINERS_BIND_H
