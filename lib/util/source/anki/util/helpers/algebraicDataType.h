/**
 * File: lazyFunctor.h
 *
 * Author: Michael Willett
 * Created: 10/2/2018
 *
 * Description: 
 * 
 *      typeclass for defining unions of types (similar to variant)
 * 
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __UTIL_ADT_H__
#define __UTIL_ADT_H__

#include <type_traits>


namespace Anki{ 
namespace Util {

namespace {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // helpers for getting type information about Argument Packs
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template <typename... Ts> struct TypePack {};

  template <typename Arg, typename... Ts>
  struct GetPackIdx : 
    std::integral_constant<size_t, 1> {};
   
  template <typename Arg, typename T0, typename... Ts>
  struct GetPackIdx<Arg, T0, Ts...> : 
    std::integral_constant<size_t, std::is_same<Arg, T0>::value ? 0 : 1 + GetPackIdx<Arg, Ts...>::value> {};

  template <typename... Types>
  struct IsUniqueList : 
    std::true_type {};

  template <typename T0, typename... Ts>
  struct IsUniqueList<T0, Ts...> : 
    std::integral_constant<bool, (GetPackIdx<T0, Ts...>::value >= sizeof...(Ts)) && IsUniqueList<Ts...>::value> {};
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // helpers for checking if a set of functions accepts a specific type for an argument
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<typename> struct void_type { using type = void; };
  template<typename> using  void_t = typename void_type<void>::type;

  template<typename Arg, typename Func, typename = void>
  struct Accepts :
    std::false_type {};

  template<typename Arg, typename Func>
  struct Accepts<Arg, Func, void_t< decltype( std::declval<Func>()(std::declval<Arg>()) ) >> : 
    std::true_type {};
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template <typename Arg, typename... Funcs>
  struct NumAccepting : 
    std::integral_constant<size_t, 0> {};

  template <typename Arg, typename Func0, typename... Funcs>
  struct NumAccepting<Arg, Func0, Funcs...> : 
    std::integral_constant<size_t, ((size_t) Accepts<Arg, Func0>::value) + NumAccepting<Arg, Funcs...>::value> {};

  // Check if all types are accepted at least once by the set of functions
  template <typename TPack, typename... Funcs>
  struct AcceptsAll : 
    std::true_type {};

  template <typename T0, typename... Ts, typename... Funcs>
  struct AcceptsAll<TypePack<T0, Ts...>, Funcs...> : 
    std::integral_constant<bool, (NumAccepting<T0, Funcs...>::value > 0) && AcceptsAll<TypePack<Ts...>, Funcs...>::value> {};
    
  // check if all types are accepted at most once by the set of functions
  template <typename TPack, typename... Funcs>
  struct AcceptsOnce : 
    std::true_type {};

  template <typename T0, typename... Ts, typename... Funcs>
  struct AcceptsOnce<TypePack<T0, Ts...>, Funcs...> : 
    std::integral_constant<bool, (NumAccepting<T0, Funcs...>::value < 2) && AcceptsAll<TypePack<Ts...>, Funcs...>::value> {};

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // map a set-of-types to a set-of-functions and apply when given a specified type-index
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  template <size_t... Ints> struct IndexPack { using type = IndexPack; };

  template <size_t I, typename IPack> struct _prepend;
  template <size_t I, size_t... Is>   struct _prepend<I, IndexPack<Is...>> : IndexPack<I, Is...> {};

  template <size_t Idx, typename Sequence>
  struct _get;

  template <size_t I, size_t... Is>
  struct _get<0, IndexPack<I, Is...>> : std::integral_constant<size_t, I> {};

  template <size_t Idx, size_t I, size_t... Is>
  struct _get<Idx, IndexPack<I, Is...>> : _get<Idx-1, IndexPack<Is...>> {};
   

  // --------------------------------------------------------------

  template <typename Arg, typename FPack> 
  struct AcceptIdx : 
    std::integral_constant<int, -1> {};
   
  template <typename Arg, typename F0, typename... Fs>
  struct AcceptIdx<Arg, TypePack<F0, Fs...>> : 
    std::integral_constant<int, Accepts<Arg, F0>::value ? 0 : 1 + AcceptIdx<Arg, TypePack<Fs...>>::value> {};

  // map the types in TPack to the first function in FPack that accept that type
  template <typename TPack, typename FPack> 
  struct MapFuncs : IndexPack<> {};
  
  template <typename T1, typename... Types, typename FPack>
  struct MapFuncs<TypePack<T1, Types...>, FPack> : 
    _prepend< AcceptIdx<T1, FPack>::value, typename MapFuncs<TypePack<Types...>, FPack>::type> {};
  

  template <size_t I>
  struct FindType {
    template <typename... Types, typename... Funcs>
    inline static decltype(auto) visit(size_t idx, std::tuple<Types*...>& tup, Funcs&&... fs) {
      using FIdx = _get<I, typename MapFuncs<TypePack<Types...>, TypePack<Funcs...>>::type>;
      return (idx == I) ? std::get< FIdx::value >(std::tuple<Funcs...>(std::forward<Funcs>(fs)...))(*std::get<I>(tup))
                        : FindType<I - 1>::template visit(idx, tup, std::forward<Funcs>(fs)...);
    }
  };

  template <>
  struct FindType<0> {
    template <typename... Types, typename... Funcs>
    inline static decltype(auto) visit(size_t idx, std::tuple<Types*...>& tup, Funcs&&... fs) { 
      using FIdx = _get<0, typename MapFuncs<TypePack<Types...>, TypePack<Funcs...>>::type>;
      return std::get< FIdx::value >(std::tuple<Funcs...>(std::forward<Funcs>(fs)...))(*std::get<0>(tup));
    }
  };

  template <size_t I>
  struct DeleteType {
    template <typename Tup>
    inline static void visit(size_t idx, Tup& tup) {
      (idx == I) ? delete std::get<I>(tup) : DeleteType<I - 1>::template visit(idx, tup);
    }
  };

  template <>
  struct DeleteType<0> {
    template <typename Tup>
    inline static void visit(size_t idx, Tup& tup) { delete std::get<0>(tup); }
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Pattern Matched SumType
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename... Types>
class SumType {
public:
  template <typename InitType, 
            int TypeIdx = GetPackIdx<InitType, Types...>::value,
            typename = std::enable_if_t<TypeIdx < sizeof...(Types)> >
  SumType(InitType val) {
    static_assert(IsUniqueList<Types...>::value, "Cannot resolve addresses of a SumType with duplicate types.");
    idx = TypeIdx;
    std::get<TypeIdx>( concrete ) = new InitType( val );
  }

  SumType(const SumType<Types...>& other) {
    this->concrete = other.concrete;
    this->idx = other.idx;
  }

  ~SumType() {
    // DeleteType<sizeof...(Types)-1>::template visit(idx, concrete);
    // delete here can create a double free
  }

  template <typename InitType, 
            int TypeIdx = GetPackIdx<InitType, Types...>::value,
            typename = std::enable_if_t<TypeIdx < sizeof...(Types)> >
  SumType<Types...>& operator=(InitType val) {
    SetTo(val);
    return *this;
  }

  SumType<Types...>& operator=(const SumType<Types...>& other) {
    this->concrete = other.concrete;
    this->idx = other.idx;
    return *this;
  }

  template <typename InitType, 
            int TypeIdx = GetPackIdx<InitType, Types...>::value,
            typename = std::enable_if_t<TypeIdx < sizeof...(Types)> >
  void SetTo(InitType val) {
    DeleteType<sizeof...(Types)-1>::template visit(idx, concrete);

    idx = TypeIdx;
    std::get<TypeIdx>( concrete ) = new InitType( val );
  }

  template <typename... Funcs>
  inline decltype(auto) Match(Funcs&&... fs) {
    static_assert(AcceptsAll<TypePack<Types...>, Funcs...>::value,  "Function list does not match all types of variant"); 
    static_assert(AcceptsOnce<TypePack<Types...>, Funcs...>::value, "Function list contains ambiguous Argument types"); 

    return FindType<sizeof...(Types)-1>::template visit(idx, concrete, std::forward<Funcs>(fs)...);
  }

private:
  // TODO: we could make this a union. Otherwise this class could be a wrapper for std::variant
  //       see https://gist.github.com/tibordp/6909880 for variant implementation
  std::tuple<Types*...> concrete;
  size_t idx;
};


} // Util
} // Anki

#endif