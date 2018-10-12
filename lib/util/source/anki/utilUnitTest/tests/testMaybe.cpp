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
#include "util/helpers/noncopyable.h"
#include "util/helpers/maybe.h"
#include "util/math/math.h"


TEST(TestMaybe, NothingFMap)
{
  using namespace Anki::Util;

  Maybe<int> notInt = Nothing<int>();

  int plus2CalledCount = 0;
  auto plus2 = [&plus2CalledCount](int x) -> int { 
    ++plus2CalledCount;
    return x+2; 
  };

  auto result = notInt.FMap(plus2);
  
  EXPECT_TRUE(notInt.IsNothing());
  EXPECT_TRUE(result.IsNothing());
  EXPECT_EQ(0, plus2CalledCount);

  // test monadic chaining
  auto result2 = result.FMap(plus2)
                       .FMap(plus2)
                       .FMap(plus2);

  EXPECT_TRUE(result.IsNothing());
  EXPECT_TRUE(result2.IsNothing());
  EXPECT_EQ(0, plus2CalledCount);

  // test infix format
  using namespace Anki::Util::MaybeOperators;
  
  auto result3 = plus2 *= plus2 *= result;
  auto result4 = FMap( plus2, FMap(plus2, Nothing<int>()) );

  EXPECT_TRUE(result.IsNothing());
  EXPECT_TRUE(result3.IsNothing());
  EXPECT_TRUE(result4.IsNothing());
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

  auto result = intM.FMap(plus2);
  int extractedVal = result.ValueOr(0);
  
  EXPECT_TRUE(intM.IsJust());
  EXPECT_TRUE(result.IsJust());
  EXPECT_EQ(6, extractedVal) << "calculated " << extractedVal << ", not 6";
  EXPECT_EQ(1, plus2CalledCount) << "called " << plus2CalledCount << " times, should have been called once";

  // test monadic chaining
  auto result2 = result.FMap(plus2)
                       .FMap(plus2)
                       .FMap(plus2);


  extractedVal = result2.ValueOr(0);

  EXPECT_TRUE(result.IsJust());
  EXPECT_TRUE(result2.IsJust());
  EXPECT_EQ(12, extractedVal);
  EXPECT_EQ(4, plus2CalledCount);

  // test infix format
  using namespace Anki::Util::MaybeOperators;

  auto result3 = plus2 *= plus2 *= result;
  auto result4 = FMap( plus2, FMap(plus2, Just(1)) );

  EXPECT_TRUE(result3.IsJust());
  EXPECT_TRUE(result4.IsJust());
  EXPECT_EQ(10, result3.ValueOr(-1)) << "calculated " << result3.ValueOr(-1) << ", not 10";
  EXPECT_EQ( 5, result4.ValueOr(-1)) << "calculated " << result4.ValueOr(-1) << ", not 10";
}

TEST(TestMaybe, NothingBind)
{
  using namespace Anki::Util;

  Maybe<int> intM = Nothing<int>();

  auto safeInv = [](float divisor) { return ( NEAR_ZERO(divisor) ) ? Nothing<float>() : Just(1/divisor); };

  // double inverse should change the type, but keep the value
  auto result = intM.Bind(safeInv)
                    .Bind(safeInv);

  float extractedVal = result.ValueOr(-1.2);
  
  EXPECT_TRUE(result.IsNothing());
  EXPECT_TRUE( FLT_NEAR(-1.2f, extractedVal) ) << "calculated " << extractedVal << ", not -1.2";

  // test infix format
  using namespace Anki::Util::MaybeOperators;

  auto result2 = safeInv >>= safeInv >>= intM;
  auto result3 = Bind( safeInv, Bind(safeInv, intM) );

  EXPECT_TRUE(result2.IsNothing());
  EXPECT_TRUE(result3.IsNothing());
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

  auto result2 = result.FMap(minus2).FMap(minus2);
  extractedVal = result2.ValueOr(-5.f);
  EXPECT_TRUE( NEAR_ZERO(extractedVal) ) << "calculated " << extractedVal << ", should be 0.f";

  // alternative syntax test
  auto result3 = result2.Bind(safeInv).Bind(safeInv);
  EXPECT_TRUE( result3.IsNothing() ) << "safeInverse should return Nothing if inverting 0";


  // test infix format
  using namespace Anki::Util::MaybeOperators;

  auto result4 = safeInv >>= safeInv >>= Just(4);
  auto result5 = Bind( safeInv, Bind(safeInv, Just(4)) );

  EXPECT_TRUE(result4.IsJust());
  EXPECT_TRUE(result5.IsJust());
  EXPECT_TRUE( FLT_NEAR(4.f, result4.ValueOr(-1)) ) << "calculated " << result4.ValueOr(-1) << ", not 4.";
  EXPECT_TRUE( FLT_NEAR(4.f, result5.ValueOr(-1)) ) << "calculated " << result5.ValueOr(-1) << ", not 4.";

  auto result6 = safeInv >>= safeInv >>= Just(0);
  EXPECT_TRUE( result6.IsNothing() ) << "safeInverse should return Nothing if inverting 0";
}

