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
#include "coretech/common/shared/math/rotation.h"
#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include <set>
#include <vector>

using namespace std;
using namespace Anki;

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

void TestQuad(const Quad2f& quad, const Quad2f& expectedQuadUnsorted, float expectedWidth, float expectedHeight)
{
  RotatedRectangle r;
  r.ImportQuad(quad);

  Quad2f expectedQuad = expectedQuadUnsorted.SortCornersClockwise();

  EXPECT_FLOAT_EQ(r.GetWidth(), expectedWidth);
  EXPECT_FLOAT_EQ(r.GetHeight(), expectedHeight);

  Quad2f rect = r.GetQuad().SortCornersClockwise();

  cout<<"quad    : ";
  for(const auto& point : quad)
    cout<<point<<"; ";
  cout<<"\nrect    : ";
  for(const auto& point : rect)
    cout<<point<<"; ";
  cout<<"\nexpected: ";
  for(const auto& point : expectedQuad)
    cout<<point<<"; ";
  cout<<endl;  

  for(Quad::CornerName i=Quad::FirstCorner; i<Quad::NumCorners; ++i) {
    EXPECT_FLOAT_EQ(rect[i].x(), expectedQuad[i].x()) << "mismatch in original shape i="<<i;
    EXPECT_FLOAT_EQ(rect[i].y(), expectedQuad[i].y()) << "mismatch in original shape i="<<i;
  }

  // now move everything around and try again
  Quad2f quad2(quad);
  Quad2f expectedQuad2(expectedQuad);

  RotationMatrix2d rot(-1.456);
  Point2f trans(-1.2, 12.4);
  for(auto& point : quad2) {
    point = rot * (point + trans);
  }

  for(auto& point : expectedQuad2) {
    point = rot * (point + trans);
  }

  r.ImportQuad(quad2);
  rect = r.GetQuad().SortCornersClockwise();

  expectedQuad2 = expectedQuad2.SortCornersClockwise();

  for(Quad::CornerName i=Quad::FirstCorner; i<Quad::NumCorners; ++i) {
    EXPECT_FLOAT_EQ(rect[i].x(), expectedQuad2[i].x()) << "mismatch in rotated shape i="<<i;
    EXPECT_FLOAT_EQ(rect[i].y(), expectedQuad2[i].y()) << "mismatch in rotated shape i="<<i;
  }

}

GTEST_TEST(TestRotatedRectangle, RectFromQuad_trapezoid)
{
  Quad2f trap(Point2f(1.5, -1.0),
                  Point2f(3.5, -1.0),
                  Point2f(3.0, 1.4),
                  Point2f(2.0, 1.4));
  Quad2f expectedQuad(Point2f(1.5, 1.4),
                          Point2f(1.5, -1.0),
                          Point2f(3.5, -1.0),
                          Point2f(3.5, 1.4));
  TestQuad(trap, expectedQuad, 3.5 - 1.5, 1.4 + 1.0);
}

GTEST_TEST(TestRotatedRectangle, RectFromQuad_trapezoid2)
{
  // same as last trap, but rotated
  Quad2f trap(Point2f(-23.441, 30.445),
                  Point2f(-22.533, 32.227),
                  Point2f(-24.899, 32.870),
                  Point2f(-25.353, 31.979));
  RotatedRectangle r;
  r.ImportQuad(trap);

  EXPECT_NEAR(r.GetWidth(), 3.5 - 1.5, 0.01);
  EXPECT_NEAR(r.GetHeight(), 1.4 + 1.0, 0.01);
}

GTEST_TEST(TestRotatedRectangle, RectFromQuad_square)
{
  Quad2f quad(Point2f(-1.0, 1.0),
                  Point2f(1.0, 1.0),
                  Point2f(1.0, -1.0),
                  Point2f(-1.0, -1.0));
  Quad2f expectedQuad(quad);
  TestQuad(quad, expectedQuad, 2.0, 2.0);
}

GTEST_TEST(TestRotatedRectangle, RectFromQuad_pgram)
{
  Quad2f quad(Point2f(-1.5, 1.0),
                  Point2f(0.5, 1.0),
                  Point2f(0.9, 1.5),
                  Point2f(-1.0, 1.5));
  RotatedRectangle r;
  r.ImportQuad(quad);

  EXPECT_FLOAT_EQ(r.GetWidth(), 1.5 + 0.9);
  EXPECT_FLOAT_EQ(r.GetHeight(), 0.5);
}
