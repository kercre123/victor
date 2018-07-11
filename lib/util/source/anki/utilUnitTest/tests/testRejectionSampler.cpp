/**
 * File: testRejectionSampler.cpp
 *
 * Author: ross
 * Created: Jun 29 2018
 *
 * Description: Unit tests for RejectionSamplerHelper
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=RejectionSamplerHelper.*
 **/


#include "util/helpers/includeGTest.h"
#include "util/random/rejectionSamplerHelper.h"
#include "util/random/randomGenerator.h"

using namespace Anki::Util;

TEST(RejectionSamplerHelper, Test)
{
  RandomGenerator rng(123);
  constexpr bool kExpectReordering = true;
  using Type = int;
  auto generator = [&rng](){ return rng.RandIntInRange(0,0); };
  RejectionSamplerHelper<Type, kExpectReordering> sampler;
  
  // do a quick test of the !kExpectReordering to ensure it compiles
  RejectionSamplerHelper<Type, !kExpectReordering> otherType;
  EXPECT_EQ( otherType.GetNumConditions(), 0);
  
  // without conditions, it accepts everything
  EXPECT_EQ( sampler.GetNumConditions(), 0 );
  EXPECT_TRUE( sampler.Evaluate(rng, 1) );
  EXPECT_TRUE( sampler.Evaluate(rng, -1) );
  
  std::vector<Type> noCondSamples = sampler.Generate( rng, 2, generator );
  ASSERT_EQ( noCondSamples.size(), 2 );
  EXPECT_EQ( noCondSamples[0], 0 );
  EXPECT_EQ( noCondSamples[1], 0 );
  
  auto rejectZeroHandle = sampler.AddCondition( [](const Type& sample){ return sample != 0; } );
  EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, 1 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, -1 ) );
  EXPECT_EQ( sampler.GetNumConditions(), 1 );
  
  // scoped condition
  {
    auto handle = sampler.AddScopedCondition( [](const Type& sample){ return sample != 1; } );
    EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
    EXPECT_FALSE( sampler.Evaluate( rng, 1 ) );
    EXPECT_TRUE( sampler.Evaluate( rng, -1 ) );
    EXPECT_EQ( sampler.GetNumConditions(), 2 );
  }
  EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, 1 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, -1 ) );
  EXPECT_EQ( sampler.GetNumConditions(), 1 );
  
  // a condition that always rejects a specific value
  class ConditionRejectOnly : public RejectionSamplingCondition<Type> {
  public:
    explicit ConditionRejectOnly(Type val) : _rejectOnly(val) {}
    virtual bool operator()(const Type& sample) {
      _called = true;
      return (sample != _rejectOnly);
    }
    Type _rejectOnly;
    bool _called = false;
  };
  
  // insert a rejection of 5 after the basic reject of 0
  auto rejectFiveHandle = sampler.AddCondition( std::make_shared<ConditionRejectOnly>(5) );
  EXPECT_FALSE( rejectFiveHandle->_called );
  EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
  EXPECT_FALSE( rejectFiveHandle->_called );
  EXPECT_FALSE( sampler.Evaluate( rng, 5 ) );
  EXPECT_TRUE( rejectFiveHandle->_called );
  EXPECT_TRUE( sampler.Evaluate( rng, 1 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, -1 ) );
  EXPECT_EQ( sampler.GetNumConditions(), 2 );
  
  rejectFiveHandle->_called = false;
  
  // scoped condition
  {
    // insert a rejection of 2 after the reject of 0 but before 5
    auto rejectTwoHandle = sampler.AddScopedCondition( std::make_shared<ConditionRejectOnly>(2), rejectFiveHandle );
    EXPECT_EQ( sampler.GetNumConditions(), 3 );
    EXPECT_FALSE( rejectTwoHandle->_called );
    EXPECT_FALSE( rejectFiveHandle->_called );
    EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
    EXPECT_FALSE( rejectTwoHandle->_called );
    EXPECT_FALSE( rejectFiveHandle->_called );
    EXPECT_FALSE( sampler.Evaluate( rng, 2 ) );
    EXPECT_TRUE( rejectTwoHandle->_called );
    EXPECT_FALSE( rejectFiveHandle->_called );
    rejectTwoHandle->_called = false;
    EXPECT_FALSE( sampler.Evaluate( rng, 5 ) );
    EXPECT_TRUE( rejectTwoHandle->_called );
    EXPECT_TRUE( rejectFiveHandle->_called );
    rejectTwoHandle->_called = false;
    rejectFiveHandle->_called = false;
    EXPECT_TRUE( sampler.Evaluate( rng, 1 ) );
    EXPECT_TRUE( rejectTwoHandle->_called );
    EXPECT_TRUE( rejectFiveHandle->_called );
    rejectFiveHandle->_called = false;
  }
  
  // without the 2 rejector, it should get to 5 and pass
  EXPECT_TRUE( sampler.Evaluate( rng, 2 ) );
  EXPECT_TRUE( rejectFiveHandle->_called );
  EXPECT_EQ( sampler.GetNumConditions(), 2 );
  
  // changing the rejection param to 2 should fail 2
  rejectFiveHandle->_called = false;
  rejectFiveHandle->_rejectOnly = 2;
  EXPECT_FALSE( sampler.Evaluate( rng, 2 ) );
  EXPECT_TRUE( rejectFiveHandle->_called );
  
  // removing the 5-rejector, only 0 should be rejected
  sampler.RemoveCondition( rejectFiveHandle );
  EXPECT_EQ( sampler.GetNumConditions(), 1 );
  EXPECT_TRUE( sampler.Evaluate( rng, 2 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, 5 ) );
  EXPECT_FALSE( sampler.Evaluate( rng, 0 ) );
  
  // removing the last condition should pass everything
  sampler.ClearConditions();
  EXPECT_EQ( sampler.GetNumConditions(), 0 );
  EXPECT_TRUE( sampler.Evaluate( rng, 0 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, 2 ) );
  EXPECT_TRUE( sampler.Evaluate( rng, 5 ) );
  
}

