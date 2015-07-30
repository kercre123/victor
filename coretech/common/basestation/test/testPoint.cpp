#include "util/helpers/includeGTest.h"

#include "anki/common/basestation/math/point_impl.h"

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
  
}

GTEST_TEST(TestPoint, Comparison)
{
  Point3f x(1.f, 2.f, 6.f);
  Point3f y(4.f, 5.f, 3.f);

  Point<3, f32>::CompareX comp0;
  EXPECT_TRUE (comp0(x,y));
  EXPECT_FALSE(comp0(y,x));

  Point3f::CompareY comp1;
  EXPECT_TRUE (comp1(x,y));
  EXPECT_FALSE(comp1(y,x));

  Point3f::CompareZ comp2;
  EXPECT_FALSE(comp2(x,y));
  EXPECT_TRUE (comp2(y,x));
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
