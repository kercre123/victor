/**
 * File: testFilters
 *
 * Author: MattMichini
 * Created: 02/28/2018
 *
 * Description: Unit tests for filters
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "util/helpers/includeGTest.h"

#include "util/filters/lowPassFilterSimple.h"

#include <vector>


class TestLowPassFilterSimple : public ::testing::Test {
public:

  // Create a step input (0 for the first sample, 1 after that) and ensure the filtered
  // value rises in the expected way.
  void RampZeroToOne(Anki::Util::LowPassFilterSimple& filter, int nSamples) {

    float samplePeriod_sec = filter.GetSamplePeriod_sec();
    float timeConstant_sec = filter.GetTimeConstant_sec();

    // Add 0 as first sample:
    filter.Reset();
    filter.AddSample(0.f);

    // Keep track of previous filtered value
    float prevFilteredVal = filter.GetFilteredValue();
    EXPECT_FLOAT_EQ(0.f, prevFilteredVal);
    
    for (int i=1 ; i<nSamples ; i++) {
      filter.AddSample(1.f);
      
      const float currFilteredVal = filter.GetFilteredValue();
      
      // Filtered value should always be between 0 and 1
      EXPECT_GE(currFilteredVal, 0.f);
      EXPECT_LE(currFilteredVal, 1.f);
      
      // Filtered value should be strictly increasing
      EXPECT_GT(currFilteredVal, prevFilteredVal);
      
      // Filtered value should reach ~63.2% of the step value by the time
      // t reaches the desired time constant, and should be higher thereafter.
      // Include small fudge factor.
      const float currTime_sec = i * samplePeriod_sec;
      if (currTime_sec < timeConstant_sec) {
        EXPECT_LT(currFilteredVal, 0.64f);
      } else {
        EXPECT_GT(currFilteredVal, 0.63f);
      }
      
      // Update previous filtered value
      prevFilteredVal = currFilteredVal;
    }
  }

};

// --gtest_filter=TestLowPassFilterSimple.TestConstantInput
TEST_F(TestLowPassFilterSimple, TestConstantInput)
{
  const float kSamplePeriod_sec = 0.1f;
  const float kTimeConstant_sec = 1.f;
  Anki::Util::LowPassFilterSimple filter(kSamplePeriod_sec, kTimeConstant_sec);
  
  // Test the filter with several constant values, and reset in between.
  const std::vector<float> constantTestVals{-1.f, 0.f, 1.f};

  for (const auto testVal : constantTestVals) {
    filter.Reset();
    const int nSamples = 100;
    for (int i=0 ; i<nSamples ; i++) {
      filter.AddSample(testVal);
      // The filtered value should always be equal to the constant input value
      EXPECT_FLOAT_EQ(testVal, filter.GetFilteredValue());
    }
  }
}



// --gtest_filter=TestLowPassFilterSimple.TestStepResponse
TEST_F(TestLowPassFilterSimple, TestStepResponse)
{
  const float kSamplePeriod_sec = 0.01f;
  const float kTimeConstant_sec = 1.f;
  Anki::Util::LowPassFilterSimple filter(kSamplePeriod_sec, kTimeConstant_sec);
  
  const int nSamples = 500;
  RampZeroToOne(filter, nSamples);
}


// --gtest_filter=TestLowPassFilterSimple.TestImpulseResponse
TEST_F(TestLowPassFilterSimple, TestImpulseResponse)
{
  const float kSamplePeriod_sec = 0.01f;
  const float kTimeConstant_sec = 0.05f;
  Anki::Util::LowPassFilterSimple filter(kSamplePeriod_sec, kTimeConstant_sec);
  
  const int nSamples = 100;
  
  for (int i=0 ; i<nSamples ; i++) {
    // Create an impulse input (zero first, then a single 1, then all zeros):
    if (i==1) {
      filter.AddSample(1.f);
    } else {
      filter.AddSample(0.f);
    }
    
    const float currFilteredVal = filter.GetFilteredValue();
    static float prevFilteredVal = currFilteredVal;
    
    // Filtered value should always be between 0 and 1
    EXPECT_GE(currFilteredVal, 0.f);
    EXPECT_LE(currFilteredVal, 1.f);
    
    // After second sample, filtered value should be nonzero
    // and strictly decreasing.
    if (i >= 2) {
      EXPECT_GT(currFilteredVal, 0.f);
      EXPECT_LT(currFilteredVal, prevFilteredVal);
    }
    
    // Update previous filtered value
    prevFilteredVal = currFilteredVal;
  }
}

// --gtest_filter=TestLowPassFilterSimple.TestFilterParamChanges
TEST_F(TestLowPassFilterSimple, TestFilterParamChanges)
{
  const float kInitialSamplePeriod_sec = 0.1f;
  const float kInitialTimeConstant_sec = 1.f;
  const float kSamplePeriod_sec = 0.01f;
  const float kTimeConstant_sec = 2.f;
  const int nSamples = 500;

  // Construct filter with one sample period, but immediately change it just to test
  // that we can.
  Anki::Util::LowPassFilterSimple filter(kInitialSamplePeriod_sec, kInitialTimeConstant_sec);
  filter.SetSamplePeriod(kSamplePeriod_sec);
  RampZeroToOne(filter, nSamples);

  // Now change the time constant and run the test again
  filter.SetTimeConstant(kTimeConstant_sec);
  RampZeroToOne(filter, nSamples);
}