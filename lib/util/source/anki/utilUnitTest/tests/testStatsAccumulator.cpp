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
#include "util/stats/bivariateStatsAccumulator.h"

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

TEST(StatsAccumulator, Bivariate)
{
  BivariateStatsAccumulator bva;
  
  // samples of a bivariate gaussian with params
  float meanx = 1;
  float meany = -2;
  float cov11 = 1;
  float cov12 = 0.2;
  float cov22 = 2;
    
  std::vector<float> x
    = {1.62180021e-01, -3.74181812e-01,  4.55431539e-01, -1.42271548e-01,
       2.33873386e+00,  1.79201031e+00,  1.02344387e+00,  6.82744254e-01,
       2.97540069e-01, -3.60752458e-01,  1.12836233e+00,  8.50282966e-01,
       1.47052651e+00,  1.00696357e+00,  1.11025678e+00, -1.63310005e-02,
       -1.44139097e+00,  1.49609369e+00,  6.38794829e-01,  1.05005273e+00,
       7.84595069e-01,  3.39975991e-01, -3.61374817e-01, -1.24798548e+00,
       1.94352737e+00,  2.11820164e-01,  6.27991912e-01,  2.71231810e+00,
       6.68432343e-01,  1.87481188e+00, -1.22481120e+00,  1.28884279e+00,
       1.94176683e+00,  1.03749186e+00,  1.09380122e+00, -2.86471870e-01,
       2.42349715e-01,  9.91215248e-01,  9.09689319e-01,  2.28900659e-01,
       1.68843317e+00,  1.22656635e+00,  2.49717453e+00,  1.82243898e+00,
       1.17776588e+00,  1.42574403e+00,  2.82725172e-03,  6.12553534e-01,
       1.16041960e+00,  1.10644799e+00,  1.88867578e+00,  5.61248977e-01,
       -1.84029050e-01,  1.78148582e+00,  1.47603381e+00,  1.36608462e+00,
       2.55208861e-01,  1.45973389e+00,  5.12293856e-02,  8.11410693e-01,
       2.34357071e+00,  9.16709554e-01,  1.38959885e+00,  2.02447035e+00,
       1.92215089e+00, -1.20447894e-01,  1.22067372e+00,  1.24562043e+00,
       1.08143022e+00, -2.55512669e-01,  1.25649903e+00,  1.59849840e-02,
       6.31072453e-01,  2.34688107e+00,  1.18820872e+00,  5.92427826e-01,
       2.53661127e+00,  1.18525163e+00,  2.63158550e+00,  1.00241813e+00,
       8.05358352e-01,  9.04062148e-01, -3.29449228e-01,  2.78693328e+00,
       1.31004193e+00,  1.66921850e+00,  2.35374350e+00,  6.31408597e-01,
       1.50252567e+00,  2.27334839e+00,  1.72155987e+00,  3.30387558e+00,
       8.09347759e-02,  1.65318606e+00,  1.97395562e+00,  2.58761436e+00,
       1.00765329e+00,  1.03331111e+00, -1.75938587e-01,  1.60874939e+00};
  
  std::vector<float> y
    = {-2.07845047, -0.44906082, -2.94011059, -1.25740058, -2.41562942, -1.35391902,
       -1.45502379, -1.20402376, -1.24978077, -2.95488514, -4.05757905, -2.26258651,
       -2.66682291, -3.23946847, -2.5896344 , -1.44815676, -0.9259421 ,  1.57390816,
       -3.04215803, -1.01888652, -0.32992881, -1.84453437, -1.71193228, -1.35154552,
       -2.7462413 , -5.41073602, -2.53849543, -0.93929871, -3.77987645, -1.43564907,
       -1.66113778, -3.10783398,  1.06282647, -2.20088547, -2.16930406, -2.68796346,
       -4.16718419, -2.46413741, -1.6893647 , -3.54688634, -0.05874837, -1.52484762,
       -0.47855649, -0.8166382 , -1.44457512,  0.29338412, -4.1397113 , -1.69342905,
       -1.73425073, -0.4013719 , -3.7548537 , -0.08169498, -3.01218659, -2.17105105,
       -3.48117718, -3.0729152 , -1.48786404, -1.96937728, -1.17106686, -1.41781454,
       -1.27878406, -0.33264455, -2.34147072, -1.26172507,  1.86352357, -0.98785429,
       -3.0826252 , -2.39426437, -1.08650775, -2.59460482, -1.55089022, -2.15528357,
       -2.6054821 , -3.93184509, -2.47241871, -1.5256092 , -2.79836948, -2.59590729,
       0.23571758, -1.96253832, -2.11337847, -2.37725976, -1.72237592,  0.28528652,
       -0.50714006, -4.57576283, -1.76160403, -3.64997453, -1.10512487, -2.41139806,
       -3.06791979, -3.56716411, -1.73706921, -3.00313457, -3.16319208, -2.98262859,
       -2.92647834, -4.58832568, -3.00243882,  0.75763483};
  
  ASSERT_EQ( x.size(), y.size() );
  for( size_t i=0; i<x.size(); ++i ) {
    bva.Accumulate(x[i],y[i]);
  }
  
  float minx = *std::min_element(x.begin(), x.end());
  float miny = *std::min_element(y.begin(), y.end());
  float maxx = *std::max_element(x.begin(), x.end());
  float maxy = *std::max_element(y.begin(), y.end());
  
  
  EXPECT_EQ( bva.GetNum(), x.size() );
  EXPECT_NEAR( bva.GetMinX(), minx, 0.15f*fabs(minx) );
  EXPECT_NEAR( bva.GetMinY(), miny, 0.15f*fabs(miny) );
  EXPECT_NEAR( bva.GetMaxX(), maxx, 0.15f*fabs(maxx) );
  EXPECT_NEAR( bva.GetMaxY(), maxy, 0.15f*fabs(maxy) );
  EXPECT_NEAR( bva.GetMeanX(), meanx, 0.15f*fabs(meanx) );
  EXPECT_NEAR( bva.GetMeanY(), meany, 0.15f*fabs(meany) );
  EXPECT_NEAR( bva.GetVarianceX(), cov11, 0.15f*fabs(cov11) );
  EXPECT_NEAR( bva.GetVarianceY(), cov22, 0.15f*fabs(cov22) );
  float rho = cov12 / (sqrt(cov11) * sqrt(cov22));
  EXPECT_NEAR( bva.GetCovariance(), rho, 0.20f*fabs(rho) );
  
  // now do it with constants
  
  bva.Clear();
  
  for( int i=0; i<100; ++i ) {
    bva.Accumulate(1.0f,2.0f);
  }
  
  EXPECT_EQ( bva.GetNum(), 100 );
  EXPECT_NEAR( bva.GetMinX(), 1.0f, 1e-5f);
  EXPECT_NEAR( bva.GetMinY(), 2.0f, 1e-5f);
  EXPECT_NEAR( bva.GetMaxX(), 1.0f, 1e-5f);
  EXPECT_NEAR( bva.GetMaxY(), 2.0f, 1e-5f);
  EXPECT_NEAR( bva.GetMeanX(), 1.0f, 1e-5f);
  EXPECT_NEAR( bva.GetMeanY(), 2.0f, 1e-5f);
  EXPECT_NEAR( bva.GetVarianceX(), 0.0f, 1e-5f);
  EXPECT_NEAR( bva.GetVarianceY(), 0.0f, 1e-5f);
  EXPECT_NEAR( bva.GetCovariance(), 0.0f, 1e-5f);
  
  
  
}
