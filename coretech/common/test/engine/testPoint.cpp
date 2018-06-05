#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/engine/math/point_impl.h"

#include <iostream>

#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)

using namespace std;
using namespace Anki;

GTEST_TEST(TestPoint, PointInitialization)
{
  Point3f a3(1.f, 2.f, 3.f);
  EXPECT_EQ(a3.x(), 1.f);
  EXPECT_EQ(a3.y(), 2.f);
  EXPECT_EQ(a3.z(), 3.f);
  
  Point3f b3;
  EXPECT_EQ(b3.x(), 0.f);
  EXPECT_EQ(b3.y(), 0.f);
  EXPECT_EQ(b3.z(), 0.f);
  
  b3 = {4.f, 5.f, 6.f};
  EXPECT_EQ(b3.x(), 4.f);
  EXPECT_EQ(b3.y(), 5.f);
  EXPECT_EQ(b3.z(), 6.f);
  
  a3 += 3.f;
  EXPECT_TRUE(b3 == a3);
  
  Point<5,float> p5;
  p5 = {1.f, 2.f, 3.f, 4.f, 5.f};
  for(int i=0; i<5; ++i) {
    EXPECT_EQ(p5[i], float(i+1));
  }
  
  Point<2, int> p6;
  p6 = {1, 2};
  p6.SetCast(6.5);
  for(int i=0; i<2; ++i) {
    EXPECT_EQ(p6[i], 6);
  }
  
  Point<2, float> p7;
  p7 = {3.4, 6.7};
  p6.SetCast(p7);
  EXPECT_EQ(p6[0], 3);
  EXPECT_EQ(p6[1], 6);
  
  Point<2, float> p8 = {4.5, 3.7};
  Point<2, int> p9 = {4, 3};
  EXPECT_TRUE(p9 == p8.CastTo<int>());
  
}

GTEST_TEST(TestPoint, DotProductAndLength)
{
  Point3f x(1.f, 2.f, 3.f);
  Point3f y(4.f, 5.f, 6.f);
  
  const float eps = 10.f*std::numeric_limits<float>::epsilon();
  
  EXPECT_EQ(DotProduct(x,y), 32.f);
  EXPECT_TRUE(NEAR(std::sqrt(DotProduct(x,x)), x.Length(), eps));
  
  Point<5,float> a(1.f, 2.f, 3.f, 4.f, 5.f);
  Point<5,float> b(2.f, 3.f, 4.f, 5.f, 6.f);
  
  EXPECT_EQ(DotProduct(a,b), 70.f);
  EXPECT_TRUE(NEAR(std::sqrt(DotProduct(a,a)), a.Length(), eps));
  
}

GTEST_TEST(TestPoint, Comparison)
{
  const Point3f A(1.f, 2.f, 3.f);
  const Point3f B(4.f, 5.f, 6.f);
  const Point3f C(0.5f, 1.5f, 2.f);
  const Point3f D(1.f, 3.f, 4.f);
  const Point3f E(0.5f, 2.f, 2.f);
  const Point3f F(2.f, 3.f, 2.f);
  
  // All A strictly less than B
  EXPECT_TRUE(A.AllLT(B));
  EXPECT_TRUE(A.AllLTE(B));
  EXPECT_TRUE(A.AnyLT(B));
  EXPECT_TRUE(A.AnyLTE(B));
  EXPECT_FALSE(A.AllGT(B));
  EXPECT_FALSE(A.AllGTE(B));
  EXPECT_FALSE(A.AnyGT(B));
  EXPECT_FALSE(A.AnyGTE(B));
  
  // All A strictly greater than C
  EXPECT_TRUE(A.AllGT(C));
  EXPECT_TRUE(A.AllGTE(C));
  EXPECT_FALSE(A.AllLT(C));
  EXPECT_FALSE(A.AllLTE(C));
  EXPECT_FALSE(A.AnyLT(C));
  EXPECT_FALSE(A.AnyLTE(C));
  
  // All A <= D
  EXPECT_TRUE(A.AllLTE(D));
  EXPECT_TRUE(A.AnyLTE(D));
  EXPECT_TRUE(A.AnyLT(D));
  EXPECT_FALSE(A.AllLT(D)); // A.x() == D.x()
  
  // All A >= E
  EXPECT_TRUE(A.AllGTE(E));
  EXPECT_TRUE(A.AnyGTE(E));
  EXPECT_TRUE(A.AnyGT(E));
  EXPECT_FALSE(A.AllGT(E)); // A.y() == E.y()
  
  // Mixed
  EXPECT_TRUE(A.AnyGT(F));
  EXPECT_TRUE(A.AnyLT(F));
  EXPECT_TRUE(A.AnyGTE(F));
  EXPECT_TRUE(A.AnyLTE(F));
  EXPECT_FALSE(A.AllGT(F));
  EXPECT_FALSE(A.AllLT(F));
  EXPECT_FALSE(A.AllGTE(F));
  EXPECT_FALSE(A.AllLTE(F));
  
}

GTEST_TEST(TestPoint, CladConversion)
{
  const Point2f p2(1.23f, 4.56f);
  const Point3f p3(1.23f, 4.56f, 7.89f);
  
  const CladPoint2d p2_clad = p2.ToCladPoint2d();
  const CladPoint3d p3_clad = p3.ToCladPoint3d();
  
  EXPECT_TRUE(Util::IsFltNear(p2_clad.x, p2.x()));
  EXPECT_TRUE(Util::IsFltNear(p2_clad.y, p2.y()));
  
  EXPECT_TRUE(Util::IsFltNear(p3_clad.x, p3.x()));
  EXPECT_TRUE(Util::IsFltNear(p3_clad.y, p3.y()));
  EXPECT_TRUE(Util::IsFltNear(p3_clad.z, p3.z()));
  
  const Point2f p2_check(p2_clad);
  const Point3f p3_check(p3_clad);
  
  EXPECT_TRUE(IsNearlyEqual(p2, p2_check));
  EXPECT_TRUE(IsNearlyEqual(p3, p3_check));
}
