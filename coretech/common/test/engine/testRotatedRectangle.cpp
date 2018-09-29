/**
 * File: testRotatedRectangle.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-03
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect_impl.h"
#include "coretech/common/engine/math/rotation.h"
#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include <set>
#include <vector>

using namespace std;
using namespace Anki;

GTEST_TEST(TestRotatedRectangle, RectangleConstructionDirections) {
  // check to make sure the same rectangle constructed from different
  // edges result in the same RR

  std::array<Point2f, 4> corners = {{
    Point2f(0,0),
    Point2f(1,1),
    Point2f(-1,1),
    Point2f(0,2)
  }};

  float sideLength = std::sqrt(2);
  Point2f center(0,1);

  RotatedRectangle r0 = RotatedRectangle(corners[0], corners[1], sideLength);
  RotatedRectangle r1 = RotatedRectangle(corners[1], corners[0], -sideLength);
  RotatedRectangle r2 = RotatedRectangle(corners[2], corners[0], sideLength);
  RotatedRectangle r3 = RotatedRectangle(corners[0], corners[2], -sideLength);
  RotatedRectangle r4 = RotatedRectangle(corners[1], corners[3], sideLength);
  RotatedRectangle r5 = RotatedRectangle(corners[3], corners[1], -sideLength);
  RotatedRectangle r6 = RotatedRectangle(corners[3], corners[2], sideLength);
  RotatedRectangle r7 = RotatedRectangle(corners[2], corners[3], -sideLength);

  EXPECT_TRUE(r0.Contains(center)) << "r0 Contains center is false!";
  EXPECT_TRUE(r1.Contains(center)) << "r1 Contains center is false!";
  EXPECT_TRUE(r2.Contains(center)) << "r2 Contains center is false!";
  EXPECT_TRUE(r3.Contains(center)) << "r3 Contains center is false!";
  EXPECT_TRUE(r4.Contains(center)) << "r4 Contains center is false!";
  EXPECT_TRUE(r5.Contains(center)) << "r5 Contains center is false!";
  EXPECT_TRUE(r6.Contains(center)) << "r6 Contains center is false!";
  EXPECT_TRUE(r7.Contains(center)) << "r7 Contains center is false!";

  EXPECT_TRUE(r0.Contains(corners[0])) << "r0 Contains bottom is false!";
  EXPECT_TRUE(r1.Contains(corners[0])) << "r1 Contains bottom is false!";
  EXPECT_TRUE(r2.Contains(corners[0])) << "r2 Contains bottom is false!";
  EXPECT_TRUE(r3.Contains(corners[0])) << "r3 Contains bottom is false!";
  EXPECT_TRUE(r4.Contains(corners[0])) << "r4 Contains bottom is false!";
  EXPECT_TRUE(r5.Contains(corners[0])) << "r5 Contains bottom is false!";
  EXPECT_TRUE(r6.Contains(corners[0])) << "r6 Contains bottom is false!";
  EXPECT_TRUE(r7.Contains(corners[0])) << "r7 Contains bottom is false!";

  EXPECT_TRUE(r0.Contains(corners[1])) << "r0 Contains right is false!";
  EXPECT_TRUE(r1.Contains(corners[1])) << "r1 Contains right is false!";
  EXPECT_TRUE(r2.Contains(corners[1])) << "r2 Contains right is false!";
  EXPECT_TRUE(r3.Contains(corners[1])) << "r3 Contains right is false!";
  EXPECT_TRUE(r4.Contains(corners[1])) << "r4 Contains right is false!";
  EXPECT_TRUE(r5.Contains(corners[1])) << "r5 Contains right is false!";
  EXPECT_TRUE(r6.Contains(corners[1])) << "r6 Contains right is false!";
  EXPECT_TRUE(r7.Contains(corners[1])) << "r7 Contains right is false!";

  EXPECT_TRUE(r0.Contains(corners[2])) << "r0 Contains left is false!";
  EXPECT_TRUE(r1.Contains(corners[2])) << "r1 Contains left is false!";
  EXPECT_TRUE(r2.Contains(corners[2])) << "r2 Contains left is false!";
  EXPECT_TRUE(r3.Contains(corners[2])) << "r3 Contains left is false!";
  EXPECT_TRUE(r4.Contains(corners[2])) << "r4 Contains left is false!";
  EXPECT_TRUE(r5.Contains(corners[2])) << "r5 Contains left is false!";
  EXPECT_TRUE(r6.Contains(corners[2])) << "r6 Contains left is false!";
  EXPECT_TRUE(r7.Contains(corners[2])) << "r7 Contains left is false!";

  EXPECT_TRUE(r0.Contains(corners[3])) << "r0 Contains top is false!";
  EXPECT_TRUE(r1.Contains(corners[3])) << "r1 Contains top is false!";
  EXPECT_TRUE(r2.Contains(corners[3])) << "r2 Contains top is false!";
  EXPECT_TRUE(r3.Contains(corners[3])) << "r3 Contains top is false!";
  EXPECT_TRUE(r4.Contains(corners[3])) << "r4 Contains top is false!";
  EXPECT_TRUE(r5.Contains(corners[3])) << "r5 Contains top is false!";
  EXPECT_TRUE(r6.Contains(corners[3])) << "r6 Contains top is false!";
  EXPECT_TRUE(r7.Contains(corners[3])) << "r7 Contains top is false!";


}

GTEST_TEST(TestRotatedRectangle, RectangleIntersectionsRotated)
{
  RotatedRectangle R0(-0.2, 1.0, 1.2, -0.5, 0.4);
  set< pair<float, float> > pointsInR0;
  pointsInR0.insert(pair<float, float>(0, 1));
  pointsInR0.insert(pair<float, float>(0.25, 1));
  pointsInR0.insert(pair<float, float>(0.25, 0.75));
  pointsInR0.insert(pair<float, float>(0.5, 0.75));
  pointsInR0.insert(pair<float, float>(0.5, 0.5));
  pointsInR0.insert(pair<float, float>(0.5, 0.25));
  pointsInR0.insert(pair<float, float>(0.75, 0.5));
  pointsInR0.insert(pair<float, float>(0.75, 0.25));
  pointsInR0.insert(pair<float, float>(0.75, 0.0));
  pointsInR0.insert(pair<float, float>(1.0, 0.25));
  pointsInR0.insert(pair<float, float>(1.0, 0.0));
  pointsInR0.insert(pair<float, float>(1.0, -0.25));
  pointsInR0.insert(pair<float, float>(1.25, 0.0));
  pointsInR0.insert(pair<float, float>(1.25, -0.25));

  EXPECT_TRUE(R0.Contains(0.0, 1.0));

  unsigned int inside = 0;
  for(float x = -1.5; x <= 1.5; x += 0.25) {
    for(float y = -1.5; y <= 1.5; y += 0.25) {
      bool in = R0.Contains(x, y);
      if(in) {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) != pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is true, but NOT in list of points!";
        inside++;
      }
      else {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) == pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is false, but IS in list of points!";
      }
    }
  }

  EXPECT_EQ(inside, pointsInR0.size());
}

GTEST_TEST(TestRotatedRectangle, RectangleIntersectionsAligned)
{
  RotatedRectangle R0(-1.0, 0.0, 0.0, 0.0, 0.5);
  set< pair<float, float> > pointsInR0;
  pointsInR0.insert(pair<float, float>(-1.0, 0.5));
  pointsInR0.insert(pair<float, float>(-1.0, 0.25));
  pointsInR0.insert(pair<float, float>(-1.0, 0.0));
  pointsInR0.insert(pair<float, float>(-0.75, 0.5));
  pointsInR0.insert(pair<float, float>(-0.75, 0.25));
  pointsInR0.insert(pair<float, float>(-0.75, 0.0));
  pointsInR0.insert(pair<float, float>(-0.5, 0.5));
  pointsInR0.insert(pair<float, float>(-0.5, 0.25));
  pointsInR0.insert(pair<float, float>(-0.5, 0.0));
  pointsInR0.insert(pair<float, float>(-0.25, 0.5));
  pointsInR0.insert(pair<float, float>(-0.25, 0.25));
  pointsInR0.insert(pair<float, float>(-0.25, 0.0));
  pointsInR0.insert(pair<float, float>(0.0, 0.5));
  pointsInR0.insert(pair<float, float>(0.0, 0.25));
  pointsInR0.insert(pair<float, float>(0.0, 0.0));

  unsigned int inside = 0;
  for(float x = -1.5; x <= 1.5; x += 0.25) {
    for(float y = -1.5; y <= 1.5; y += 0.25) {
      bool in = R0.Contains(x, y);
      if(in) {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) != pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is true, but NOT in list of points!";
        inside++;
      }
      else {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) == pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is false, but IS in list of points!";
      }
    }
  }

  EXPECT_EQ(inside, pointsInR0.size());
}


GTEST_TEST(TestRotatedRectangle, RectangleIntersectionsAlignedNegativeWidth)
{
  RotatedRectangle R0(-1.0, 0.0, 0.0, 0.0, -0.5);
  set< pair<float, float> > pointsInR0;
  pointsInR0.insert(pair<float, float>(-1.0,  -0.5));
  pointsInR0.insert(pair<float, float>(-1.0,  -0.25));
  pointsInR0.insert(pair<float, float>(-1.0,   0.0));
  pointsInR0.insert(pair<float, float>(-0.75, -0.5));
  pointsInR0.insert(pair<float, float>(-0.75, -0.25));
  pointsInR0.insert(pair<float, float>(-0.75,  0.0));
  pointsInR0.insert(pair<float, float>(-0.5,  -0.5));
  pointsInR0.insert(pair<float, float>(-0.5,  -0.25));
  pointsInR0.insert(pair<float, float>(-0.5,   0.0));
  pointsInR0.insert(pair<float, float>(-0.25, -0.5));
  pointsInR0.insert(pair<float, float>(-0.25, -0.25));
  pointsInR0.insert(pair<float, float>(-0.25, -0.0));
  pointsInR0.insert(pair<float, float>( 0.0,  -0.5));
  pointsInR0.insert(pair<float, float>( 0.0,  -0.25));
  pointsInR0.insert(pair<float, float>( 0.0,   0.0));

  unsigned int inside = 0;
  for(float x = -1.5; x <= 1.5; x += 0.25) {
    for(float y = -1.5; y <= 1.5; y += 0.25) {
      bool in = R0.Contains(x, y);
      if(in) {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) != pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is true, but NOT in list of points!";
        inside++;
      }
      else {
        EXPECT_TRUE(pointsInR0.find(pair<float, float>(x,y)) == pointsInR0.end()) <<
          "Point ("<<x<<", "<<y<<") Contains is false, but IS in list of points!";
      }
    }
  }

  EXPECT_EQ(inside, pointsInR0.size());
}

// void TestQuad(const Quad2f& quad, const Quad2f& expectedQuadUnsorted, float expectedWidth, float expectedHeight)
// {
//   RotatedRectangle r(quad);

//   Quad2f expectedQuad = expectedQuadUnsorted.SortCornersClockwise();

//   EXPECT_FLOAT_EQ(r.GetWidth(), expectedWidth);
//   EXPECT_FLOAT_EQ(r.GetHeight(), expectedHeight);

//   Quad2f rect = r.GetQuad().SortCornersClockwise();

//   cout<<"quad    : ";
//   for(const auto& point : quad)
//     cout<<point<<"; ";
//   cout<<"\nrect    : ";
//   for(const auto& point : rect)
//     cout<<point<<"; ";
//   cout<<"\nexpected: ";
//   for(const auto& point : expectedQuad)
//     cout<<point<<"; ";
//   cout<<endl;  

//   for(Quad::CornerName i=Quad::FirstCorner; i<Quad::NumCorners; ++i) {
//     EXPECT_FLOAT_EQ(rect[i].x(), expectedQuad[i].x()) << "mismatch in original shape i="<<i;
//     EXPECT_FLOAT_EQ(rect[i].y(), expectedQuad[i].y()) << "mismatch in original shape i="<<i;
//   }

//   // now move everything around and try again
//   Quad2f quad2(quad);
//   Quad2f expectedQuad2(expectedQuad);

//   RotationMatrix2d rot(-1.456);
//   Point2f trans(-1.2, 12.4);
//   for(auto& point : quad2) {
//     point = rot * (point + trans);
//   }

//   for(auto& point : expectedQuad2) {
//     point = rot * (point + trans);
//   }

//   r = RotatedRectangle(quad2);
//   rect = r.GetQuad().SortCornersClockwise();

//   expectedQuad2 = expectedQuad2.SortCornersClockwise();

//   for(Quad::CornerName i=Quad::FirstCorner; i<Quad::NumCorners; ++i) {
//     EXPECT_FLOAT_EQ(rect[i].x(), expectedQuad2[i].x()) << "mismatch in rotated shape i="<<i;
//     EXPECT_FLOAT_EQ(rect[i].y(), expectedQuad2[i].y()) << "mismatch in rotated shape i="<<i;
//   }

// }

// GTEST_TEST(TestRotatedRectangle, RectFromQuad_trapezoid)
// {
//   Quad2f trap(Point2f(1.5, -1.0),
//                   Point2f(3.5, -1.0),
//                   Point2f(3.0, 1.4),
//                   Point2f(2.0, 1.4));
//   Quad2f expectedQuad(Point2f(1.5, 1.4),
//                           Point2f(1.5, -1.0),
//                           Point2f(3.5, -1.0),
//                           Point2f(3.5, 1.4));
//   TestQuad(trap, expectedQuad, 3.5 - 1.5, 1.4 + 1.0);
// }

// GTEST_TEST(TestRotatedRectangle, RectFromQuad_trapezoid2)
// {
//   // same as last trap, but rotated
//   Quad2f trap(Point2f(-23.441, 30.445),
//                   Point2f(-22.533, 32.227),
//                   Point2f(-24.899, 32.870),
//                   Point2f(-25.353, 31.979));
//   RotatedRectangle r(trap);

//   EXPECT_NEAR(r.GetWidth(), 3.5 - 1.5, 0.01);
//   EXPECT_NEAR(r.GetHeight(), 1.4 + 1.0, 0.01);
// }

// GTEST_TEST(TestRotatedRectangle, RectFromQuad_square)
// {
//   Quad2f quad(Point2f(-1.0, 1.0),
//                   Point2f(1.0, 1.0),
//                   Point2f(1.0, -1.0),
//                   Point2f(-1.0, -1.0));
//   Quad2f expectedQuad(quad);
//   TestQuad(quad, expectedQuad, 2.0, 2.0);
// }

// GTEST_TEST(TestRotatedRectangle, RectFromQuad_pgram)
// {
//   Quad2f quad(Point2f(-1.5, 1.0),
//                   Point2f(0.5, 1.0),
//                   Point2f(0.9, 1.5),
//                   Point2f(-1.0, 1.5));
//   RotatedRectangle r(quad);

//   EXPECT_FLOAT_EQ(r.GetWidth(), 1.5 + 0.9);
//   EXPECT_FLOAT_EQ(r.GetHeight(), 0.5);
// }
