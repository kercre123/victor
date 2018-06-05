#include "util/helpers/includeGTest.h"

#include "coretech/common/engine/math/linearClassifier_impl.h"
#include "util/math/math.h"

#include "json/json.h"
#include <cmath>

#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)

using namespace Anki;

Json::Value vec2json( const std::vector<f32>& vals )
{
  Json::Value val;
  for( auto iter = vals.begin(); iter != vals.end(); ++iter )
  {
    val.append(*iter);
  }
  return val;
}

GTEST_TEST(TestLinearClassifier, ClassifierInitialization)
{
  // Test uninitialized
  LinearClassifier classifier;
  std::vector<f32> testVec = {0.0f, 1.0f, -1.0f};

  EXPECT_TRUE( std::isnan(classifier.ClassifyOdds( testVec )) );
  EXPECT_TRUE( std::isnan(classifier.ClassifyLogOdds( testVec )) );
  EXPECT_TRUE( std::isnan(classifier.ClassifyProbability( testVec )) );
  EXPECT_TRUE( classifier.GetInputDim() == 0 );

  // Test initialized
  Json::Value config;
  std::vector<f32> weights(3);
  config["Weights"] = vec2json(weights);
  config["Offset"] = 0.0f;
  classifier.Init( config );

  EXPECT_FALSE( std::isnan(classifier.ClassifyOdds( testVec )) );
  EXPECT_FALSE( std::isnan(classifier.ClassifyLogOdds( testVec )) );
  EXPECT_FALSE( std::isnan(classifier.ClassifyProbability( testVec )) );
  EXPECT_FALSE( classifier.GetInputDim() == 0 );
}

GTEST_TEST(TestLinearClassifier, DimChecks)
{
  LinearClassifier classifier;
  Json::Value config;
  std::vector<f32> weights(4);
  config["Weights"] = vec2json(weights);
  config["Offset"] = 0.0f;
  classifier.Init(config);
  EXPECT_TRUE( classifier.GetInputDim() == 4 );

  std::vector<f32> testVec = {0.0f, 1.0f, -1.0f};
  EXPECT_TRUE( std::isnan(classifier.ClassifyOdds( testVec )) );
  EXPECT_TRUE( std::isnan(classifier.ClassifyLogOdds( testVec )) );
  EXPECT_TRUE( std::isnan(classifier.ClassifyProbability( testVec )) );

  testVec.push_back(3.0f);
  EXPECT_FALSE( std::isnan(classifier.ClassifyOdds( testVec )) );
  EXPECT_FALSE( std::isnan(classifier.ClassifyLogOdds( testVec )) );
  EXPECT_FALSE( std::isnan(classifier.ClassifyProbability( testVec )) );
}

GTEST_TEST(TestLinearClassifier, ClassifyChecks)
{
  LinearClassifier classifier;
  Json::Value config;
  std::vector<f32> weights = {1.0f, 2.0f, -1.0f};
  config["Weights"] = vec2json(weights);
  config["Offset"] = 1.0f;
  classifier.Init(config);

  const float eps = 1e-6;
  std::vector<f32> testVec = {3.0f, 1.0f, 1.0f};
  EXPECT_TRUE( Util::IsNear(classifier.ClassifyLogOdds( testVec ), 5.0f, eps) );
  EXPECT_TRUE( Util::IsNear(classifier.ClassifyOdds( testVec ), 148.4131591025766f, eps) );
  EXPECT_TRUE( Util::IsNear(classifier.ClassifyProbability( testVec ), 0.9933071490757152f, eps) );

  // Test infs and nans
  // Positive inf log-odds
  testVec[0] = std::numeric_limits<float>::infinity();
  testVec[1] = 0.0f;
  testVec[2] = 0.0f;
  EXPECT_EQ( classifier.ClassifyOdds( testVec ), std::numeric_limits<float>::infinity() );
  EXPECT_EQ( classifier.ClassifyLogOdds( testVec ), std::numeric_limits<float>::infinity() );
  EXPECT_EQ( classifier.ClassifyProbability( testVec ), 1.0 );

  // Negative inf log-odds
  testVec[0] = 0.0f;
  testVec[1] = 0.0f;
  testVec[2] = std::numeric_limits<float>::infinity();
  EXPECT_EQ( classifier.ClassifyOdds( testVec ), 0.0 );
  EXPECT_EQ( classifier.ClassifyLogOdds( testVec ), -std::numeric_limits<float>::infinity() );
  EXPECT_EQ( classifier.ClassifyProbability( testVec ), 0.0 );

  // NaN log-odds from sum of + and - inf
  testVec[0] = std::numeric_limits<float>::infinity();
  testVec[1] = 0.0f;
  testVec[2] = std::numeric_limits<float>::infinity();
  EXPECT_TRUE( std::isnan(classifier.ClassifyOdds( testVec )));
  EXPECT_TRUE( std::isnan(classifier.ClassifyLogOdds( testVec )));
  EXPECT_TRUE( std::isnan(classifier.ClassifyProbability( testVec )));
}