TEST(TestMaybe, MutableClassMember)
{
  using namespace Anki::Util;

  class NumWrapper {
  public:
    NumWrapper(float x) : _x(x) {}

    void decr() { --_x; }

    Maybe<void> invert() 
    { 
      if ( NEAR_ZERO(_x) ) {
        return Nothing<void>(); 
      } else { 
        _x = 1/_x; 
        return Just<void>();
      }
    }

    float get() const { return _x; }

  private:
    float _x;
  };

  auto decrSelf   = [](NumWrapper& x) { x.decr(); };
  auto invertSelf = [](NumWrapper& x) { return x.invert(); };

  auto wrapM = Just<NumWrapper>(2.f);

  wrapM.FMap(decrSelf);
  wrapM.FMap(decrSelf);
  auto failHere = wrapM.Bind(invertSelf);
  wrapM.FMap(decrSelf);
  auto succeedHere = wrapM.Bind(invertSelf);
  auto extract = wrapM.FMap( [](auto w) { return w.get(); } );

  EXPECT_TRUE(failHere.IsNothing());
  EXPECT_TRUE(succeedHere.IsJust());
  EXPECT_TRUE( FLT_NEAR(-1.f, extract.ValueOr(0.f)) ) << "should have inverted -1";
}

namespace {
  template<typename T>
  T SubtractAbs(T x, T y) { return fabs(x - y); }

  template<typename T>
  T Double(T x) { return x*2; }
    
  struct IntWrapper : private Anki::Util::noncopyable {
    IntWrapper(int a) : x(a) {}
    int x;
    int& operator++() { return ++x; }
    static int inc(int y) { return ++y; }
    int decr() { return --x; }
    int sub(int y) { return x-y; }
  };

}

TEST(TestMaybe, FMapFunctionTypes)
{
  using namespace Anki::Util;

  // these tests are just to ensure these calls are compiled without SFINAE errors.
  // Other tests check proper execution

  // std::function
  float z = 2;
  std::function<void(double)> a = [=](double x) { return z + x; };
  Just(5).FMap(a);

  // std::bind
  auto b = std::bind(&SubtractAbs<int>, std::placeholders::_1, 5);
  Just(2).FMap(b);

  // lambda
  auto c = [](float x) { return x / 2.f ; };
  Just(5).FMap(c);

  // anonymous
  auto d = &Double<int>;
  Just(2).FMap(d);

  // static member function reference
  auto e = &IntWrapper::inc;
  Just(2).FMap(e);

  // member function of existing type
  // MAKE_FUNCTOR(IntWrapper, decr);
  // auto f = std::bind(&IntWrapper::operator++, std::placeholders::_1);

  // Just<IntWrapper>(2).FMap(f);
  // Just<IntWrapper>(2).FMap(decr);

  // IntWrapper(2).decr();
  // decr( IntWrapper(2) );
  // auto safeObj = Just<IntWrapper>(8);

  // using namespace Anki::Util::MaybeOperators;
  // safeObj->*decr;
}

namespace Helper
{
  template <int... Is>
  struct index {};

  template <int N, int... Is>
  struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

  template <int... Is>
  struct gen_seq<0, Is...> : index<Is...> {};

  template <typename Func, typename... Args, int... Is>
  decltype(auto) ForwardTupleArgs(Func&& f, std::tuple<Args...>&& tup, index<Is...>)
  {
    return std::forward<Func>(f)(std::get<Is>(tup)...);
  }

