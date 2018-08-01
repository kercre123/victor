/**
 * File: testStronglyTyped.cpp
 *
 * Author: ross
 * Created: Jul 31 2018
 *
 * Description: Unit tests for StronglyTyped
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=TestStronglyTyped.*
 **/


#include "util/helpers/includeGTest.h"
#include "util/helpers/stronglyTyped.h"
#include <type_traits>

using namespace Anki::Util;

namespace
{
  struct NoEq {};
  template<typename T, typename Arg> NoEq operator== (const T&, const Arg&);
  
  struct NoPlus {};
  template<typename T, typename Arg> NoPlus operator+ (const T&, const Arg&);
  
  template<typename T, typename Arg>
  struct EqualExists
  {
    enum { value = !std::is_same<decltype(*(T*)(0) == *(Arg*)(0)), NoEq>::value };
  };
  
  template<typename T, typename Arg>
  struct PlusExists
  {
    enum { value = !std::is_same<decltype(*(T*)(0) + *(Arg*)(0)), NoPlus>::value };
  };
}

TEST(TestStronglyTyped, CheckStronglyTyped)
{
  using Underlying = uint8_t;
  using TypeA = StronglyTyped<Underlying, struct TypeA_ID>;
  using TypeB = StronglyTyped<Underlying, struct TypeB_ID>;
  
  static_assert( !std::is_convertible<TypeA, TypeB>::value, "Should not be able to convert between the two" );
  static_assert( !std::is_convertible<TypeB, TypeA>::value, "Should not be able to convert between the two" );
  static_assert( std::is_convertible<Underlying, TypeA>::value, "Should be able to convert into strongly typed" );
  static_assert( std::is_convertible<Underlying, TypeB>::value, "Should be able to convert into strongly typed" );
  static_assert( !std::is_convertible<TypeA, Underlying>::value, "Should be not able to convert out of strongly typed" );
  static_assert( !std::is_convertible<TypeB, Underlying>::value, "Should be not able to convert out of strongly typed" );
  
  Underlying pod1 = 42;
  Underlying pod2 = 255;
  TypeA strong1 = TypeA(pod1);
  TypeB strong2 = TypeB(pod2);
  
  auto sum1a = pod1 + strong1;
  auto sum1b = strong1 + pod1;
  
  auto sum2a = pod2 + strong2;
  auto sum2b = strong2 + pod2;
  
  static_assert( std::is_same<decltype(pod1),Underlying>::value, "");
  static_assert( std::is_same<decltype(pod2),Underlying>::value, "");
  static_assert( std::is_same<decltype(sum1a),TypeA>::value, "");
  static_assert( std::is_same<decltype(sum1b),TypeA>::value, "");
  static_assert( std::is_same<decltype(sum2a),TypeB>::value, "");
  static_assert( std::is_same<decltype(sum2b),TypeB>::value, "");
  
  static_assert( EqualExists<TypeA, Underlying>::value, "" );
  static_assert( EqualExists<TypeB, Underlying>::value, "" );
  static_assert( EqualExists<Underlying,TypeA>::value, "" );
  static_assert( EqualExists<Underlying, TypeB>::value, "" );
  static_assert( !EqualExists<TypeB,TypeA>::value, "" );
  static_assert( !EqualExists<TypeA,TypeB>::value, "" );
  
  static_assert( PlusExists<TypeA, Underlying>::value, "" );
  static_assert( PlusExists<TypeB, Underlying>::value, "" );
  static_assert( PlusExists<Underlying,TypeA>::value, "" );
  static_assert( PlusExists<Underlying, TypeB>::value, "" );
  static_assert( !PlusExists<TypeB,TypeA>::value, "" );
  static_assert( !PlusExists<TypeA,TypeB>::value, "" );
}

