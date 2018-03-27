#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/shared/radiansMath.h"

#include <iostream>
#include <vector>

#define TOLERANCE_RAD 0.0002f
#define EXPECT_NEAR_EQ(a,b) EXPECT_NEAR(a,b,TOLERANCE_RAD)


using namespace std;
using namespace Anki;

GTEST_TEST(TestRadiansMath, ComputeMinMaxMean)
{
  // Convert vector of degrees to vector of radians
  auto deg2rad = [](std::vector<float> angles_deg) -> std::vector<Radians> {
    std::vector<Radians> angles_rad;
    for (const auto angle : angles_deg) {
      angles_rad.push_back(DEG_TO_RAD(angle));
    }
    return angles_rad;
  };
  
  Radians meanAngle;
  float distToMin;
  float distToMax;
  bool res;
  
  // Empty
  std::vector<Radians> angles;
  res = RadiansMath::ComputeMinMaxMean(angles, meanAngle, distToMin, distToMax);
  EXPECT_FALSE(res);
  
  // Case 1
  angles = deg2rad({ -8, -2, 4, 10 });
  res = RadiansMath::ComputeMinMaxMean(angles, meanAngle, distToMin, distToMax);
  EXPECT_TRUE(res);
  EXPECT_NEAR_EQ(meanAngle.ToFloat(), DEG_TO_RAD(1.f));
  EXPECT_NEAR_EQ(distToMin, DEG_TO_RAD(-9.f));
  EXPECT_NEAR_EQ(distToMax, DEG_TO_RAD(9.f));
  
  
  // Case 2
  angles = deg2rad({ -175, 173, 173 });
  res = RadiansMath::ComputeMinMaxMean(angles, meanAngle, distToMin, distToMax);
  EXPECT_TRUE(res);
  EXPECT_NEAR_EQ(meanAngle.ToFloat(), DEG_TO_RAD(177.f));
  EXPECT_NEAR_EQ(distToMin, DEG_TO_RAD(-4.f));
  EXPECT_NEAR_EQ(distToMax, DEG_TO_RAD(8.f));
  
  
} // TestRadiansMath::ComputeMinMaxMean





