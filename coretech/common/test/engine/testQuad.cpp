#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/engine/math/quad_impl.h"

#include <iostream>

using namespace Anki;

GTEST_TEST(TestQuad, Contains)
{
  using namespace Quad;
  
  // If you add a quad here, make sure to add a set of truths below
  const std::vector<Quad2f> quads = {{
    {{ 0.f,  0.f}, { 0.f,  1.f}, {1.f,   0.f}, { 1.f,   1.f}},
    {{-1.f, -1.f}, {-1.f,  1.f}, {1.f,  -1.f}, { 1.f,   1.f}},
    {{-.1f, 2.1f}, {-3.f, -.1f}, {2.5f, 0.6f}, {0.2f, -1.5f}},
    {{3.5f,  1.f}, { 2.f,  3.f}, { 1.f,  .1f}, { .5f,  1.2f}},
  }};
 
  // If you add a point here, make sure to add a truth entry for each quad below
  const std::vector<Point2f> points = {{
    {0.f,   0.f},
    {0.1f, 0.2f},
    {0.5f, 0.5f},
    {-.5f, -.5f},
    { 2.f,  2.f},
    { 0.f,  2.f},
    {-2.f,  1.f},
    {-2.f,  .1f},
    {-.2f, -.2f}
  }};
  
  // one set of truths for each quad
  const std::vector<std::vector<bool> > truths = {{
    {true,  true,  true,  false, false, false, false, false, false}, // one truth for each point
    {true,  true,  true,  true,  false, false, false, false, true},
    {true,  true,  true,  true,  false, true,  false, true,  true},
    {false, false, false, false, true,  false, false, false, false}
  }};
  
  // If this fails, the quads and truths definitions are messed up above
  ASSERT_EQ(quads.size(), truths.size());
  
  for(s32 iQuad=0; iQuad < quads.size(); ++iQuad)
  {
    // If this fails, there's not a truth defined for each point
    ASSERT_EQ(points.size(), truths[iQuad].size());
    
    for(s32 iPoint=0; iPoint < points.size(); ++iPoint)
    {
      const bool isContained = quads[iQuad].Contains(points[iPoint]);
      EXPECT_EQ(truths[iQuad][iPoint], isContained);
    } // for each point
    
  } // for each quad/truth
  
} // GTEST_TEST(TestQuad, Contains)
