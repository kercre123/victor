#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include "util/logging/logging.h"
#include "util/math/math.h"

#include "coretech/planning/shared/path.h"


using namespace std;
using namespace Anki::Planning;

// TODO: Add more path tests!

GTEST_TEST(TestPath, GetDistToArcSegment)
{
  // Define a CCW circular path
  Path p;
  const float pathRadius_mm = 100;
  const float targetSpeed = 100;
  const float accel = 100;
  const float decel = 100;
  const float x_center_mm = 10;
  const float y_center_mm = 20;
  p.AppendArc(x_center_mm, y_center_mm, pathRadius_mm, 0, M_PI_F, targetSpeed, accel, decel);
  p.AppendArc(x_center_mm, y_center_mm, pathRadius_mm, M_PI_F, M_PI_F, targetSpeed, accel, decel);

  ASSERT_TRUE(p.CheckContinuity(1));
  
  const float kShortestDistTol_mm = 0.01f;
  const float kAngleDiffTol_rad = DEG_TO_RAD(0.01f);
  const float kAngularStep_rad = DEG_TO_RAD(0.2f);
  
  const float kRadialOffset_mm = 10.f;
  const float kOutsideRadius_mm = pathRadius_mm + kRadialOffset_mm;
  const float kInsideRadius_mm = pathRadius_mm - kRadialOffset_mm;
  
  float x, y;
  float shortestDistToPath_mm;
  float angDiff_rad;
  float distToEndOfPath_mm;
  
  // Get distance to arc segment while...
  
  // 1) moving CCW along the outside of the circle
  int currSegment = 0;
  for (float angleWrtCenter_rad = 0.f; angleWrtCenter_rad < 2*M_PI_F; angleWrtCenter_rad += kAngularStep_rad) {
    x = cosf(angleWrtCenter_rad) * kOutsideRadius_mm + x_center_mm;
    y = sinf(angleWrtCenter_rad) * kOutsideRadius_mm + y_center_mm;
    
    auto segment = p.GetSegmentConstRef(currSegment);
    auto status = segment.GetDistToSegment(x, y, angleWrtCenter_rad + M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);

    // If end of segment, go to the next one
    if (status == SegmentRangeStatus::OOR_NEAR_END) {
      EXPECT_TRUE( NEAR(angleWrtCenter_rad, M_PI_F, kAngularStep_rad) );
      EXPECT_TRUE( distToEndOfPath_mm <= 0);
      segment = p.GetSegmentConstRef(++currSegment);
      status = segment.GetDistToSegment(x, y, angleWrtCenter_rad + M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);
    }
    
    EXPECT_TRUE( NEAR(shortestDistToPath_mm, -kRadialOffset_mm, kShortestDistTol_mm) );
    EXPECT_TRUE( NEAR(angDiff_rad, 0.f, kAngleDiffTol_rad) );
    EXPECT_TRUE( status == SegmentRangeStatus::IN_SEGMENT_RANGE);
    EXPECT_TRUE( distToEndOfPath_mm >= 0);
  }

  // 2) moving CCW along the inside of the circle
  currSegment = 0;
  for (float angleWrtCenter_rad = 0.f; angleWrtCenter_rad < 2*M_PI_F; angleWrtCenter_rad += kAngularStep_rad) {
    x = cosf(angleWrtCenter_rad) * kInsideRadius_mm + x_center_mm;
    y = sinf(angleWrtCenter_rad) * kInsideRadius_mm + y_center_mm;
    
    auto segment = p.GetSegmentConstRef(currSegment);
    auto status = segment.GetDistToSegment(x, y, angleWrtCenter_rad + M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);

    if (status == SegmentRangeStatus::OOR_NEAR_END) {
      EXPECT_TRUE( NEAR(angleWrtCenter_rad, M_PI_F, kAngularStep_rad) );
      EXPECT_TRUE( distToEndOfPath_mm <= 0);
      segment = p.GetSegmentConstRef(++currSegment);
      status = segment.GetDistToSegment(x, y, angleWrtCenter_rad + M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);
    }
    
    EXPECT_TRUE( NEAR(shortestDistToPath_mm, kRadialOffset_mm, kShortestDistTol_mm) );
    EXPECT_TRUE( NEAR(angDiff_rad, 0.f, kAngleDiffTol_rad) );
    EXPECT_TRUE( status == SegmentRangeStatus::IN_SEGMENT_RANGE);
    EXPECT_TRUE( distToEndOfPath_mm >= 0);
  }

  
  // Define a CW circular path
  p.Clear();
  p.AppendArc(x_center_mm, y_center_mm, pathRadius_mm, 0, -M_PI_F, targetSpeed, accel, decel);
  p.AppendArc(x_center_mm, y_center_mm, pathRadius_mm, -M_PI_F, -M_PI_F, targetSpeed, accel, decel);
  
  ASSERT_TRUE(p.CheckContinuity(1));

  
  // 3) moving CW along the outside of the circle
  currSegment = 0;
  for (float angleWrtCenter_rad = 0.f; angleWrtCenter_rad > -2*M_PI_F; angleWrtCenter_rad -= kAngularStep_rad) {
    x = cosf(angleWrtCenter_rad) * kOutsideRadius_mm + x_center_mm;
    y = sinf(angleWrtCenter_rad) * kOutsideRadius_mm + y_center_mm;
    
    auto segment = p.GetSegmentConstRef(currSegment);
    auto status = segment.GetDistToSegment(x, y, angleWrtCenter_rad - M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);
    
    if (status == SegmentRangeStatus::OOR_NEAR_END) {
      EXPECT_TRUE( NEAR(angleWrtCenter_rad, -M_PI_F, kAngularStep_rad) );
      EXPECT_TRUE( distToEndOfPath_mm <= 0);
      segment = p.GetSegmentConstRef(++currSegment);
      status = segment.GetDistToSegment(x, y, angleWrtCenter_rad - M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);
    }
    
    EXPECT_TRUE( NEAR(shortestDistToPath_mm, kRadialOffset_mm, kShortestDistTol_mm) );
    EXPECT_TRUE( NEAR(angDiff_rad, 0.f, kAngleDiffTol_rad) );
    EXPECT_TRUE( status == SegmentRangeStatus::IN_SEGMENT_RANGE);
    EXPECT_TRUE( distToEndOfPath_mm >= 0);
  }
  
  // 4) moving CW along the inside of the circle
  currSegment = 0;
  for (float angleWrtCenter_rad = 0.f; angleWrtCenter_rad > -2*M_PI_F; angleWrtCenter_rad -= kAngularStep_rad) {
    x = cosf(angleWrtCenter_rad) * kInsideRadius_mm + x_center_mm;
    y = sinf(angleWrtCenter_rad) * kInsideRadius_mm + y_center_mm;
    
    auto segment = p.GetSegmentConstRef(currSegment);
    auto status = segment.GetDistToSegment(x, y, angleWrtCenter_rad - M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);

    if (status == SegmentRangeStatus::OOR_NEAR_END) {
      EXPECT_TRUE( NEAR(angleWrtCenter_rad, -M_PI_F, kAngularStep_rad) );
      EXPECT_TRUE( distToEndOfPath_mm <= 0);
      segment = p.GetSegmentConstRef(++currSegment);
      status = segment.GetDistToSegment(x, y, angleWrtCenter_rad - M_PI_2_F, shortestDistToPath_mm, angDiff_rad, &distToEndOfPath_mm);
    }
    
    EXPECT_TRUE( NEAR(shortestDistToPath_mm, -kRadialOffset_mm, kShortestDistTol_mm) );
    EXPECT_TRUE( NEAR(angDiff_rad, 0.f, kAngleDiffTol_rad) );
    EXPECT_TRUE( status == SegmentRangeStatus::IN_SEGMENT_RANGE);
    EXPECT_TRUE( distToEndOfPath_mm >= 0);
  }
}


