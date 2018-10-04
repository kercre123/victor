/**
 * File: monads.h
 *
 * Author: Michael Willett
 * Created: 10/2/2018
 *
 * Description: Implementation of high-kinded Monad class, common helper functions, as well as
 *              common monad implementations
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __UTIL_MONADS_H__
#define __UTIL_MONADS_H__

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

#if 1
template<typename T>
class Maybe
{
public:

  // Default Constructor, creates `Nothing`
  Maybe() : _data(nullptr), _valid(false) {} 

  // Construct `Just Val` with a move
  Maybe(T&& val) 
  : _data( std::make_shared<T>( std::move(val) ) )
  , _valid(true) {} 

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }
      
  template<typename Func, typename std::enable_if<!std::is_void<typename std::result_of<Func&(T&)>::type>::value>::type = 0>
  Maybe<typename std::result_of<Func&(T&)>::type>
  fmap(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    return (_valid) ? Maybe<RT>( std::forward<Func>(f)(*_data) ) : Maybe<RT>(); 
  }

  template<typename Func, typename RT = Maybe<Unit> >
  RT
  fmap(Func&& f) const {
    using FT = decltype(f(*((T*)nullptr)));
    static_assert(std::is_void<FT>::value, "fmap operation must provide a function of `void` type");
    if(_valid) { std::forward<Func>(f)(*_data); }
    return (_valid) ? RT::Just(Unit()) : RT::Nothing(); 
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
  Maybe<T>& operator|=(Maybe<T>& other) {
    if (!_valid && other->_valid) {
      _data = other._data;
      _valid = true;
    }
    return *this;
  }

  Maybe<T>& Or(Maybe<T>& other) {
    return (_valid) ? *this : other;
  }

  static Maybe<T> Just(T&& t) { return Maybe<T>(std::move(t)); }
  static Maybe<T> Nothing()   { return Maybe<T>(); }

protected:
  std::shared_ptr<T>  _data;
  bool _valid;
};

#else

// CRPT so we can statically remove all if checks!
template<typename Impl>
class Maybe
{
public:

  // test the state of Maybe object
  constexpr bool IsJust()    const { return static_cast<Impl const&>(*this).IsJust(); }
  constexpr bool IsNothing() const { return static_cast<Impl const&>(*this).IsNothing(); }

  // monadic operations for calling a function of the wrapped type
  template<typename Func>
  decltype(auto) fmap(Func&& f) const { return static_cast<Impl const&>(*this).fmap(f); }

  template<typename Func>
  decltype(auto) bind(Func&& f) const { return static_cast<Impl const&>(*this).bind(f); }

  template<typename Func>
  decltype(auto) operator>>(Func&& f) const { return static_cast<Impl const&>(*this) >> (f); }

  template<typename T>
  Maybe<T>& Or(Maybe<T>& other) { return static_cast<Impl const&>(*this).Or(f); }

protected:
  // use private constructor and friend class to prevent accidentally templating on wrong derived class
  Maybe(){};
  friend Impl;
};

template<typename T>
class Just : public Maybe<Just<T>>//, public T
{
public:

  // Construct `Just Val` with a move
  Just(T&& val) 
  : _data( std::make_shared<T>( std::move(val) ) )

  // test the state of Maybe object
  constexpr bool IsJust()    const { return true; }
  constexpr bool IsNothing() const { return false; }
      
  template<typename Func, typename std::enable_if<!std::is_void<typename std::result_of<Func&(T&)>::type>::value>::type = 0>
  Maybe<typename std::result_of<Func&(T&)>::type>
  fmap(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    return Maybe<RT>( std::forward<Func>(f)(*_data) ); 
  }

  template<typename Func, typename RT = Maybe<Unit> >
  RT
  fmap(Func&& f) const {
    using FT = decltype(f(*((T*)nullptr)));
    static_assert(std::is_void<FT>::value, "fmap operation must provide a function of `void` type");
    if(_valid) { std::forward<Func>(f)(*_data); }
    return RT::Just(Unit()); 
  }

  // bind operations for mapping functions that already return a Maybe
  template<typename Func>
  typename std::result_of<Func&(T&)>::type
  bind(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    static_assert(IsMaybe<RT>(), "bind operation must provide a function that returns `Maybe` type");
    return std::forward<Func>(f)(*_data); 
  }

  // infix operator for monadic bind, similar to Haskell `>>=`
  // NOTE: I chose not to overload `operator>>=` specifically for two reasons:
  //       1) C++ semantics a trailing `=` character implies self modification
  //       2) `>>=` reads strictly right->left, while `>>` reads left->right
  template<typename Func>     
  typename std::result_of<Func&(T&)>::type
  operator>>(Func&& f) { return bind(f); }

  // Monadic OR operations. for two type classes with the same type:
  //   Maybe1.IsJust()    Or Maybe2             = Maybe1
  //   Maybe1.IsNothing() Or Maybe2.IsJust()    = Maybe2
  //   Maybe1.IsNothing() Or Maybe2.IsNothing() = Nothing
  Maybe<T>& Or(Maybe<T>& other) {
    return *this;
  }

protected:
  std::shared_ptr<T>  _data;
};

template<typename T>
class Nothing : public Maybe<Nothing<T>>
{
public:

  // Construct `Just Val` with a move
  Nothing() {}

  // test the state of Maybe object
  constexpr bool IsJust()    const { return false; }
  constexpr bool IsNothing() const { return true; }
      
  template<typename Func, typename std::enable_if<!std::is_void<typename std::result_of<Func&(T&)>::type>::value>::type = 0>
  Maybe<typename std::result_of<Func&(T&)>::type>
  fmap(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    return Nothing<RT>(); 
  }

  template<typename Func, typename RT = Maybe<Unit> >
  RT
  fmap(Func&& f) const {
    using FT = decltype(f(*((T*)nullptr)));
    static_assert(std::is_void<FT>::value, "fmap operation must provide a function of `void` type");
    if(_valid) { std::forward<Func>(f)(*_data); }
    return RT::Nothing(); 
  }

  // bind operations for mapping functions that already return a Maybe
  template<typename Func>
  typename std::result_of<Func&(T&)>::type
  bind(Func&& f) const {
    using RT = decltype(f(*((T*)nullptr)));
    static_assert(IsMaybe<RT>(), "bind operation must provide a function that returns `Maybe` type");
    return RT::Nothing(); 
  }

  // infix operator for monadic bind, similar to Haskell `>>=`
  template<typename Func>     
  typename std::result_of<Func&(T&)>::type
  operator>>(Func&& f) { return bind(f); }

  // Monadic OR operations. for two type classes with the same type:
  //   Maybe1.IsJust()    Or Maybe2             = Maybe1
  //   Maybe1.IsNothing() Or Maybe2.IsJust()    = Maybe2
  //   Maybe1.IsNothing() Or Maybe2.IsNothing() = Nothing
  // Maybe<T>& operator|=(Maybe<T>& other) { *this = other; return *this; }
  Maybe<T>& Or(Maybe<T>& other)         { return other; }
};


// helper for declaring that something can be a Maybe type, without specifying
// if it is Just<T> or Nothing<T>
template<typename T>
class MaybeT : Just<T>, Nothing<T> {};
#endif

} // Util
} // Anki

#endif