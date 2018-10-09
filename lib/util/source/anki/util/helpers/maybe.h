/**
 * File: maybe.h
 *
 * Author: Michael Willett
 * Created: 10/2/2018
 *
 * Description: 
 * 
 *      Implementation of c++ Maybe Monad
 *      For a good tutorial on monadic types: 
 *        http://adit.io/posts/2013-04-17-functors,_applicatives,_and_monads_in_pictures.html
 * 
 * 
 *    In general, this class can be used to encapsulate type safe calls to an object that
 *    may or may not exist. Common C++ patterns often use pointers that must be checked
 *    for null explicitly before derefernce, or otherwise require that the result to be
 *    passed by mutable reference through the argument list, and the function returns bool.
 *    Often times these patterns can degrade overtime as code is refactored, where the user
 *    forgets to check the return type of a function, and falsely assume the resulting type
 *    is valid. 
 * 
 *    While `std::optional` introduced in C++17 aims to provide a type signature
 *    to help the developer with the 
 *    
 *    The Maybe typeclass encapsulates this behavior entirely within its own internal state
 *    releiving the developer of the responsibility of checking the success of each step
 *    until explicit derefernce performed by `ValueOr`.
 * 
 *    Operations on the Maybe type can be performed by the FMap and Bind operations.
 *    FMap should be used in general applications of functions of the wrapped types, and 
 *    Bind should be used when the result of the function is also a Maybe type
 * 
 *    IMPLEMENTATION NOTES:
 *      -- input functions to FMap and Bind are not defined as std::function types for two
 *         reasons. First, C++ lambda statements do not implicitly convert to std::function,
 *         expression type, and limit the useability of `auto` specifier in this context.
 *         Second, std::function adds additional runtime overhead when wrapping certain types.
 * 
 *    TODO:
 *      -- Current implementation stores wrapped data inside std::shared_ptr, though it should
 *         be possible to store this as a std::unique_ptr through more intiligent copy 
 *         construtors
 * 
 *      -- currently only supports FMap and Bind operations with a single input argument T.
 *         `std::forward<Func>(f)(*_data)` calls can be replaced with a deferred bind_first
 *         operation once a working implementation of argument currying can be mind to work
 *         with arbitrary function types, allowing for lazy evaluation upon dereference. 
 * 
 *      -- Could statically speedup `if(_valid)` checks by making distinct Just<T> and Nothing<T>
 *         classes, though the lack of Algebraic Data Types in C++ makes it difficult for
 *         a typename to map to two types concurrently
 *
 * 
 * Copyright: Anki, Inc. 2018
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
  template <typename U> friend class Maybe;  // allow deferred binding of function parameter to result type

public:
  using Type = T;

  // static constructor helpers
  template<typename U>
  static Maybe<T> Just(U val)                  { return Maybe<T>( val ); }
  static Maybe<T> Nothing()                    { return Maybe<T>(); }

  // test the state of Maybe object
  bool IsJust()    const { return  _valid; }
  bool IsNothing() const { return !_valid; }

    
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
  using Type = void;

  // static constructor helpers
  static Maybe<void> Just()    { return Maybe<void>(true); }
  static Maybe<void> Nothing() { return Maybe<void>(false); }

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

  bool _valid;
};

} // Util
} // Anki

#endif