/**
 * File: testLineSegment2d.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2/21/18
 *
 * Description: Various tests for lines intersection
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/lineSegment2d.h"

/*
 * Test Line Segment Intersection
 */
TEST(LineSegment, TestIntersection)
{
  {
    Anki::LineSegment l1( {5.0f, 1.0f}, {5.0f, -1.0f});
    Anki::LineSegment l2( {-5.0f, 0.0f}, {10.0f, 0.0f});

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_TRUE(res);
    EXPECT_EQ(Anki::Point2f(5.0, 0.0), intersection);
  }

  {
    Anki::LineSegment l1( {1.0f, 1.0f}, {10.0f, 10.0f});
    Anki::LineSegment l2( {5.0f, 0.0f}, {5.0f, 10.0f});

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_TRUE(res);
    EXPECT_EQ(Anki::Point2f(5.0, 5.0), intersection);
  }

  {
    Anki::LineSegment l1( {1.0f, 10.0f}, {15.0f, 3.0f});
    Anki::LineSegment l2( {-5.0f, -7.0f}, {20.0f, 15.0f});

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_TRUE(res);
    EXPECT_EQ(Anki::Point2f(9.492753623188406, 5.753623188405797), intersection);
  }

  {
    Anki::LineSegment l1(Anki::Point2f(11.0, -10.0),Anki::Point2f(15.0, 3.0));
    Anki::LineSegment l2(Anki::Point2f(-5.0, -7.0),Anki::Point2f(13.0, 15.0));

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_FALSE(res);
  }

  {
    Anki::LineSegment l1(Anki::Point2f(11.0, -10.0),Anki::Point2f(23.0, 30.0));
    Anki::LineSegment l2(Anki::Point2f(-5.0, -7.0),Anki::Point2f(23.0, 28.0));

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_TRUE(res);
    EXPECT_EQ(Anki::Point2f(22.04, 26.8), intersection);
  }

  // overlapping lines
  {
    Anki::LineSegment l1(Anki::Point2f(0.0, 0.0),Anki::Point2f(5.0, 5.0));
    Anki::LineSegment l2(Anki::Point2f(2.0, 2.0),Anki::Point2f(10.0, 10.0));

    Anki::Point2f intersection;
    bool res = l1.IntersectsAt(l2, intersection);
    ASSERT_FALSE(res);
  }
  
}