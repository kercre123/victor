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

#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"

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
  FastPolygon::ResetCounts();

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

GTEST_TEST(TestPolygon, FastPolygonSortedEdges)
{
  FastPolygon::ResetCounts();

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

  FastPolygon fastPoly(poly);

  // Create a uniform grid of test points around the polygon
  const float stepSize = 1.0;

  std::vector<Point2f> testPoints;

  for(float x = -250.0; x < -20.0; x += stepSize) {
    for(float y = 80.0; y < 250.0; y += stepSize) {
      testPoints.emplace_back( Point2f {x, y} );
    }
  }

  printf("created %lu test points\n", testPoints.size());

  std::vector<bool> pointInside;
  unsigned int numInside = 0;
  for(const auto& point : testPoints) {
    bool inside = fastPoly.Contains(point);
    if(inside)
      numInside++;
    pointInside.push_back( inside );
  }

  ASSERT_GT(numInside, testPoints.size() / 8) << "not enough points inside polygon";
  ASSERT_LT(numInside, testPoints.size() * (7.0 / 8.0)) << "too many points inside polygon";

  ASSERT_GT(FastPolygon::_numDotProducts, 0) << "not tracking dot products! must compile in debug!";

  unsigned int numDotsUnsorted = FastPolygon::_numDotProducts;

  ////////////////////////////////////////////////////////////////////////////////

  FastPolygon::ResetCounts();
  ASSERT_EQ(FastPolygon::_numDotProducts, 0) << "count reset didn't work!";

  fastPoly.SortEdgeVectors();


  for( size_t i=0; i<testPoints.size(); ++i) {
    ASSERT_EQ( pointInside[i], fastPoly.Contains( testPoints[i] ) )
      << "point["<<i<<"]: "<<testPoints[i]<<" inconsistent! Before sort, value was "<<pointInside[i];
  }

  unsigned int numDotsSorted = FastPolygon::_numDotProducts;
  ASSERT_GT(numDotsSorted, 0) << "not correctly tracking dot products!";

  EXPECT_LT(numDotsSorted, numDotsUnsorted) << "should take less work once sorted";

  printf("took %u dot products unsorted, and %u sorted\n", numDotsUnsorted, numDotsSorted);

}

GTEST_TEST(TestPoly, CladConversion)
{
  // 2D:
  {
    const Poly2f poly2d {
      {1.2f, 2.3f},
      {3.4f, 4.5f},
      {5.6f, 6.7f},
      {7.8f, 8.9f},
      {9.0f, 0.1f},
    };
    
    const std::vector<CladPoint2d> cladPoly2d = poly2d.ToCladPoint2dVector();
    
    EXPECT_EQ(poly2d.size(), cladPoly2d.size());
    
    auto polyIter = poly2d.begin();
    auto polyEnd  = poly2d.end();
    auto cladIter = cladPoly2d.begin();
    auto cladEnd  = cladPoly2d.end();
    
    while(polyIter != polyEnd && cladIter != cladEnd)
    {
      EXPECT_TRUE(Util::IsFltNear(polyIter->x(), cladIter->x));
      EXPECT_TRUE(Util::IsFltNear(polyIter->y(), cladIter->y));
      ++polyIter;
      ++cladIter;
    }
    
    const Poly2f polyCheck(cladPoly2d);
    auto polyCheckIter = polyCheck.begin();
    auto polyCheckEnd  = polyCheck.end();
    polyIter = poly2d.begin();
    
    while(polyIter != polyEnd && polyCheckIter != polyCheckEnd)
    {
      EXPECT_TRUE(IsNearlyEqual(*polyIter, *polyCheckIter));
      ++polyIter;
      ++polyCheckIter;
    }
  }
  
  // 3D:
  {
    const Poly3f poly3d {
      {1.2f, 2.3f, -3.2f},
      {3.4f, 4.5f, -5.4f},
      {5.6f, 6.7f, -7.6f},
      {7.8f, 8.9f, -9.8f},
      {9.0f, 0.1f, -1.0f},
      {-0.1f, -1.2f, -2.3f},
    };
    
    const std::vector<CladPoint3d> cladPoly3d = poly3d.ToCladPoint3dVector();
    
    EXPECT_EQ(poly3d.size(), cladPoly3d.size());
    
    auto polyIter = poly3d.begin();
    auto polyEnd  = poly3d.end();
    auto cladIter = cladPoly3d.begin();
    auto cladEnd  = cladPoly3d.end();
    
    while(polyIter != polyEnd && cladIter != cladEnd)
    {
      EXPECT_TRUE(Util::IsFltNear(polyIter->x(), cladIter->x));
      EXPECT_TRUE(Util::IsFltNear(polyIter->y(), cladIter->y));
      EXPECT_TRUE(Util::IsFltNear(polyIter->z(), cladIter->z));
      ++polyIter;
      ++cladIter;
    }
    
    const Poly3f polyCheck(cladPoly3d);
    auto polyCheckIter = polyCheck.begin();
    auto polyCheckEnd  = polyCheck.end();
    polyIter = poly3d.begin();
    
    while(polyIter != polyEnd && polyCheckIter != polyCheckEnd)
    {
      EXPECT_TRUE(IsNearlyEqual(*polyIter, *polyCheckIter));
      ++polyIter;
      ++polyCheckIter;
    }
  }
  
}
