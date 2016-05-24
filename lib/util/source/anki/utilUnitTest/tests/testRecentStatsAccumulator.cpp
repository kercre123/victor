/**
 * File: testRecentStatsAccumulator
 *
 * Author: Mark Wesley
 * Created: 06/25/15
 *
 * Description: Unit tests for RecentStatsAccumulator
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=RecentStatsAccumulator*
 **/


#include "util/stats/recentStatsAccumulator.h"
#include "util/helpers/includeGTest.h"
#include <vector>


using namespace Anki::Util::Stats;


TEST(RecentStatsAccumulator, Empty)
{
  // This is a clone of the StatsAccumulator version, to verify this behaves the same way
  RecentStatsAccumulator sa(10);

  EXPECT_FLOAT_EQ(sa.GetVal(), 0.0);
  EXPECT_FLOAT_EQ(sa.GetNum(), 0.0);
}

TEST(RecentStatsAccumulator, SingleVal)
{
  // This is a clone of the StatsAccumulator version, to verify this behaves the same way
  RecentStatsAccumulator sa(10);
  sa += 4.7;

  EXPECT_FLOAT_EQ(sa.GetVal(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetMean(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetStd(), 0.0);
  EXPECT_FLOAT_EQ(sa.GetMin(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetMax(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetNum(), 1.0);
}

TEST(RecentStatsAccumulator, MultipleVals)
{
  // This is a clone of the StatsAccumulator version, to verify this behaves the same way
  RecentStatsAccumulator sa(200);

  std::vector<double> vals;
  double v = 1.3;
  double sum = 0.0;
  unsigned int num = 100;

  for(unsigned int i=0; i<num; i++) {
    sa += v;
    sum += v;
    vals.push_back(v);

    v = v - v*v/100.0;
  }

  double mean = sum / num;

  // compute std the old fashioned was
  double std = 0;
  for(unsigned int i=0; i<num; i++) {
    std += pow(vals[i] - mean, 2);
  }
  std = sqrt(std / num);

  EXPECT_FLOAT_EQ(sa.GetVal(), sum);
  EXPECT_FLOAT_EQ(sa.GetMean(), mean);
  EXPECT_FLOAT_EQ(sa.GetStd(), std);
  EXPECT_FLOAT_EQ(sa.GetVariance(), pow(std,2));
  EXPECT_FLOAT_EQ(sa.GetMax(), 1.3);
  EXPECT_FLOAT_EQ(sa.GetNum(), num);
}

TEST(RecentStatsAccumulator, MultipleResets)
{
  // Test RecentStatsAccumulator specific behaviour where it should:
  // (a) reset to give from last <= N samples only
  // (b) always give data from >0 samples, and generally >= N/2 samples (as long as >= N/2 samples have been added)

  const uint32_t kMaxSamplesTracked = 6;
  
  RecentStatsAccumulator sa(kMaxSamplesTracked);
  
  sa += 4.7;
  EXPECT_FLOAT_EQ(sa.GetVal(),   4.7);
  EXPECT_FLOAT_EQ(sa.GetMean(),  4.7);
  EXPECT_FLOAT_EQ(sa.GetStd(),   0.0);
  EXPECT_FLOAT_EQ(sa.GetMin(),   4.7);
  EXPECT_FLOAT_EQ(sa.GetMax(),   4.7);
  EXPECT_FLOAT_EQ(sa.GetNum(),   1.0);
  sa += 6.2;
  EXPECT_FLOAT_EQ(sa.GetVal(),  10.9);
  EXPECT_FLOAT_EQ(sa.GetMean(),  5.45);
  EXPECT_FLOAT_EQ(sa.GetStd(),   0.75);
  EXPECT_FLOAT_EQ(sa.GetMin(),   4.7);
  EXPECT_FLOAT_EQ(sa.GetMax(),   6.2);
  EXPECT_FLOAT_EQ(sa.GetNum(),   2.0);
  sa += -10000.0;
  sa +=  10000.0;
  EXPECT_FLOAT_EQ(sa.GetVal(),  10.9);
  EXPECT_FLOAT_EQ(sa.GetMean(),  2.725);
  EXPECT_FLOAT_EQ(sa.GetStd(),   7071.0684);
  EXPECT_FLOAT_EQ(sa.GetMin(), -10000.0);
  EXPECT_FLOAT_EQ(sa.GetMax(),  10000.0);
  EXPECT_FLOAT_EQ(sa.GetNum(),   4.0);
  sa += -500.0;
  sa +=  500.0;
  EXPECT_FLOAT_EQ(sa.GetVal(),  10.9);
  EXPECT_FLOAT_EQ(sa.GetMean(), 1.8166667);
  EXPECT_FLOAT_EQ(sa.GetStd(),  5780.7158);
  EXPECT_FLOAT_EQ(sa.GetMin(), -10000.0);
  EXPECT_FLOAT_EQ(sa.GetMax(),  10000.0);
  EXPECT_FLOAT_EQ(sa.GetNum(),   6.0);
  // next add will flush 1 tracker and move to the other (which should have ~1/2 as many values in it)
  sa +=  0.0;
  EXPECT_FLOAT_EQ(sa.GetVal(),  10000.0);
  EXPECT_FLOAT_EQ(sa.GetMean(), 2500.0);
  EXPECT_FLOAT_EQ(sa.GetStd(),  4344.5366);
  EXPECT_FLOAT_EQ(sa.GetMin(), -500.0);
  EXPECT_FLOAT_EQ(sa.GetMax(),  10000.0);
  EXPECT_FLOAT_EQ(sa.GetNum(),   4.0);
  sa +=  1.0;
  sa +=  2.0;
  EXPECT_FLOAT_EQ(sa.GetVal(),  10003.0);
  EXPECT_FLOAT_EQ(sa.GetMean(), 1667.1666);
  EXPECT_FLOAT_EQ(sa.GetStd(),  3737.7207);
  EXPECT_FLOAT_EQ(sa.GetMin(), -500.0);
  EXPECT_FLOAT_EQ(sa.GetMax(),  10000.0);
  EXPECT_FLOAT_EQ(sa.GetNum(),   6.0);
  // next add will flush 1 tracker and move to the other (which should have ~1/2 as many values in it)
  sa +=  0.0;
  EXPECT_FLOAT_EQ(sa.GetVal(),   3.0);
  EXPECT_FLOAT_EQ(sa.GetMean(),  1.0);
  EXPECT_FLOAT_EQ(sa.GetStd(),   0.81649661);
  EXPECT_FLOAT_EQ(sa.GetMin(),   0.0);
  EXPECT_FLOAT_EQ(sa.GetMax(),   2.0);
  EXPECT_FLOAT_EQ(sa.GetNum(),   3.0);
}

