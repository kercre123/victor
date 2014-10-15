/**
 * File: testPolygon.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-10-13
 *
 * Description: test polygon
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "gtest/gtest.h"

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include <vector>

using namespace Anki;

GTEST_TEST(TestPolygon, MinAndMax)
{
  Poly3f poly3d {
      {1.0f, 1.1f, 7.5f},
      {0.0f, 1.2f, -7.5f},
      {-1.0f, -0.8f, 30.3f}
    };

  // poly3d.emplace_back( Point3f{1.0f, 1.1f, 7.5f} );
  // poly3d.emplace_back( Point3f{0.0f, 1.2f, -7.5f} );
  // poly3d.emplace_back( Point3f{-1.0f, -0.8f, 30.3f} );

  EXPECT_NE(poly3d.begin(), poly3d.end());

  EXPECT_FLOAT_EQ(poly3d.GetMinX(), -1.0);
  EXPECT_FLOAT_EQ(poly3d.GetMinY(), -0.8);
  EXPECT_FLOAT_EQ(poly3d.GetMaxX(), 1.0);
  EXPECT_FLOAT_EQ(poly3d.GetMaxY(), 1.2);

  Poly2f poly2d(poly3d);

  EXPECT_FLOAT_EQ(poly2d.GetMinX(), -1.0);
  EXPECT_FLOAT_EQ(poly2d.GetMinY(), -0.8);
  EXPECT_FLOAT_EQ(poly2d.GetMaxX(), 1.0);
  EXPECT_FLOAT_EQ(poly2d.GetMaxY(), 1.2);

  const Poly2f& constPoly(poly2d);

  Poly2f poly2d_copy(constPoly);

  EXPECT_FLOAT_EQ(poly2d_copy.GetMinX(), -1.0);
  EXPECT_FLOAT_EQ(poly2d_copy.GetMinY(), -0.8);
  EXPECT_FLOAT_EQ(poly2d_copy.GetMaxX(), 1.0);
  EXPECT_FLOAT_EQ(poly2d_copy.GetMaxY(), 1.2);

  Quad2f quad {{3.5f,  1.f}, { 2.f,  3.f}, { 1.f,  .1f}, { .5f,  1.2f}};
  Poly2f polyFromQuad;
  polyFromQuad.ImportQuad2d(quad);

  EXPECT_FLOAT_EQ(polyFromQuad.GetMinX(), 0.5);
};

GTEST_TEST(TestPolygon, center)
{

  // means should be (0, 1, 2)

  Poly3f poly3d {
    {1.0f, 1.0f, 5.0f},
    {0.0f, 2.0f, -2.0f},
    {-1.0f, 0.0f, 3.0f}
  };

  Point3f centroid = poly3d.ComputeCentroid();

  EXPECT_FLOAT_EQ(centroid.x(), 0.0);
  EXPECT_FLOAT_EQ(centroid.y(), 1.0);
  EXPECT_FLOAT_EQ(centroid.z(), 2.0);
}

GTEST_TEST(TestPolygon, FastPolygonContains)
{
  Poly2f poly {
    {-184.256309,  108.494849},
    {-201.111228,  150.067496},
    {-201.111228,  204.267496},
    {-160.19712,   220.855432},
    { -82.19712,   220.855432},
    { -65.342201,  179.282785},
    { -65.342201,  125.082785},
    {-106.256309,  108.494849}
  };

  std::vector<Point2f> pointsInside {
    {-200.0, 202.0},
    {-180.0, 210.0},
    {-132.0, 220.0},
    {-81.0, 215.0},
    {-68.0, 180.0},
    {-67.0, 126.0},
    {-180.0, 109},
    {-188.0, 121.0},
    {-198.0, 153.0},
    {-146.0, 174.0},
    {-130.0, 164.0}
  };

  std::vector<Point2f> pointsOutside {
    {0.0, 0.0},
    {-0.0, -0.0},
    {-187.0, 214.0},
    {-61.0, 215.0},
    {-65.0, 186.0},
    {-135.0, 107.0},
    {-200.0, 105.0}
  };

  FastPolygon fastPoly(poly);

  for(const auto& point : pointsInside) {
    EXPECT_TRUE( fastPoly.Contains( point ) )
      << "should contain point ("<<point.x()<<", "<<point.y()<<")";
  }

  for(const auto& point : pointsOutside) {
    EXPECT_FALSE( fastPoly.Contains( point ) )
      << "should not point ("<<point.x()<<", "<<point.y()<<")";
  }

  printf("did %d dot products for %d checks\n", FastPolygon::_numDotProducts, FastPolygon::_numChecks);
}

