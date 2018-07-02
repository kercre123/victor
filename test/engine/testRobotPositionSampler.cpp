/**
 * File: testRobotPositionSampler
 *
 * Author: ross
 * Created: 10/14/15
 *
 * Description: Tests robot point sampling
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=TestRobotPosSampler.*
 **/

#include "gtest/gtest.h"
#include "engine/utils/robotPointSamplerHelper.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

using namespace Anki::Cozmo;

TEST( TestRobotPosSampler, DistributionsCorrect )
{
  // verifies that the (x,y) output of SamplePointInAnnulus is uniform by binning into a grid and
  // checking its sample statistics.
  
  Anki::Util::RandomGenerator rng(123);
  using Grid = std::vector<std::vector<int>>;
  
  const float minR = 1;
  const float maxR = 2;
  const float centerX = maxR;
  const float centerY = maxR;
  
  auto sampleMorePoints = [&]( Grid& grid, unsigned int numPoints ) {
    int numBins = (int) grid.size();
    const float binSize = 2*maxR / numBins;
    
    for( int i=0; i<numPoints; ++i ) {
      auto pt = RobotPointSamplerHelper::SamplePointInAnnulus( rng, minR, maxR );
      const float rsq = pt.x()*pt.x() + pt.y()*pt.y();
      EXPECT_LE( rsq, maxR*maxR );
      EXPECT_GE( rsq, minR*minR );
      
      pt.x() += centerX; // center at (2,2)
      pt.y() += centerY;
      int i1 = pt.x() / binSize;
      int i2 = pt.y() / binSize;
      ++grid[i1][i2];
    }
  };
  
  // now check every square bin that is fully contained by the ring has the same approx count
  auto getCOV = [&](const Grid& grid, float& prob) {
    int numBins = (int)grid.size();
    const float binSize = 2*maxR / numBins;
    std::vector<int> containedCounts;
    float totalCounts = 0.0f;
    for( int i=0; i<numBins; ++i ) {
      const float x1 = i*binSize;
      const float x2 = (i+1)*binSize;
      for( int j=0; j<numBins; ++j ) {
        
        const float y1 = j*binSize;
        const float y2 = (j+1)*binSize;
        
        const float r1Sq = (x1-centerX)*(x1-centerX) + (y1-centerY)*(y1-centerY);
        const float r2Sq = (x1-centerX)*(x1-centerX) + (y2-centerY)*(y2-centerY);
        const float r3Sq = (x2-centerX)*(x2-centerX) + (y1-centerY)*(y1-centerY);
        const float r4Sq = (x2-centerX)*(x2-centerX) + (y2-centerY)*(y2-centerY);
        const bool contained = (r1Sq >= minR*minR) && (r1Sq <= maxR*maxR) &&
                               (r2Sq >= minR*minR) && (r2Sq <= maxR*maxR) &&
                               (r3Sq >= minR*minR) && (r3Sq <= maxR*maxR) &&
                               (r4Sq >= minR*minR) && (r4Sq <= maxR*maxR);
        if( contained ) {
          totalCounts += grid[i][j];
          containedCounts.push_back( grid[i][j] );
        }
      }
    }
    
    // ratio of bins containing at least one sample to the total number of bins
    prob = static_cast<float>(containedCounts.size()) / (numBins*numBins);
    
    float avg = totalCounts / containedCounts.size();
    float var = 0.0f;
    for( int i=0; i<containedCounts.size(); ++i ) {
      var += (containedCounts[i] - avg)*(containedCounts[i] - avg);
    }
    var /= (containedCounts.size() - 1);
    return sqrt(var)/avg;
  };
  
  // variance is small relative to average
  Grid coarseGrid(10, std::vector<int>(10, 0));
  sampleMorePoints(coarseGrid, 1000);
  float dummy = 0.0f;
  const float val1 = getCOV(coarseGrid, dummy);
  EXPECT_LE( val1, 0.25f );
  
  // converges with more samples
  sampleMorePoints(coarseGrid, 1000);
  const float val2 = getCOV(coarseGrid, dummy);
  EXPECT_LT( val2/val1, 0.9f ); // should drop ~10%

  // test that "throwing a dart probability" matches. we need a finer grid here based on what's
  // being tested, but we don't want to use a finer grid above since it needs way more samples
  Grid fineGrid(100, std::vector<int>(100, 0));
  sampleMorePoints(fineGrid, 1000);
  float pHit = 0.0f;
  getCOV(fineGrid, pHit);
  float pExpected = M_PI_F*( maxR*maxR - minR*minR ) / (4*maxR*maxR); // ratio of areas
  EXPECT_NEAR( pHit, pExpected, 0.05f );
  
}

// todo: more tests
