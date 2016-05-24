/**
 * File: testStatsAccumulator
 *
 * Author: bneuman (brad)
 * Created: 2013-04-01
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "util/stats/statsAccumulator.h"

#include "util/helpers/includeGTest.h"
#include <vector>
#include <math.h>

using namespace Anki::Util::Stats;

TEST(StatsAccumulator, Empty)
{
  StatsAccumulator sa;

  EXPECT_FLOAT_EQ(sa.GetVal(), 0.0);
  EXPECT_FLOAT_EQ(sa.GetNum(), 0.0);
}

TEST(StatsAccumulator, SingleVal)
{
  StatsAccumulator sa;
  sa += 4.7;

  EXPECT_FLOAT_EQ(sa.GetVal(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetMean(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetStd(), 0.0);
  EXPECT_FLOAT_EQ(sa.GetMin(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetMax(), 4.7);
  EXPECT_FLOAT_EQ(sa.GetNum(), 1.0);
}

TEST(StatsAccumulator, MultipleVals)
{
  StatsAccumulator sa;

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
