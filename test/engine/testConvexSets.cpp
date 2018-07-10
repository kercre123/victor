/**
 * File: testConvexSets.cpp
 *
 * Author: Michael Willett
 * Created: 2/21/18
 *
 * Description: Various tests for lines intersection
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/lineSegment2d.h"
#include "coretech/common/engine/math/ball.h"
#include "coretech/common/engine/math/fastPolygon2d.h"

#include "util/random/randomGenerator.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LineSegment Tests
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// Intersection
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

// Perpendicular Bisector
TEST(LineSegment, TestBisector)
{
  {
    Anki::LineSegment l1( {-5.0f, -5.0f}, {5.0f, 5.0f});
    Anki::Line2f bi = l1.GetPerpendicularBisector();
 
    ASSERT_TRUE( bi.Contains({-1.f, 1.f}) );
    ASSERT_TRUE( bi.Contains({1.f, -1.f}) );
    ASSERT_TRUE( bi.Contains({0.f, 0.f}) );

    ASSERT_FALSE( bi.Contains({0.f, 2.f}) );
  }

  {
    Anki::LineSegment l1( {0.f, 5.0f}, {10.0f, 5.0f});
    Anki::Line2f bi = l1.GetPerpendicularBisector();
 
    ASSERT_TRUE( bi.Contains({5.f, 5.f}) );
    ASSERT_TRUE( bi.Contains({5.f, 0.f}) );

    ASSERT_FALSE( bi.Contains({0.f, 0.f}) );
    ASSERT_FALSE( bi.Contains({0.f, 5.f}) );
  }
}

// Contains
TEST(LineSegment, TestContains)
{
  // vertical line
  {
    Anki::LineSegment l( {5.0f, 1.0f}, {5.0f, -1.0f});

    ASSERT_TRUE( l.Contains({5.f,  0.f}) );
    ASSERT_TRUE( l.Contains({5.f,  1.f}) );
    ASSERT_TRUE( l.Contains({5.f, -1.f}) );

    ASSERT_FALSE( l.Contains({5.f, 2.f}) );
    ASSERT_FALSE( l.Contains({5.f, -2.f}) );
  }

  // line with slope == 1 and y-intercept == 2
  {
    Anki::LineSegment l( {2.0f, 4.0f}, {8.0f, 10.0f});

    ASSERT_TRUE( l.Contains({3.f,  5.f}) );

    ASSERT_FALSE( l.Contains({0.f, 2.f}) );
    ASSERT_FALSE( l.Contains({10.f, 12.f}) );
  }
  
  // horizontal line
  {
    Anki::LineSegment l( {-99999.f, 0.0f}, {99999.f, 0.0f});

    ASSERT_TRUE(l.Contains({999.f,  0.f}));
    ASSERT_TRUE(l.Contains({-999.f,  0.f}));
    
    //points above and below
    ASSERT_FALSE( l.Contains({0.f, 1.f}) );
    ASSERT_FALSE( l.Contains({10.f, -1.f}) );
  }
}

// InHalfPlane
TEST(LineSegment, TestInHalfPlane)
{
  // check if in positive x halfplane
  {
    Anki::Line2f xAxis({0.f, 1.f}, 0);          //  y = 0
    Anki::Line2f xAxisMinus({0.f, -1.f}, 0);    // -y = 0 (reverses sign of dot products)

    Anki::LineSegment aboveX( {-5.0f, 1.0f}, {8.0f, 8.0f} );
    Anki::LineSegment crossX( {-5.0f, 1.0f}, {8.0f, -8.0f} ) ;
    Anki::LineSegment belowX( {-5.0f, -1.0f}, {8.0f, -8.0f} );

    ASSERT_TRUE( aboveX.InHalfPlane(xAxis) );
    ASSERT_TRUE( belowX.InHalfPlane(xAxisMinus) );

    ASSERT_FALSE( aboveX.InHalfPlane(xAxisMinus) );
    ASSERT_FALSE( belowX.InHalfPlane(xAxis) );
    ASSERT_FALSE( crossX.InHalfPlane(xAxisMinus) );
    ASSERT_FALSE( crossX.InHalfPlane(xAxis) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AffineHyperplane Tests
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// Contains
TEST(AffineHyperplane, Contains)
{
  // x-axis
  {
    Anki::Line2f xAxis({0.f, 1.0}, 0);
    Anki::Util::RandomGenerator gen;

    // random points where y = 0;
    for(int i=0; i<100; ++i)
    {
      float x = gen.RandDblInRange(FLT_MIN, FLT_MAX);
      ASSERT_TRUE( xAxis.Contains({x, 0.f}) );
    }
    
    // random points where y != 0;
    for(int i=0; i<100; ++i)
    {
      float x = gen.RandDblInRange(FLT_MIN, FLT_MAX);
      float y = gen.RandDblInRange(FLT_MIN, FLT_MAX);
      EXPECT_EQ( xAxis.Contains({x, y}), (y == 0.f) ); 
    }
  }

  // Linear 4-dimensional hyperplane: x1 + x2 + x3 + x4 = 0
  {
    Anki::Point<4, float> A(1.f, 1.f, 1.f, 1.f);
    Anki::AffineHyperplane<4, float> plane(A, 0.f);
    Anki::Util::RandomGenerator gen;

    // random point on plane generator
    for(int i=0; i<100; ++i)
    {
      Anki::Point<4, float> p;
      float p4 = 0;
      for (int j=0; j<3; ++j) { 
        p[j] = gen.RandDblInRange(-99999, 99999); 
        p4 -= p[j];
      }
      p[3] = p4;

      ASSERT_TRUE( plane.Contains(p) );
    }
    
    // random point
    for(int i=0; i<100; ++i)
    {
      // get random point
      Anki::Point<4, float> p;
      for (int j=0; j<4; ++j) { 
        p[j] = gen.RandDblInRange(-99999, 99999); 
      }

      float sum = 0;
      for (int j=0; j<4; ++j) { sum += p[j]; }

      EXPECT_EQ( plane.Contains(p) , (sum == 0.f) ); 
    }
  }
}
 
// InHalfPlane
TEST(AffineHyperplane, InHalfPlane)
{
  // All tests for linear 4-dimensional hyperplane: x1 + x2 + x3 + x4 = 0
  {
    Anki::AffineHyperplane<4, float> p({1.f, 1.f, 1.f, 1.f}, 0.f);
    
    // InHalfPlane is strictly Ax + b > 0, so cannot be in the halfplane defined by itself
    ASSERT_FALSE(p.InHalfPlane(p));
  }

  {
    Anki::Point<4, float> A(1.f, 1.f, 1.f, 1.f);
    Anki::AffineHyperplane<4, float> plane(A, 0.f);
    Anki::Util::RandomGenerator gen;

    // random offsets of plane
    for(int i=0; i<100; ++i)
    {
      float b = gen.RandDblInRange(FLT_MIN, FLT_MAX);
      Anki::AffineHyperplane<4, float> r(A, b);

      EXPECT_EQ( r.InHalfPlane(plane) , (b > 0) ); 
    }
  }

  {
    Anki::Point<4, float> A(1.f, 1.f, 1.f, 1.f);
    Anki::AffineHyperplane<4, float> plane(A, 0.f);
    Anki::Util::RandomGenerator gen;

    // small random changes in A
    for(int i=0; i<100; ++i)
    {
      Anki::Point<4, float> At = A;
      for (int j=0; j<4; ++j) { 
        At[j] += gen.RandDbl() - gen.RandDbl(); 
      }
      Anki::AffineHyperplane<4, float> r(At, 0.f);

      EXPECT_EQ( r.InHalfPlane(plane) , (A == At) ); 
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Uniform Ball Tests
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
// Contains
TEST(Ball, Contains)
{
  {
    const Anki::Point3f origin(0.f, 0.f, 0.f);
    const f32 radius = 1.f;
    const Anki::Ball3f b(origin, radius);
    Anki::Util::RandomGenerator gen;

    // random points in unit sphere
    for(int i=0; i<1000; ++i)
    {
      Anki::Point3f p;
      for (int j=0; j<3; ++j) { p[j] = gen.RandDbl() - gen.RandDbl(); }

      // VIC-2171: Unit test must apply same logic for tolerance as method being tested
      const f32 distanceBetween = Anki::ComputeDistanceBetween(p, origin);
      EXPECT_EQ( b.Contains(p), FLT_LE(SQUARE(distanceBetween), SQUARE(radius)) );
    }
  }
}

// InHalfPlane
TEST(Ball, InHalfPlane)
{
  // test axis lines    
  {
    Anki::Ball2f b({0.f, 0.f}, 10);

    Anki::Line2f xAxis({0.f, 1.f}, 0);          //  y = 0
    Anki::Line2f xAxisMinus({0.f, -1.0}, 0);    // -y = 0 (reverses sign of dot products)

    Anki::Line2f yAxis({1.f, 0.f}, 0);          //  x = 0
    Anki::Line2f yAxisMinus({-1.f, 0.f}, 0);    // -x = 0 

    ASSERT_FALSE( b.InHalfPlane(xAxis) );
    ASSERT_FALSE( b.InHalfPlane(xAxisMinus) );
    ASSERT_FALSE( b.InHalfPlane(yAxis) );
    ASSERT_FALSE( b.InHalfPlane(yAxisMinus) );
  }

  // test tangent lines    
  {
    Anki::Ball2f b({0.f, 0.f}, 10);
    
    Anki::Line2f xTangentClosed({0.f, 1.f}, 10.f);      //  y = -10
    Anki::Line2f xTangentOpen({0.f, 1.f}, 10.0001f); 

    ASSERT_FALSE( b.InHalfPlane(xTangentClosed) );
    ASSERT_TRUE( b.InHalfPlane(xTangentOpen) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ConvexPolygon Tests
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
GTEST_TEST(ConvexPolygon, GetCentroid)
{
  // center should be .5, .5 for both polygons
  Anki::ConvexPolygon poly1( {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
  } );  

  Anki::ConvexPolygon poly2( {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.5f, 1.0f}, // extra point on edge should not skew centroid
    {0.0f, 1.0f}
  } );


  Anki::Point2f centroid1 = poly1.ComputeCentroid();
  Anki::Point2f centroid2 = poly2.ComputeCentroid();

  EXPECT_FLOAT_EQ(centroid1.x(), .5f);
  EXPECT_FLOAT_EQ(centroid1.y(), .5f);
  EXPECT_FLOAT_EQ(centroid2.x(), .5f);
  EXPECT_FLOAT_EQ(centroid2.y(), .5f);
}