  template <typename Func, typename... Args>
  decltype(auto) ForwardTupleArgs(Func&& f, std::tuple<Args...>&& tup)
  {
    return ForwardTupleArgs(std::forward<Func>(f), std::forward<std::tuple<Args...>>(tup), gen_seq<sizeof...(Args)>{});
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename Func, typename... Ts> 
class LazyFunctor
{
protected:
  Func op;
  std::tuple<Ts&&...> values;

public:

  LazyFunctor (Func&& f, Ts&&... ts) 
  : op(std::forward<Func>(f))
  , values( std::forward<Ts&&...>(ts...) ) {}

  // evaluate all args upon derefence
  decltype(auto) operator*() { 
    return Helper::ForwardTupleArgs(std::forward<Func>(op), std::forward<decltype(values)>(values)); 
  }
};

template <typename Func> 
class LazyFunctorNoArgs
{
protected:
  Func op;

public:
  LazyFunctorNoArgs (Func&& f) 
  : op(std::forward<Func>(f)) {}

  template <typename... Ts> 
  decltype(auto) operator() (Ts&&... ts) { 
    return LazyFunctor<Func, Ts...>(std::forward<Func>(op), std::forward<Ts&&...>(ts...));
  }
};

template <typename Func>
inline decltype(auto) MakeLazy(Func&& f) {
  return LazyFunctorNoArgs<Func>(std::forward<Func>(f));
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename Func, typename T, typename... Ts> 
class LazyFunctorMember
{
protected:
  Func op;
  std::tuple<Ts&&...> values;

public:

  LazyFunctorMember (Func&& f, Ts&&... ts) 
  : op(std::forward<Func>(f))
  , values( std::forward<Ts&&...>(ts...) ) {}

  decltype(auto) operator() (T&& t) { 
    return LazyFunctor<Func, T, Ts...>(std::forward<Func>(op), std::forward<T&&, Ts&&...>(t, ts...));
  }

  // evaluate all args upon derefence
  // decltype(auto) operator*() { 
  //   return Helper::ForwardTupleArgs(std::forward<Func>(op), std::forward<decltype(values)>(values)); 
  // }
};

template <typename Func, typename T> 
class LazyFunctorMemberNoArgs
{
protected:
  Func op;

public:
  LazyFunctorMemberNoArgs (Func&& f) 
  : op(std::forward<Func>(f)) {}

  template <typename... Ts> 
  decltype(auto) operator() (Ts&&... ts) { 
    return LazyFunctorMember<Func, Ts...>(std::forward<Func>(op), std::forward<Ts&&...>(ts...));
  }
};


template <typename T, typename Func>
inline decltype(auto) MakeLazyMember(Func&& f) {
  return LazyFunctorMemberNoArgs<Func, T>(std::forward<Func>(f));
};


TEST(TestMaybe, TestCurryReferencePassing)
{
  // testing binding a Maybe
  using namespace Anki::Util;
  using namespace Anki::Util::MaybeOperators;

  // auto decr = Curry(std::mem_fn(&IntWrapper::decr));
  auto decrM = std::mem_fn(&IntWrapper::decr);
  auto decr = MakeLazy(decrM);
  // auto decrL = MakeLazy( [](IntWrapper& w) -> int { return w.decr(); } );

  IntWrapper eight(8);
  eight.decr();
  EXPECT_EQ(7, eight.x) << "no map";


  EXPECT_EQ(6, decrM(eight)) << "mem_fn bad return";
  EXPECT_EQ(6, eight.x) << "mem_fn bad";

  auto tupleArgs = std::tuple<IntWrapper&&>( std::forward<IntWrapper>(eight) );
  int retv = Helper::ForwardTupleArgs(std::forward<decltype(decrM)>(decrM), std::forward<decltype(tupleArgs)>(tupleArgs)); 
  EXPECT_EQ(5, retv) << "ForwardTupleArgs not passing object by reference";
  EXPECT_EQ(5, eight.x) << "ForwardTupleArgs not passing object by reference";

  // auto just8 = Just<IntWrapper>(8);
  // auto nada = Nothing<IntWrapper>();

  retv = *decr(eight);

  EXPECT_EQ(4, retv) << "curry bad result";
  EXPECT_EQ(4, eight.x) << "curry not passing object by reference";

  // int eval = *decr(eight);
  // EXPECT_EQ(6, eval) << "no map";
  auto decrMem = MakeLazyMember<IntWrapper>(std::mem_fn(&IntWrapper::decr));

  auto nada = Nothing<IntWrapper>();
  // auto just10 = Just<IntWrapper>(10);

  // auto retv = just10->*decrMem();
  nada->*decrMem();

  EXPECT_EQ(6, eight.x) << "no map";


  // nada->*sub(5);

  // auto result0 = decrR.ValueOr(0.f);
  // auto result1 = just8.ValueOr(IntWrapper(0));
  // auto result2 = nada.ValueOr(IntWrapper(-5));

  // EXPECT_EQ(7, *result0) << "no decr val";
  // EXPECT_EQ(7, result1.x) << "no map";
  // EXPECT_EQ(-5, result2.x) << "not nada";
}

TEST(TestMaybe, TestCurry)
{
  // using namespace Anki::Util;

  // auto a = [](double x, double y) { return x + y; };
  // auto b = Curry(a, 5);
  // auto bp = Curry(a, 5, 2);
  // auto cp = b(2);
  // double c = *cp;
  // double d = *bp;

  // EXPECT_EQ(c, a(5,2));
  // EXPECT_EQ(d, a(5,2));

  // auto trip = [](double x, double y, double z) { return x + y - z; };
  // auto twoArgs = Curry(trip, 5);
  // double done1 = *twoArgs(2, 3);
  // auto oneArg = Curry(trip, 5, 2);
  // double done2 = *oneArg(3);


  // EXPECT_EQ( done1, trip(5,2,3) );
  // EXPECT_EQ( done2, trip(5,2,3) );
  // EXPECT_EQ( done2, done1 );
  // EXPECT_EQ( done1, 4 );


  // auto nest = Curry(twoArgs, 2);
  // double done3 = *nest(3); // TODO:: fix this double deref here
  // EXPECT_EQ( done2, done3 );
}


TEST(TestMaybe, NothingVoid)
{
  using namespace Anki::Util;

  // from Nothing<void>
  auto notAnything = Nothing<void>();

  int calledCount = 0;
  auto get2 = [&calledCount]() -> int { 
    ++calledCount;
    return 2; 
  };
    
  auto maybe2 = [&calledCount]() { 
    ++calledCount;
    return Just(2); 
  };

  auto resultFMap     = notAnything.FMap(get2);
  auto resultBind     = notAnything.Bind(maybe2);
  auto resultNotThis  = notAnything.ThisOr(Nothing<void>());
  // NOTE: ValueOr does not exist for Maybe<void> since void cannot resolve to an instance
  
  EXPECT_TRUE(notAnything.IsNothing());
  EXPECT_FALSE(notAnything.IsJust());
  EXPECT_TRUE(resultFMap.IsNothing());
  EXPECT_TRUE(resultBind.IsNothing());
  EXPECT_EQ(0, calledCount) << "bind or fmap called the bound method even though the input was Nothing";

  EXPECT_EQ(6, resultFMap.ValueOr(6)) << "Fmap result did not return default value in `ValueOr`";
  EXPECT_TRUE(resultNotThis.IsNothing()) << "Bind result did not return default value in `ThisOr`";

  // to Nothing<void>
  auto notInt = Nothing<int>();
  auto voidM  = notInt.FMap( [](auto x) { ++x; } );
  auto voidM2 = Nothing<int>().FMap( [](auto x) { ++x; } );

  EXPECT_TRUE(voidM.IsNothing());
  EXPECT_TRUE(voidM2.IsNothing());
  static_assert(std::is_same<decltype(voidM)::Type, void>(), "");
}

TEST(TestMaybe, JustVoid)
{
  using namespace Anki::Util;

  // from Just<void>
  auto justEmpty = Just<void>();

  auto get2   = []() { return 2; };
  auto maybe4 = []() { return Just(4); };

  auto resultFMap = justEmpty.FMap(get2);
  auto resultBind = justEmpty.Bind(maybe4);
  auto resultThis = justEmpty.ThisOr(Nothing<void>());
  // NOTE: ValueOr does not exist for Maybe<void> since void cannot resolve to an instance
  
  EXPECT_FALSE(justEmpty.IsNothing());
  EXPECT_TRUE(justEmpty.IsJust());

  EXPECT_TRUE(resultFMap.IsJust());
  EXPECT_TRUE(resultBind.IsJust());
  EXPECT_TRUE(resultThis.IsJust());

  EXPECT_EQ(2, resultFMap.ValueOr(6)) << "Fmap result did not return current value in `ValueOr`";
  EXPECT_EQ(4, resultBind.ValueOr(6)) << "Bind result did not return current value in `ValueOr`";

  // to Just<void>
  auto justInt = Just<int>(5);
  auto voidM  = justInt.FMap( [](auto x) { ++x; } );

  EXPECT_TRUE(voidM.IsJust());
  static_assert(std::is_same<decltype(voidM)::Type, void>(), "");
}

TEST(TestMaybe, ValueOrTypeConversion) 
{
  using namespace Anki::Util;
  auto intM = Just<int>(2); 
  auto result = intM.ValueOr(.2);

  static_assert(std::is_same<decltype(result), int>(), "ValueOr does maintain declaired type" );
  EXPECT_TRUE( result == 2 ) << "calculated " << result << ", not 2.";

  intM = Nothing<int>();
  result = intM.ValueOr(.2);
  EXPECT_TRUE( result == 0 ) << "calculated " << result << ", should of cast (float .2) to (int 0).";
}

TEST(TestMaybe, ThisOrInternalTypeConversion) 
{
  using namespace Anki::Util;

  const float floatVal = -5.6;
  auto intM = Just<int>(2); 
  auto floatM = Just<float>(floatVal);
  auto resultM = intM.ThisOr(floatM);
  auto result = resultM.ValueOr(0);

  static_assert(std::is_same<decltype(resultM), decltype(intM)>(), "ThisOr does maintain left declaired type" );
  EXPECT_TRUE( result == 2 ) << "calculated " << result << ", not 2.";

  intM = Nothing<int>();
  resultM = intM.ThisOr(floatM);
  result = resultM.ValueOr(0);
  EXPECT_TRUE( result == ((int) floatVal) ) << "calculated " << result << ", should of cast (float " << floatVal << ") to (int " <<  (int) floatVal << ").";
}