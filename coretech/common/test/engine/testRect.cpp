#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/engine/math/rect_impl.h"

#include <iostream>

using namespace Anki;


GTEST_TEST(TestRect, TwoPointConstruction)
{
  // Float version
  {
    std::vector<f32> floatVals{-100.f, -10.f, -1.f, -.00001f, 0.f, 0.00001f, 1.f, 10.f, 100.f};
    
    for(f32 x1 : floatVals)
    {
      for(f32 y1 : floatVals)
      {
        const Point2f pt1(x1,y1);
        
        for(f32 x2 : floatVals)
        {
          for(f32 y2 : floatVals)
          {
            // Construct from two points and make sure Rectangle has valid width, height, and area
            const Point2f pt2(x2,y2);
            const Rectangle<f32> rect(pt1,pt2);
            EXPECT_GE(rect.GetWidth(),  0.f);
            EXPECT_GE(rect.GetHeight(), 0.f);
            EXPECT_GE(rect.Area(),      0.f);
          }
        }
      }
    }
  }
  
  // Integer version
  {
    std::vector<s32> intVals{-100, -10, -1, 0, 1, 10, 100};
    
    for(s32 x1 : intVals)
    {
      for(s32 y1 : intVals)
      {
        const Point2i pt1(x1,y1);
        
        for(s32 x2 : intVals)
        {
          for(s32 y2 : intVals)
          {
            // Construct from two points and make sure Rectangle has valid width, height, and area
            const Point2i pt2(x2,y2);
            const Rectangle<s32> rect(pt1,pt2);
            EXPECT_GE(rect.GetWidth(),  0);
            EXPECT_GE(rect.GetHeight(), 0);
            EXPECT_GE(rect.Area(),      0);
          }
        }
      }
    }
  }
  
} // GTEST_TEST(TestRect, Contains)

GTEST_TEST(TestRect, CladConversion)
{
  const Rectangle<float> rect(1.2f, 3.4f, 5.6f, 7.8f);
  const CladRect cladRect = rect.ToCladRect();
  
  // Check conversion to CLAD
  EXPECT_TRUE(Util::IsFltNear(rect.GetX(), cladRect.x_topLeft));
  EXPECT_TRUE(Util::IsFltNear(rect.GetY(), cladRect.y_topLeft));
  EXPECT_TRUE(Util::IsFltNear(rect.GetWidth(),  cladRect.width));
  EXPECT_TRUE(Util::IsFltNear(rect.GetHeight(), cladRect.height));
  
  // Make sure "top left" means the same thing in both
  const Point2f& topLeft = rect.GetTopLeft();
  EXPECT_TRUE(Util::IsFltNear(topLeft.x(), cladRect.x_topLeft));
  EXPECT_TRUE(Util::IsFltNear(topLeft.y(), cladRect.y_topLeft));
  
  // Make sure conversion back to Rectangle from CLAD works
  const Rectangle<f32> rectCheck(cladRect);
  EXPECT_TRUE(IsNearlyEqual(rectCheck.GetTopLeft(),     rect.GetTopLeft()));
  EXPECT_TRUE(IsNearlyEqual(rectCheck.GetBottomRight(), rect.GetBottomRight()));
  EXPECT_TRUE(IsNearlyEqual(rectCheck.GetTopRight(),    rect.GetTopRight()));
  EXPECT_TRUE(IsNearlyEqual(rectCheck.GetBottomLeft(),  rect.GetBottomLeft()));
}
