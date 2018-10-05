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
//    const Unit retValue = {};
//    return retValue;
struct Unit {};

// helpers for checking return types of arguments
template <typename T> class Maybe;
template <typename T> struct CheckMaybe : public std::false_type {};
template <typename T> struct CheckMaybe< Maybe<T> > : public std::true_type {};
template <typename T> constexpr bool IsMaybe() { return CheckMaybe<T>::value; }

template<typename T>
class Maybe
{
public:
  // Default Constructor, creates `Nothing`
  Maybe() : _data(nullptr), _valid(false) {} 

  // Construct `Just Val` with a move
  Maybe(T val) 
  : _data( std::make_shared<T>( val ) )
  , _valid(true) {} 

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }

  template<typename Func, typename = typename std::enable_if<std::is_void<typename std::result_of<Func&(T&)>::type>::value>::type>
  Maybe<Unit>
  fmap(Func&& f) const {
    if(_valid) { std::forward<Func>(f)(*_data); }
    return (_valid) ? Maybe<Unit>::Just(Unit()) : Maybe<Unit>::Nothing(); 
  }
    
  template<typename Func, typename = typename std::enable_if<!std::is_void<typename std::result_of<Func&(T&)>::type>::value>::type>
  Maybe<typename std::result_of<Func&(T&)>::type>
  fmap(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    return (_valid) ? Maybe<RT>( std::forward<Func>(f)(*_data) ) : Maybe<RT>(); 
  }

  // bind operations for mapping functions that already return a Maybe
  template<typename Func>
  typename std::result_of<Func&(T&)>::type
  bind(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    static_assert(IsMaybe<RT>(), "bind operation must provide a function that returns `Maybe` type");
    return (_valid) ? std::forward<Func>(f)(*_data) : RT::Nothing(); 
  }

  // infix operator for monadic bind, similar to Haskell `>>=`
  template<typename Func>     
  typename std::result_of<Func&(T&)>::type
  operator>>(Func&& f) { return bind(f); }

  // Monadic OR operations. for two type classes with the same type:
  //   Maybe1.IsJust()    Or Maybe2             = Maybe1
  //   Maybe1.IsNothing() Or Maybe2.IsJust()    = Maybe2
  //   Maybe1.IsNothing() Or Maybe2.IsNothing() = Nothing
  // Maybe<T>& operator|=(Maybe<T>& other) {
  //   if (!_valid && other->_valid) {
  //     _data = other._data;
  //     _valid = true;
  //   }
  //   return *this;
  // }

  Maybe<T>& Or(Maybe<T>& other) {
    return (_valid) ? *this : other;
  }

  T extract(T defaultVal) const { return (_valid) ? *_data : defaultVal; }

  static Maybe<T> Just(T t) { return Maybe<T>( t ); }
  static Maybe<T> Nothing()   { return Maybe<T>(); }

protected:
  std::shared_ptr<T>  _data;
  bool _valid;
};

template<typename T>
Maybe<T> Just(T t) { return Maybe<T>(t); }

template<typename T>
Maybe<T> Nothing()   { return Maybe<T>(); }
} // Util
} // Anki

#endif