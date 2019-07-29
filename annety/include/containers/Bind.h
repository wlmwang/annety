#ifndef ANT_BIND_H
#define ANT_BIND_H

#include <type_traits>
#include <tuple>
#include <utility>

namespace annety
{
namespace containers
{
// Placeholder ------------------------------------------------------------------
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

// Sequence ------------------------------------------------------------------
template <int...>
struct IndexTuple {};

template <int N, int... Indexes>
struct MakeIndexes : MakeIndexes<N - 1, N - 1, Indexes...> {};

template <int... indexes>
struct MakeIndexes<0, indexes...>
{
    using type = IndexTuple<indexes...>;
};

// result type ------------------------------------------------------------------
// **std::result_of in C++11 is too limited. That C++17 is become better**
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

// Type trait ------------------------------------------------------------------
template <typename T>
struct IsWrapper : std::false_type {};

template <typename T>
struct IsWrapper<std::reference_wrapper<T>> : std::true_type {};

template <typename T>
using IsWrapperNoref = IsWrapper<typename std::remove_reference<T>::type>;

template <typename T>
using IsPointerNoref = std::is_pointer<typename std::remove_reference<T>::type>;

template <typename T>
using IsMemfuncNoref = std::is_member_function_pointer<typename std::remove_reference<T>::type>;

// Select the value of tuple ----------------------------------------------------
template <typename T, class Tuple>
inline auto select(T&& val, Tuple&) -> T&&
{
    return std::forward<T>(val);
}

template <int I, class Tuple>
inline auto select(Placeholder<I>&, Tuple& tp) -> decltype(std::get<I - 1>(tp))
{
    return std::get<I - 1>(tp);
}

// The invoker for call a callable -----------------------------------------------
template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
    -> typename std::enable_if<IsPointerNoref<F>::value,
       typename std::result_of<decltype(*std::declval<F&&>())(P&&...)>::type
    >::type
{
    return (*std::forward<F>(f))(std::forward<P>(args)...);
}

template <typename F, typename C, typename... P>
inline auto invoke(F&& f, C&& this_ptr, P&&... args)
    -> typename std::enable_if<IsMemfuncNoref<F>::value && IsPointerNoref<C>::value,
       typename std::result_of<F&&(C&&, P&&...)>::type
    >::type
{
    return (std::forward<C>(this_ptr)->*std::forward<F>(f))(std::forward<P>(args)...);
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

template <typename F, typename... P>
inline auto invoke(F&& f, P&&... args)
    -> typename std::enable_if<!IsPointerNoref<F>::value && !IsMemfuncNoref<F>::value,
       typename std::result_of<F&&(P&&...)>::type
    >::type
{
    return std::forward<F>(f)(std::forward<P>(args)...);
}


// functor for bind callable type and arguments -------------------------------------
template<typename FuncT, typename... Args>
class Bind_t
{
private:
    using FucType   = typename std::decay<FuncT>::type;
    using ArgType   = std::tuple<typename std::decay<Args>::type...>;
    using ResType   = typename ResultOf<FucType>::type;

    template <class ArgTuple, int... Indexes>
    auto do_call(ArgTuple&& tp, IndexTuple<Indexes...>) -> ResType
    {
        return invoke(call_, select(std::get<Indexes>(args_), tp)...);
    }

public:
    template <typename F, typename... P>
    Bind_t(F&& f, P&&... pars)
        : call_(std::forward<F>(f))
        , args_(std::forward<P>(pars)...) {}

    template <typename... A>
    auto operator()(A&&... args) -> ResType
    {
        using IndexType = typename MakeIndexes<std::tuple_size<ArgType>::value>::type;
        return do_call(std::forward_as_tuple(std::forward<A>(args)...), IndexType());
    }

private:
    FucType call_;
    ArgType args_;
};


// Bind function arguments ---------------------------------------------------
template <typename F, typename... P>
inline Bind_t<F&&, P&&...> make_bind(F&& f, P&&... pars)
{
    return {std::forward<F>(f), std::forward<P>(pars)...};
}

}   // namespace containers
}	// namespace annety

#endif  // ANT_BIND_H
