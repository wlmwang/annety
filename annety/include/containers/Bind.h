#ifndef ANT_BIND_H
#define ANT_BIND_H

#include <type_traits>
#include <tuple>
#include <utility>

// _support_late_binding_return_value

namespace simple {

// Placeholder ------------------------------------------------------------------

template <int N>
struct placeholder {};

static placeholder<1> ALLOW_UNUSED_TYPE _1; static placeholder<11> ALLOW_UNUSED_TYPE _11;
static placeholder<2> ALLOW_UNUSED_TYPE _2; static placeholder<12> ALLOW_UNUSED_TYPE _12;
static placeholder<3> ALLOW_UNUSED_TYPE _3; static placeholder<13> ALLOW_UNUSED_TYPE _13;  
static placeholder<4> ALLOW_UNUSED_TYPE _4; static placeholder<14> ALLOW_UNUSED_TYPE _14;
static placeholder<5> ALLOW_UNUSED_TYPE _5; static placeholder<15> ALLOW_UNUSED_TYPE _15;  
static placeholder<6> ALLOW_UNUSED_TYPE _6; static placeholder<16> ALLOW_UNUSED_TYPE _16;
static placeholder<7> ALLOW_UNUSED_TYPE _7; static placeholder<17> ALLOW_UNUSED_TYPE _17;
static placeholder<8> ALLOW_UNUSED_TYPE _8; static placeholder<18> ALLOW_UNUSED_TYPE _18;
static placeholder<9> ALLOW_UNUSED_TYPE _9; static placeholder<19> ALLOW_UNUSED_TYPE _19; 
static placeholder<10> ALLOW_UNUSED_TYPE _10; static placeholder<20> ALLOW_UNUSED_TYPE _20;

// Sequence & Generater -------------------------------------------------------------

template <int... N>
struct seq { typedef seq<N..., sizeof...(N)> next_type; };

template <typename... P>
struct gen;

template <>
struct gen<>
{
    typedef seq<> seq_type;
};

template <typename P1, typename... P>
struct gen<P1, P...>
{
    typedef typename gen<P...>::seq_type::next_type seq_type;
};


// Type list --------------------------------------------------------------

template <typename... TypesT>
struct type_list;

// insert
template <typename T, typename ArrayT>
struct type_insert;

template <typename T, typename... TypesT>
struct type_insert<T, type_list<TypesT...>>
{
    typedef type_list<T, TypesT...> type;
};

// at
template <int N, typename ArrayT>
struct type_at;

template <typename T1, typename... TypesT>
struct type_at<0, type_list<T1, TypesT...>>
{
    typedef T1 type;
};

template <int N, typename T1, typename... TypesT>
struct type_at<N, type_list<T1, TypesT...>>
     : type_at<N - 1, type_list<TypesT...>>
{};


// Select the type of tuple -------------------------------------------

template <class TupleT, typename... BindT>
struct merge;

template <typename... ParsT>
struct merge<std::tuple<ParsT...>>
{
    typedef type_list<> type;
};

template <typename... ParsT, typename B1, typename... BindT>
struct merge<std::tuple<ParsT...>, B1, BindT...>
{
    typedef std::tuple<ParsT...> tp_t;
    typedef typename type_insert<
            B1,
            typename merge<tp_t, BindT...>::type
    >::type type;
};

template <typename... ParsT, int N, typename... BindT>
struct merge<std::tuple<ParsT...>, placeholder<N>&, BindT...>
{
    typedef std::tuple<ParsT...> tp_t;
    typedef typename type_insert<
            typename std::tuple_element<N - 1, tp_t>::type,
            typename merge<tp_t, BindT...>::type
    >::type type;
};

template <int N, class TupleT, typename... BindT>
using select_t = typename type_at<N, typename merge<TupleT, BindT...>::type>::type;


// Select the value of tuple ----------------------------------------------------------

template <typename R, typename T, class TupleT>
inline R select(TupleT& /*tp*/, T&& val)
{
	return static_cast<R>(val);
}

template <typename R, int N, class TupleT>
inline R select(TupleT& tp, placeholder<N>)
{
	return static_cast<R>(std::get<N - 1>(tp));
}

// Type concept ----------------------------------------------------------------------

template <typename T>
using is_pointer_noref = std::is_pointer<typename std::remove_reference<T>::type>;

template <typename T>
using is_memfunc_noref = std::is_member_function_pointer<typename std::remove_reference<T>::type>;

template <typename T>
struct is_wrapper
    : std::false_type
{};

template <typename T>
struct is_wrapper<std::reference_wrapper<T>>
    : std::true_type
{};

template <typename T>
using is_wrapper_noref = is_wrapper<typename std::remove_reference<T>::type>;


// The invoker for call a callable ------------------------------------------------------------

template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
    -> typename std::enable_if<is_pointer_noref<F>::value,
       typename std::result_of<decltype(*std::declval<F&&>())(P&&...)>::type
    >::type
{
    return (*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename P1, typename... P>
inline auto invoke(F&& f, P1&& this_ptr, P&&... args)
    -> typename std::enable_if<is_memfunc_noref<F>::value && is_pointer_noref<P1>::value,
       typename std::result_of<F&&(P1&&, P&&...)>::type
    >::type
{
    return (std::forward<P1>(this_ptr)->*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename P1, typename... P>
inline auto invoke(F&& f, P1&& this_obj, P&&... args)
    -> typename std::enable_if<is_memfunc_noref<F>::value && !is_pointer_noref<P1>::value && !is_wrapper_noref<P1>::value,
       typename std::result_of<F&&(P1&&, P&&...)>::type
    >::type
{
    return (std::forward<P1>(this_obj).*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename P1, typename... P>
inline auto invoke(F&& f, P1&& this_wrp, P&&... args)
    -> typename std::enable_if<is_memfunc_noref<F>::value && !is_pointer_noref<P1>::value && is_wrapper_noref<P1>::value,
       typename std::result_of<F&&(typename std::remove_reference<P1>::type::type, P&&...)>::type
    >::type
{
    typedef typename std::remove_reference<P1>::type wrapper_t;
    typedef typename wrapper_t::type this_t;
    this_t& this_obj (std::forward<P1>(this_wrp));
    return (this_obj.*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
    -> typename std::enable_if<!is_pointer_noref<F>::value && !is_memfunc_noref<F>::value,
       typename std::result_of<F&&(P&&...)>::type
    >::type
{
    return std::forward<F>(f)(std::forward<P>(args)...);
}


// Simple functor for bind callable type and arguments ------------------------------------------

template<typename FuncT, typename... ParsT>
class fr
{
	using seq_type		= typename gen<ParsT...>::seq_type;
	using callable_type	= typename std::decay<FuncT>::type;
	using args_type		= std::tuple<typename std::decay<ParsT>::type...>;

	callable_type call_;
	args_type     args_;

	template <class TupleT, int... N>
	auto do_call(TupleT&& tp, seq<N...>)
		-> decltype(invoke(call_, std::declval<select_t<N, TupleT, ParsT...>>()...))
	{
		return invoke(call_, select<select_t<N, TupleT, ParsT...>>(tp, std::get<N>(args_))...);
	}

public:
	template <typename F, typename... P>
	fr(F&& f, P&&... args)
		: call_(std::forward<F>(f))
		, args_(std::forward<P>(args)...) {}

	template <typename... P>
	auto operator()(P&&... args)
		-> decltype(this->do_call(std::declval<std::tuple<P&&...>>(), std::declval<seq_type>()))
	{
		return this->do_call(std::forward_as_tuple(std::forward<P>(args)...), seq_type());
	}
};


// Bind function arguments ---------------------------------------------------------------------

template <typename F, typename... P>
inline fr<F&&, P&&...> bind(F&& f, P&&... args)
{
	return {std::forward<F>(f), std::forward<P>(args)...};
}

}	// namespace simple

#endif  // ANT_BIND_H
