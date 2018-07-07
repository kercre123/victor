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
#include "coretech/common/engine/math/point_impl.h"
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

TEST( TestRobotPosSampler, RejectIfInRange )
{
  using namespace Anki;
  
  Anki::Util::RandomGenerator rng(123);
  const float minDist_mm = 0.5f;
  const float maxDist_mm = 1.0f;
  
  std::vector<Point2f> otherPts;
  otherPts.emplace_back(0.f, 0.f); // (0, 0)
  otherPts.emplace_back(5.f, 0.f); // (5, 0)
  otherPts.emplace_back(0.f, 5.f); // (0, 5)
  
  RobotPointSamplerHelper::RejectIfInRange rejectIfInRange(minDist_mm, maxDist_mm);
  rejectIfInRange.SetOtherPositions(otherPts);
  
  // Generate some test samples which are definitely in range or out of range
  const size_t nSamples = 100;
  std::vector<Point2f> inRangeSamples;
  std::vector<Point2f> notInRangeSamples;
  for (const auto otherPt : otherPts) {
    std::generate_n(std::back_inserter(inRangeSamples), nSamples,
                    [&](){
                      return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, minDist_mm, maxDist_mm);
                    });
    std::generate_n(std::back_inserter(notInRangeSamples), nSamples,
                    [&](){
                      return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, 0.f, 0.5f);
                    });
    std::generate_n(std::back_inserter(notInRangeSamples), nSamples,
                    [&](){
                      return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, 1.f, 4.f);
                    });
  }
  
  // none of the "other points" themselves should not be in range since we specified a
  // nonzero minDist_mm
  notInRangeSamples.insert(notInRangeSamples.end(),
                           otherPts.begin(),
                           otherPts.end());
  
  for (const auto& pt : inRangeSamples) {
    EXPECT_FALSE(rejectIfInRange(pt)) << "Sample point " << pt << "unexpectedly returned true for rejectIfInRange";
  }
  for (const auto& pt : notInRangeSamples) {
    EXPECT_TRUE(rejectIfInRange(pt)) << "Sample point " << pt << "unexpectedly returned false for rejectIfInRange";
  }
}


TEST( TestRobotPosSampler, RejectIfNotInRange )
{
  using namespace Anki;
  
  Anki::Util::RandomGenerator rng(123);
  const float minDist_mm = 0.5f;
  const float maxDist_mm = 1.0f;
 
  RobotPointSamplerHelper::RejectIfNotInRange rejectIfNotInRange(minDist_mm, maxDist_mm);
  
  Point2f otherPt(5.f, 5.f);
  rejectIfNotInRange.SetOtherPosition(otherPt);
  
  // Generate some test samples which are definitely in range or out of range
  const size_t nSamples = 100;
  std::vector<Point2f> inRangeSamples;
  std::vector<Point2f> notInRangeSamples;
  
  std::generate_n(std::back_inserter(inRangeSamples), nSamples,
                  [&](){
                    return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, minDist_mm, maxDist_mm);
                  });
  std::generate_n(std::back_inserter(notInRangeSamples), nSamples,
                  [&](){
                    return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, 0.f, 0.5f);
                  });
  std::generate_n(std::back_inserter(notInRangeSamples), nSamples,
                  [&](){
                    return otherPt + RobotPointSamplerHelper::SamplePointInAnnulus(rng, 1.f, 10.f);
                  });
  
  // the "other point" itself should not be in range since we specified a nonzero minDist_mm
  notInRangeSamples.push_back(otherPt);
  
  for (const auto& pt : inRangeSamples) {
    EXPECT_TRUE(rejectIfNotInRange(pt)) << "Sample point " << pt << "unexpectedly returned false for rejectIfNotInRange";
  }
  for (const auto& pt : notInRangeSamples) {
    EXPECT_FALSE(rejectIfNotInRange(pt)) << "Sample point " << pt << "unexpectedly returned true for rejectIfNotInRange";
  }
  
}

// todo: more tests
