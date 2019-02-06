#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/shared/math/point.h"
 
#include <iostream>

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
