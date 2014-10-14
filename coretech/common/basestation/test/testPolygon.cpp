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
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include <vector>

using namespace Anki;

GTEST_TEST(TestPolygon, MinAndMax)
{
  Poly3f poly3d;

  poly3d.emplace_back( Point3f{1.0f, 1.1f, 7.5f} );
  poly3d.emplace_back( Point3f{0.0f, 1.2f, -7.5f} );
  poly3d.emplace_back( Point3f{-1.0f, -0.8f, 30.3f} );

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

  Quad2f quad {{3.5f,  1.f}, { 2.f,  3.f}, { 1.f,  .1f}, { .5f,  1.2f}};
  Poly2f polyFromQuad(quad);

  EXPECT_FLOAT_EQ(polyFromQuad.GetMinX(), 0.5);

};
