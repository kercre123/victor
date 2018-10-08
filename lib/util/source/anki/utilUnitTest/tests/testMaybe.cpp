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
  int extractedVal = result.ValueOr(0);
  
  EXPECT_TRUE(intM.IsJust());
  EXPECT_TRUE(result.IsJust());
  EXPECT_EQ(6, extractedVal) << "calculated " << extractedVal << ", not 6";
  EXPECT_EQ(1, plus2CalledCount) << "called " << plus2CalledCount << " times, should have been called once";

  // test monadic chaining
  auto result2 = result.fmap(plus2)
                       .fmap(plus2)
                       .fmap(plus2);


  extractedVal = result2.ValueOr(0);

  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE(result2.IsJust());
  EXPECT_EQ(12, extractedVal);
  EXPECT_EQ(4, plus2CalledCount);
}

TEST(TestMaybe, JustBind)
{
  using namespace Anki::Util;

  Maybe<int> intM = Just(4);

  auto minus2  = [](int x)   { return x-2; };
  auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  // double inverse should change the type, but keep the value
  auto result = intM.Bind(safeInv)
                    .Bind(safeInv);

  float extractedVal = result.ValueOr(0.);
  
  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE( FLT_NEAR(4.f, extractedVal) ) << "calculated " << extractedVal << ", not 4.";

  auto result2 = result.fmap(minus2).fmap(minus2);
  extractedVal = result2.ValueOr(-5.f);
  EXPECT_TRUE( NEAR_ZERO(extractedVal) ) << "calculated " << extractedVal << ", should be 0.f";

  // alternative syntax test
  auto result3 = result2.Bind(safeInv).Bind(safeInv);
  EXPECT_TRUE( result3.IsNothing() ) << "safeInverse should return Nothing if inverting 0";
}

TEST(TestMaybe, ClassMemberAccess)
{
  using namespace Anki::Util;

  auto intM = Just(4);

  auto minus2  = [](int x)   { return x-2; };
  auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  // double inverse should change the type, but keep the value
  auto result = intM.Bind(safeInv)
                    .Bind(safeInv);

  float extractedVal = result.ValueOr(0.);
  
  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE( FLT_NEAR(4.f, extractedVal) ) << "calculated " << extractedVal << ", not 4.";

  auto result2 = result.fmap(minus2).fmap(minus2);
  extractedVal = result2.ValueOr(-5.f);
  EXPECT_TRUE( NEAR_ZERO(extractedVal) ) << "calculated " << extractedVal << ", should be 0.f";

  // alternative syntax test
  auto result3 = result2.Bind(safeInv).Bind(safeInv);
  EXPECT_TRUE( result3.IsNothing() ) << "safeInverse should return Nothing if inverting 0";
}


template <typename Func>
constexpr bool IsVoidRef() { return std::is_void<typename std::result_of<Func&()>::type>::value; }

TEST(TestMaybe, MaybeMutableClass)
{
  using namespace Anki::Util;

  class IntWrapper {
  public:
    IntWrapper(int x) : _x(x) {}
    void Inc()           { ++_x; }
    void Plus(int y)          { _x+=y; }

  private:
    int _x;
  };

  // auto just4 = Just<IntWrapper>(4);
  // auto result1 = just4.fmap( [&](auto& x) { x.Inc(); } );
  // auto result2 = just4 & [&](auto& x) { return Just(x.Inc()); };

  // auto result3 = just4.fmap( [&](auto& x) { x.Inc(); } );
  // auto result4 = just4.fmapMember( &IntWrapper::Inc );
  // auto result5 = just4.fmapMember( &IntWrapper::Plus );



  // auto resultM = just4 _Bind( Inc() );

}
  // TODO: make this 
  // (func1 >> func2 >> func3).apply(intM); 
  // func1.bind(func2).eval(intM);

  // auto result = function.Do( minus2,
  //                        minus,
  //                        invers);
