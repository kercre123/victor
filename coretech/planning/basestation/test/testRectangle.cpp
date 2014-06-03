#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "gtest/gtest.h"
#include <set>
#include <vector>

using namespace std;
using namespace Anki;

GTEST_TEST(TestPlannerRectangle, RectangleIntersectionsRotated)
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

GTEST_TEST(TestPlannerRectangle, RectangleIntersectionsAligned)
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
