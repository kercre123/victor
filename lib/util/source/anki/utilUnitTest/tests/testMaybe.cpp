/**
 * File: testMaybe
 *
 * Author: Michael Willett
 * Created: 08/11/15
 *
 * Description: Unit tests for math.h
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/helpers/maybe.h"
#include "util/math/math.h"


TEST(TestMaybe, NothingFmap)
{
  using namespace Anki::Util;

  Maybe<int> notInt = Nothing<int>();

  int plus2CalledCount = 0;
  auto plus2 = [&plus2CalledCount](int x) -> int { 
    ++plus2CalledCount;
    return x+2; 
  };

  auto result = notInt.fmap(plus2);
  
  EXPECT_TRUE(notInt.IsNothing());
  EXPECT_TRUE(result.IsNothing());
  EXPECT_EQ(0, plus2CalledCount);

  // test monadic chaining
  auto result2 = result.fmap(plus2)
                       .fmap(plus2)
                       .fmap(plus2);

  EXPECT_TRUE(result.IsNothing());
  EXPECT_TRUE(result2.IsNothing());
  EXPECT_EQ(0, plus2CalledCount);
}


TEST(TestMaybe, JustFmap)
{
  using namespace Anki::Util;

  Maybe<int> intM = Just(4);

  int plus2CalledCount = 0;
  auto plus2 = [&plus2CalledCount](int x) -> int { 
    ++plus2CalledCount;
    return x+2; 
  };

  auto result = intM.fmap(plus2);
  int extractedVal = result.extract(0);
  
  EXPECT_TRUE(intM.IsJust());
  EXPECT_TRUE(result.IsJust());
  EXPECT_EQ(6, extractedVal) << "calculated " << extractedVal << ", not 6";
  EXPECT_EQ(1, plus2CalledCount) << "called " << plus2CalledCount << " times, should have been called once";

  // test monadic chaining
  auto result2 = result.fmap(plus2)
                       .fmap(plus2)
                       .fmap(plus2);


  extractedVal = result2.extract(0);

  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE(result2.IsJust());
  EXPECT_EQ(12, extractedVal);
  EXPECT_EQ(4, plus2CalledCount);
}

TEST(TestMaybe, JustBind)
{
  using namespace Anki::Util;

  Maybe<int> intM = Just(4);
  Maybe<int> intM2(4); // this returns Just 4
  Maybe<int> intNot;   // this initializes to Nothing

  auto minus2  = [](int x)   { return x-2; };
  auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  // double inverse should change the type, but keep the value
  auto result = intM.bind(safeInv)
                    .bind(safeInv);

  float extractedVal = result.extract(0.);
  
  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE( FLT_NEAR(4.f, extractedVal) ) << "calculated " << extractedVal << ", not 4.";

  auto result2 = result.fmap(minus2).fmap(minus2);
  extractedVal = result2.extract(-5.f);
  EXPECT_TRUE( NEAR_ZERO(extractedVal) ) << "calculated " << extractedVal << ", should be 0.f";

  // alternative syntax test
  auto result3 = result2 >> safeInv >> safeInv;
  EXPECT_TRUE( result3.IsNothing() ) << "safeInverse should return Nothing if inverting 0";
}

TEST(TestMaybe, ClassMemberAccess)
{
  using namespace Anki::Util;

  Maybe<int> intM = Just(4);
  Maybe<int> intM2(4); // this returns Just 4
  Maybe<int> intNot;   // this initializes to Nothing

  auto minus2  = [](int x)   { return x-2; };
  auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  // double inverse should change the type, but keep the value
  auto result = intM.bind(safeInv)
                    .bind(safeInv);

  float extractedVal = result.extract(0.);
  
  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE( FLT_NEAR(4.f, extractedVal) ) << "calculated " << extractedVal << ", not 4.";

  auto result2 = result.fmap(minus2).fmap(minus2);
  extractedVal = result2.extract(-5.f);
  EXPECT_TRUE( NEAR_ZERO(extractedVal) ) << "calculated " << extractedVal << ", should be 0.f";

  // alternative syntax test
  auto result3 = result2 >> safeInv >> safeInv;
  EXPECT_TRUE( result3.IsNothing() ) << "safeInverse should return Nothing if inverting 0";
}

TEST(TestMaybe, DoNotation)
{
  using namespace Anki::Util;

  // this knows context
  // Maybe<int> int4 = Just(4);

  // // these do not
  // auto minus2  = Just( [](int x)   { return x-2; } );   //
  // auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  
  // auto result = function.Do( minus2,
  //                        minus2,
  //                        inverse,
  //                        minus,
  //                        invers);

  class IntWrapper {
  public:
    IntWrapper(int x) : _x(x) {}
    Unit Inc()                { ++_x; return Unit(); }
    void Plus(int y)          { _x+=y; }

  private:
    int _x;
  };
  
  // Maybe<IntWrapper> just4(4);
  // auto result = just4.bind(Inc());
}
  // TODO: make this 
  // (func1 >> func2 >> func3).apply(intM); 
  // func1.bind(func2).eval(intM);
