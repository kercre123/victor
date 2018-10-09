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

template <typename T> class Maybe;

// convenience operator forwards
template <typename T> Maybe<T> Nothing() { return Maybe<T>::Nothing(); }
template <typename T> Maybe<T> Just(T t) { return Maybe<T>::Just( t ); }
template <typename T> Maybe<T> Just()    { return Maybe<T>::Just();  }


// convenience prefix operation forwards
template <typename Func, typename U, typename T = typename std::result_of_t<Func&(U&)> >
Maybe<T> FMap(Func f, Maybe<U> u) { return u.FMap(f); }

template <typename Func, typename U, typename RT = typename std::result_of_t<Func&(U&)> >
RT Bind(Func f, Maybe<U> u) { return u.Bind(f); }

// convenience infix operation forwards. 
// NOTE: requires explicit `using namespace Anki::Util::MaybeOperators` for devleoper to use
//       to prevent accidential operator overloading
namespace MaybeOperators {
  template <typename Func, typename U, typename T = typename std::result_of_t<Func&(U&)> >
  Maybe<T> operator *=(Func f, Maybe<U> u) { return u.FMap(f); }

  template <typename Func, typename U, typename RT = typename std::result_of_t<Func&(U&)> >
  RT operator >>=(Func f, Maybe<U> u) { return u.Bind(f); }
  
}

namespace TypeOperators {
  // check if object is of Maybe typeclass
template <typename T> struct MaybeT             : public std::false_type {};
template <typename T> struct MaybeT< Maybe<T> > : public std::true_type {};
template <typename T> constexpr bool IsMaybe() { return MaybeT<T>::value; }

  // checking if a function is void type
  template <typename Func, typename... Args>
  constexpr bool IsVoidResult() { return std::is_void<std::result_of_t<Func&(Args&...)>>::value; }

  // function handle for a static cast between two types
  template <typename T, typename U>
  constexpr T StaticCast(U u) { return static_cast<U>(u); }

  template <typename Func>
  using DerefResult = std::remove_reference_t< std::result_of_t<Func> >;
} 

template<typename T>
class Maybe {
  template <typename U> friend class Maybe;           // allow deferred binding of function parameter to result type

public:
  // static constructor helpers
  template<typename U>
  static Maybe<T> Just(U val)                  { return Maybe<T>( val ); }
  static Maybe<T> Nothing()                    { return Maybe<T>(); }

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }
    
  using Type = T;
    
  template <typename Func, typename RT = Maybe< TypeOperators::DerefResult<Func&(T&)> >>
  RT FMap(Func&& f) const {
    return (_valid) ? RT::Just( f, *_data ) : RT::Nothing(); 
  }

  // bind operation for mapping functions that already return a Maybe
  template<typename Func, 
           typename RT = std::result_of_t<Func&(T&)>,
           typename    = std::enable_if_t<TypeOperators::IsMaybe<RT>()>>
  RT Bind(Func&& f) const {
    return (_valid) ? std::forward<Func>(f)(*_data) : RT::Nothing(); 
  }

  // throw a specific error if we call bind with a function that does not return `Maybe<T>`
  template<typename Func, bool IsMaybe = TypeOperators::IsMaybe<std::result_of_t<Func&(T&)>>(),
           typename = typename std::enable_if_t<!IsMaybe>>
  void Bind(Func&& f) const {
    static_assert(IsMaybe, "Attempted to Bind a function that does not return a Maybe type. Did you mean FMap (infix `*=`)?"); 
  }

  template <typename U> T        ValueOr(U defaultVal)  const { return (_valid) ? *_data : defaultVal; }
  template <typename U> Maybe<T> ThisOr(Maybe<U> other) const { return (_valid) ? *this : other.FMap( &TypeOperators::StaticCast<T,U> ); }

protected:
  // keep constructors private so construction must happen through `Just` or `Nothing`
  template <typename U>
  Maybe(U val) : _data( std::make_shared<T>(val) ), _valid(true) {} 
  Maybe()      : _data(nullptr), _valid(false) {} 

  // special constructor for forwarding function evaluation to result type construction 
  template <typename Func, typename U> 
  static Maybe<T> Just(Func&& f, U& d) { return Maybe<T>( std::forward<Func>(f)(d) ); }

  // keep data under a shared_ptr so we can quickly copy this class
  std::shared_ptr<T> _data;
  bool               _valid;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// specialization for handling encaspulation of void type
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
class Maybe<void> {
  template <typename U> friend class Maybe;           // allow defering binding of function parameter to result type

public:
  // static constructor helpers
  static Maybe<void> Just()    { return Maybe<void>(true); }
  static Maybe<void> Nothing() { return Maybe<void>(false); }

  using Type = void;

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }

  template <typename Func, typename RT = Maybe< TypeOperators::DerefResult<Func&()> >>
  RT FMap(Func&& f) const {
    return (_valid) ? RT::Just( std::forward<Func>(f)() ) : RT::Nothing(); 
  }

  template<typename Func, 
           typename RT = std::result_of_t<Func&()>,
           typename    = std::enable_if_t< TypeOperators::IsMaybe<RT>() >>
  RT Bind(Func&& f) const {
    return (_valid) ? std::forward<Func>(f)() : RT::Nothing(); 
  }

  // NOTE: Cannot implement ValueOr since void cannot be a return type
  // NOTE: ThisOr can only be used to bind other void types since no implicit cast to void is well-formed
  Maybe<void> ThisOr(Maybe<void> other) const { return (_valid) ? *this : other; }

protected:

  Maybe(bool isJust = false) : _valid(isJust) {} 

  // deferred constructor
  template <typename Func, typename U, typename = typename std::enable_if_t< TypeOperators::IsVoidResult<Func, U>()> > 
  static Maybe<void> Just(Func&& f, U& d) { 
    std::forward<Func>(f)(d); 
    return Maybe<void>(true); 
  }

  bool               _valid;
};

} // Util
} // Anki

#endif