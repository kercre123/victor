/**
 * File: maybe.h
 *
 * Author: Michael Willett
 * Created: 10/2/2018
 *
 * Description: Implementation of Maybe Monad 
 *              for a good tutorial on monadic types: 
 *                  http://adit.io/posts/2013-04-17-functors,_applicatives,_and_monads_in_pictures.html
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __UTIL_MAYBE_H__
#define __UTIL_MAYBE_H__

#include <type_traits>

namespace Anki{ 
namespace Util {

// Unit type is similar to delcaring a void object, except that it plays
// nicely with type deduction logic in return types.
// With unit, the following behaves the same as void, but will compile:
//    const Unit retValue;
//    return retValue;
// struct Unit {};

// helper for checking return types of arguments
template <typename T> class  Maybe;
template <typename T> struct MaybeT             : public std::false_type {};
template <typename T> struct MaybeT< Maybe<T> > : public std::true_type {};
template <typename T> constexpr bool IsMaybe() { return MaybeT<T>::value; }

// convenience constructor forwards
template <typename T> Maybe<T> Nothing()                    { return Maybe<T>(); }
template <typename T> Maybe<T> Just(T t)                    { return Maybe<T>( t ); }

template <typename Func, typename... Args>
constexpr bool IsVoid() { return std::is_void<std::result_of_t<Func&(Args&...)>>::value; }


template<typename T>
class Maybe
{
  template <typename U> friend class Maybe;           // allow defering binding of function parameter to result type
  template <typename U> friend Maybe<U> Nothing();    // allow non-class-member constructor helpers
  template <typename U> friend Maybe<U> Just(U u);    // allow non-class-member constructor helpers

public:
  // static constructor helpers
  static Maybe<T> Nothing()                    { return Maybe<T>(); }
  static Maybe<T> Just(T val)                  { return Maybe<T>( val ); }

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }
    
  template <typename Func, typename RT = Maybe< typename std::result_of_t<Func&(T&)> >>
  RT fmap(Func&& f) const {
    return (_valid) ? RT::Just( f, *_data ) : RT::Nothing(); 
  }

  // bind operations for mapping functions that already return a Maybe
  template<typename Func, 
           typename RT = typename std::result_of<Func&(T&)>::type,
           typename    = typename std::enable_if<IsMaybe<RT>()>>
  RT Bind(Func&& f) const {
    return (_valid) ? std::forward<Func>(f)(*_data) : RT::Nothing(); 
  }

  // 
  Maybe<T> ThisOr(Maybe<T>& other) const { return (_valid) ? *this : other; }
  T        ValueOr(T defaultVal)   const { return (_valid) ? *_data : defaultVal; }

protected:
  // keep constructors private so construction must happen through `Just` or `Nothing`
  Maybe()      : _data(nullptr), _valid(false) {} 
  Maybe(T val) : _data( std::make_shared<T>(val) ), _valid(true) {} 

  // special constructor for forwarding function evaluation to result type construction 
  template <typename Func, typename U> 
  static Maybe<T> Just(Func&& f, U& d) { return Maybe<T>( std::forward<Func>(f)(d) ); }

  // keep data under a shared_ptr so we can quickly copy this class
  std::shared_ptr<T> _data;
  bool               _valid;
};

// specialization for handling void return types
template<>
class Maybe<void> {
  template <typename U> friend class Maybe;

public:
  template <typename Func, typename RT = Maybe< typename std::result_of_t<Func&()> >>
  RT fmap(Func&& f) const {
    return (_valid) ? RT::Just( std::forward<Func>(f)() ) : RT::Nothing(); 
  }

  template<typename Func, 
           typename RT = typename std::result_of_t<Func&()>,
           typename    = typename std::enable_if<IsMaybe<RT>()>>
  RT Bind(Func&& f) const {
    return (_valid) ? std::forward<Func>(f)() : RT::Nothing(); 
  }

  static Maybe<void> Just()                  { return Maybe<void>(true); }
  static Maybe<void> Nothing()               { return Maybe<void>(false); }

protected:

  template <typename Func, typename U, typename = typename std::enable_if_t< IsVoid<Func, U>()> > 
  static Maybe<void> Just(Func&& f, U& d) { 
    std::forward<Func>(f)(d); 
    return Maybe<void>(true); 
  }

  Maybe(bool isJust) : _valid(isJust) {} 
  bool               _valid;

};

} // Util
} // Anki

#endif