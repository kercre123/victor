/**
 * File: lazyFunctor.h
 *
 * Author: Michael Willett
 * Created: 10/2/2018
 *
 * Description: 
 * 
 *      Lazily evaluated function pointers (arguments are not evaulated until derefernce)

 * 
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __UTIL_LAZY_H__
#define __UTIL_LAZY_H__

#include <type_traits>


namespace Anki{ 
namespace Util {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helpers for unpacking a deffered parameter pack
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  template <typename Tuple>
  using GetIdx = std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>;

  template<typename Func, typename Tuple, size_t... S >
  inline decltype(auto) ForwardTupleArgs(Func&& f, Tuple&& t, std::index_sequence<S...>) {
    return std::forward<Func>(f)(std::get<S>(std::forward<Tuple>(t))...);
  }

  template<typename Func, typename Tuple>
  inline decltype(auto) ForwardTupleArgs(Func&& f, Tuple&& t) {
    return ForwardTupleArgs(std::forward<Func>(f), std::forward<Tuple>(t), GetIdx<Tuple>());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Construction Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define MAKE_LAZY_MEMBER(class, func) \
  auto func = MakeLazyMember<class>(std::mem_fn(&class::func))

template <typename Func, typename T> class PostfixFunctor;

template <typename T, typename Func>
inline PostfixFunctor<Func, T> MakeLazyMember(Func&& f) {
  return PostfixFunctor<Func, T>(std::forward<Func>(f));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Functor Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename Func, typename T, typename... Tail> 
class LazyUnaryFunctor
{
protected:
  friend class PostfixFunctor<Func, T>;
  using Pack = std::tuple<Tail&&...>;

  Func op;
  Pack values;

  LazyUnaryFunctor(Func&& f, Tail&&... ts) 
  : op( std::forward<Func>(f) )
  , values( std::forward<Tail>(ts)... ) {}

public:
  decltype(auto) operator() (T& t) { 
    return ForwardTupleArgs(std::forward<Func>(op), 
                            std::tuple_cat(std::tuple<T&&>(std::forward<T>(t)), std::forward<Pack>(values)));
  }
};

template <typename Func, typename T> 
class PostfixFunctor
{
  template<typename OtherT, typename OtherFunc>
  friend PostfixFunctor<OtherFunc, OtherT> MakeLazyMember(OtherFunc&&);

private:
  Func op;
  constexpr PostfixFunctor (Func&& f) : op(std::forward<Func>(f)) {}

public:
  template <typename... Ts> 
  constexpr decltype(auto) operator() (Ts&&... ts) { 
    return LazyUnaryFunctor<Func, T, Ts...>(std::forward<Func>(op), std::forward<Ts>(ts)...); 
  }
};

} // Util
} // Anki

#endif