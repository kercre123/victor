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
  template <typename... Ts> struct TypePack  { using type = TypePack; };
  template <size_t... Is>   struct IndexPack { using type = IndexPack; };

  // template <size_t... Is> struct IndexPack : TypePack<decltype(Is)...> {};
  // template <> struct IndexPack<> : TypePack<> {};

  template <size_t I, size_t... Is>
  struct _max : std::integral_constant<size_t, I> {};

  template <size_t I0, size_t I1, size_t... Is>
  struct _max<I0, I1, Is...> : std::integral_constant<size_t, _max<I0 >= I1 ? I0 : I1, Is...>::value> {};


  template <size_t I, typename IPack> struct _prepend;
  template <size_t I, size_t... Is>   struct _prepend<I, IndexPack<Is...>> : IndexPack<I, Is...> {};

  template <size_t I, typename IPack> struct _prepend1;
  template <size_t I, size_t... Is>   struct _prepend1<I, IndexPack<Is...>> : IndexPack<I, 1 + Is...> {};

  template <size_t Idx, typename Sequence>
  struct _get;

  template <size_t Idx, size_t... Is>
  struct _get<0, IndexPack<Idx, Is...>> : std::integral_constant<size_t, Idx> {};

  template <size_t Idx, size_t I, size_t... Is>
  struct _get<Idx, IndexPack<I, Is...>> : _get<Idx-1, IndexPack<Is...>> {};

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template <typename Arg, typename... Ts>
  struct GetPackIdx : 
    std::integral_constant<size_t, 1> {};
   
  template <typename Arg, typename T0, typename... Ts>
  struct GetPackIdx<Arg, T0, Ts...> : 
    std::integral_constant<size_t, std::is_same<Arg, T0>::value ? 0 : 1 + GetPackIdx<Arg, Ts...>::value> {};


  template <size_t I, typename... Ts>
  struct GetPackElement { using type = void; };
   
  template <typename T0, typename... Ts>
  struct GetPackElement<0, T0, Ts...> { using type = T0; };

  template <size_t I, typename T0, typename... Ts>
  struct GetPackElement<I, T0, Ts...> : GetPackElement<I-1, Ts...> {};


  template <typename... Ts>
  struct GetIndices : 
    IndexPack<> {};
   
  template <typename T0, typename... Ts>
  struct GetIndices<T0, Ts...> : 
    _prepend1<0, typename GetIndices<Ts...>::type> {};


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
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // helpers for accessing elements in aligned storage
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


  template <size_t I, typename... Types>
  struct SumType_helper {
    using T = typename GetPackElement<I, Types...>::type;

    inline static void copy(std::size_t i, void* t0, const void* t1) {
      if (i == I) { new (t0) T( *reinterpret_cast<const T*>(t1) ); }
      else        { SumType_helper<I - 1, Types...>::copy(i, t0, t1); }
    }
        
    inline static void move(std::size_t i, void* t0, void* t1) {
      if (i == I) { new (t0) T( std::move(*reinterpret_cast<T*>(t1)) ); }
      else        { SumType_helper<I - 1, Types...>::move(i, t0, t1); } 
	  }

    inline static void destroy(std::size_t i, void* t) {
      if (i == I) { reinterpret_cast<T*>(t)->~T(); }
      else        { SumType_helper<I - 1, Types...>::destroy(i, t); }
    }
    
    template <typename... Funcs>
    inline static decltype(auto) match(size_t idx, void* t, Funcs&&... fs) {
      using FIdx = _get<I, typename MapFuncs<TypePack<Types...>, TypePack<Funcs...>>::type>;
      return (idx == I) ? std::get< FIdx::value >( std::forward_as_tuple(fs...) )(*reinterpret_cast<T*>(t))
                        : SumType_helper<I - 1, Types...>::match(idx, t, std::forward<Funcs>(fs)...);
    }
  };

  template <typename T, typename... Types>
  struct SumType_helper<0, T, Types...> {
    inline static void copy(std::size_t, void* t0, const void* t1) {
      t0 = new T( *reinterpret_cast<const T*>(t1) );
    }

    inline static void destroy(std::size_t, void* t) {
      reinterpret_cast<T*>(t)->~T();
    }
        
    inline static void move(std::size_t i, void* t0, void* t1) {
      new (t0) T( std::move(*reinterpret_cast<T*>(t1)) ); 
	  }

    template <typename... Funcs>
    inline static decltype(auto) match(size_t idx, void* t, Funcs&&... fs) {
      using FIdx = _get<0, typename MapFuncs<TypePack<T, Types...>, TypePack<Funcs...>>::type>;
      return std::get< FIdx::value >( std::forward_as_tuple(fs...) )(*reinterpret_cast<T*>(t));
    }
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

    _idx = TypeIdx;
    new (&_data) InitType( val );
  }

  SumType(const SumType<Types...>& other) {
    this->_idx = other._idx;
    helper_t::copy(_idx, &_data, &other._data);
  }

  ~SumType() {
    helper_t::destroy(_idx, &_data);
  }

  template <typename InitType, 
            int TypeIdx = GetPackIdx<InitType, Types...>::value,
            typename = std::enable_if_t<TypeIdx < sizeof...(Types)> >
  SumType<Types...>& operator=(InitType val) {
    SetTo(val);
    return *this;
  }

  SumType<Types...>& operator=(const SumType<Types...>& other) {
    this->_idx = other._idx;
    // CopyType<sizeof...(Types)-1>::template visit(idx, concrete, other.concrete);
    SumType_helper<sizeof...(Types)-1, Types...>::copy(_idx, &_data, &other._data);
    return *this;
  }

  template <typename InitType, 
            int TypeIdx = GetPackIdx<InitType, Types...>::value,
            typename = std::enable_if_t<TypeIdx < sizeof...(Types)> >
  void SetTo(InitType val) {
    helper_t::destroy(_idx, &_data);

    _idx = TypeIdx;
    new (&_data) InitType( val );
  }

  template <typename... Funcs>
  inline decltype(auto) Match(Funcs&&... fs) {
    static_assert(AcceptsAll<TypePack<Types...>, Funcs...>::value,  "Function list does not match all types of variant"); 
    static_assert(AcceptsOnce<TypePack<Types...>, Funcs...>::value, "Function list contains ambiguous Argument types"); 
    
    return helper_t::match(_idx, &_data, std::forward<Funcs>(fs)...);
  }

private:
  // static constexpr auto indices = GetIndices<Types...>{};


	static const size_t data_size  = _max<sizeof(Types)...>::value;
	static const size_t data_align = _max<alignof(Types)...>::value;
	using data_t = typename std::aligned_storage<data_size, data_align>::type;
  using helper_t = SumType_helper<sizeof...(Types)-1, Types...>;
	data_t _data;
  size_t _idx;
};


} // Util
} // Anki

#endif