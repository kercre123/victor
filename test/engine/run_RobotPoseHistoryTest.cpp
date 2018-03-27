#include "gtest/gtest.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/shared/types.h"

#include "clad/types/proxMessages.h"

#include "engine/robot.h"

#define private public
#define protected public

#include "engine/robotStateHistory.h"

#define DIST_EQ_THRESH 0.00001
#define ANGLE_EQ_THRESH 0.00001

// Single origin for all the poses here to use, which will not destruct before anything that uses it
const Anki::Pose3d origin(0, Anki::Z_AXIS_3D(), {0,0,0}, "Origin");

const Anki::Cozmo::ProxSensorData proxSensorValid = { .distance_mm = 100,
                                                      .signalQuality = 10,
                                                      .isInValidRange = true,
                                                      .isValidSignalQuality = true,
                                                      .isLiftInFOV = false,
                                                      .isTooPitched = false };

const Anki::Cozmo::ProxSensorData proxSensorNotValid = { .distance_mm = 100,
                                                        .signalQuality = 10,
                                                        .isInValidRange = true,
                                                        .isValidSignalQuality = true,
                                                        .isLiftInFOV = true,
                                                        .isTooPitched = false };


const uint8_t noCliffDetectedFlags = 0;
const uint8_t frontCliffDetectedFlags = (1<<Anki::Util::EnumToUnderlying(Anki::Cozmo::CliffSensor::CLIFF_FL)) | 
                                        (1<<Anki::Util::EnumToUnderlying(Anki::Cozmo::CliffSensor::CLIFF_FR));

TEST(RobotStateHistory, AddGetPose)
{
  using namespace Anki;
  using namespace Cozmo;
  
  RobotStateHistory hist;
  HistRobotState histState;
  TimeStamp_t t;
  
  // Pose 1, 2, and 3
  const Pose3d p1(0, Z_AXIS_3D(), Vec3f(0,0,0), origin );
  const Pose3d p2(0.1f, Z_AXIS_3D(), Vec3f(1,1,2), origin );
  const Pose3d p3(-0.5, Z_AXIS_3D(), Vec3f(-2,-2,-3), origin );
  const Pose3d p1p2avg(0.05f, Z_AXIS_3D(), Vec3f(0.5, 0.5, 1) , origin);
  
  RobotState state1(Robot::GetDefaultRobotState());
  RobotState state2(Robot::GetDefaultRobotState());
  RobotState state3(Robot::GetDefaultRobotState());
  
  state1.headAngle = 0;
  state2.headAngle = 0.2f;
  state3.headAngle = -0.3f;
  
  state1.liftAngle = 0;
  state2.liftAngle = 0.5f;
  state2.liftAngle = 0.7f;
  
  state1.cliffDataRaw.fill(800);
  state2.cliffDataRaw.fill(800);
  state3.cliffDataRaw.fill(800);
  
  const TimeStamp_t t1 = 0;
  const TimeStamp_t t2 = 10;
  const TimeStamp_t t3 = 1005;
  
  state1.status &= !Util::EnumToUnderlying(RobotStatusFlag::IS_CARRYING_BLOCK);
  state2.status &=  Util::EnumToUnderlying(RobotStatusFlag::IS_CARRYING_BLOCK);
  state3.status &= !Util::EnumToUnderlying(RobotStatusFlag::IS_CARRYING_BLOCK);
  
  auto WasStateCarrying = [](const RobotState& state) -> bool {
    return (state.status & Util::EnumToUnderlying(RobotStatusFlag::IS_CARRYING_BLOCK));
  };
  
  hist.SetTimeWindow(1000);
  
  // Get pose from empty history
  
  ASSERT_TRUE( hist.ComputeStateAt(t1, t, histState) == RESULT_FAIL );
  
  
  // Add and get one pose
  hist.AddRawOdomState(t1, HistRobotState(p1, state1, proxSensorNotValid, noCliffDetectedFlags));
  
  ASSERT_TRUE(hist.GetNumRawStates() == 1);
  ASSERT_TRUE(hist.ComputeStateAt(t1, t, histState) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == histState.GetPose());
  ASSERT_TRUE(state1.headAngle == histState.GetHeadAngle_rad());
  ASSERT_TRUE(state1.liftAngle == histState.GetLiftAngle_rad());
  ASSERT_TRUE(WasStateCarrying(state1) == histState.WasCarryingObject());
  
  
  // Add another pose
  HistRobotState histState2(p2, state2, proxSensorValid, frontCliffDetectedFlags);
  hist.AddRawOdomState(t2, histState2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.GetNumRawStates() == 2);
  ASSERT_TRUE(hist.ComputeStateAt(t3, t, histState) == RESULT_FAIL);
  
  // Request in range pose
  ASSERT_TRUE(hist.ComputeStateAt(4, t, histState) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == histState.GetPose());
  
  ASSERT_TRUE(hist.ComputeStateAt(6, t, histState) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2.IsSameAs(histState.GetPose(), 1e-5f, DEG_TO_RAD(0.1f)));
  
  // Request in range pose with interpolation
  ASSERT_TRUE(hist.ComputeStateAt(5, t, histState, true) == RESULT_OK);
  ASSERT_TRUE(p1p2avg.IsSameAs(histState.GetPose(), 0.0001f, 0.0001f));
  
  // since interpolation is in the middle it should be the newest
  ASSERT_TRUE(histState.WasCarryingObject() == WasStateCarrying(state2));
  ASSERT_TRUE(histState.WasProxSensorValid() == histState2.WasProxSensorValid());
  for (int i=0; i<Util::EnumToUnderlying(CliffSensor::CLIFF_COUNT); ++i) {
    CliffSensor sensor = static_cast<CliffSensor>(i);
    ASSERT_TRUE(histState.WasCliffDetected(sensor) == histState2.WasCliffDetected(sensor));
  }

  // Add new pose that should bump off oldest pose
  hist.AddRawOdomState(t3, HistRobotState(p3, state3, proxSensorValid, noCliffDetectedFlags));
  
  ASSERT_TRUE(hist.GetNumRawStates() == 2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.ComputeStateAt(9, t, histState) == RESULT_FAIL);
  
  // This should return p2
  ASSERT_TRUE(hist.ComputeStateAt(11, t, histState) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2.IsSameAs(histState.GetPose(), 1e-5f, DEG_TO_RAD(0.1f)));  
  
  // Add old pose that is out of time window
  hist.AddRawOdomState(t1, HistRobotState(p1, state1, proxSensorValid, noCliffDetectedFlags));
  
  ASSERT_TRUE(hist.GetNumRawStates() == 2);
  ASSERT_TRUE(hist.GetOldestTimeStamp() == t2);
  ASSERT_TRUE(hist.GetNewestTimeStamp() == t3);
  
  
  // Clear history
  hist.Clear();
  ASSERT_TRUE(hist.GetNumRawStates() == 0);
}


TEST(RobotStateHistory, GroundTruthPose)
{
  
  using namespace Anki;
  using namespace Cozmo;
  
  RobotStateHistory hist;
  HistRobotState histState;
  TimeStamp_t t;
  
  PoseFrameID_t frameID = 0;
  
  // Pose 1, 2, and 3
  const Pose3d p1(0.25*M_PI_F, Z_AXIS_3D(), Vec3f(1,0,0), origin );
  const Pose3d p2(M_PI_2_F, Z_AXIS_3D(), Vec3f(1,2,0), origin );
  const Pose3d p3(M_PI_2_F - 0.25*M_PI_F, Z_AXIS_3D(), Vec3f(1 - sqrtf(2),2,0), origin );
  Pose3d p1_by_p2Top3( p3 ); // Start by copying p3 so end result keeps origin
  p1_by_p2Top3 *= p2.GetInverse();
  p1_by_p2Top3 *= p1;
  
  const f32 h1 = 0;
  const f32 h2 = 0.2f;
  const f32 h3 = -0.3f;
  const f32 l1 = 0;
  const f32 l2 = 0.5f;
  const f32 l3 = 0.7f;
  const TimeStamp_t t1 = 0;
  const TimeStamp_t t2 = 10;
  const TimeStamp_t t3 = 20;
  
  hist.SetTimeWindow(1000);
  
  // Add all three poses
  histState.SetPose(frameID, p1, h1, l1);
  hist.AddRawOdomState(t1, histState);

  histState.SetPose(frameID, p2, h2, l2);
  hist.AddRawOdomState(t2, histState);
  
  histState.SetPose(frameID, p3, h3, l3);
  hist.AddRawOdomState(t3, histState);
  
  ASSERT_TRUE(hist.GetNumRawStates() == 3);

  // 1) Add ground truth pose equivalent to p1 at same time t1
  histState.SetPose(frameID, p1, h1, l1);
  ASSERT_TRUE(hist.AddVisionOnlyState(t1, histState) == RESULT_OK);
  ASSERT_TRUE(hist.GetNumVisionStates() == 1);
 
  // Requested pose at t3 should be the same as p3
  ASSERT_TRUE(hist.ComputeStateAt(t3, t, histState) == RESULT_OK);
  /*
  printf("Pose p:\n");
  p.GetPose().Print();
  
  printf("Pose p3:\n");
  p3.Print();
  */
  ASSERT_TRUE(histState.GetPose().IsSameAs(p3, DIST_EQ_THRESH, ANGLE_EQ_THRESH) );

  
  // 2) Adding ground truth pose equivalent to p1 at time t2
  histState.SetPose(frameID, p1, h1, l1);
  hist.AddVisionOnlyState(t2, histState);
  
  // Since the frame ID of the ground truth pose is the same the frame of the
  // raw pose at t3, we expect to get back the raw pose at t3.
  ASSERT_TRUE(hist.ComputeStateAt(t3, t, histState) == RESULT_OK);
  /*
  printf("Pose p:\n");
  p.GetPose().Print();
  
  printf("Pose p1_by_p2Top3:\n");
  p1_by_p2Top3.Print();
  */
  ASSERT_TRUE(histState.GetPose().IsSameAs(p3, DIST_EQ_THRESH, ANGLE_EQ_THRESH));
  
  // 3) Now inserting the same ground truth pose again but with a higher frame id
  histState.SetPose(frameID+1, p1, h1, l1);
  hist.AddVisionOnlyState(t2, histState);

  // Requested pose at t3 should be pose p1 modified by the pose diff between p2 and p3
  ASSERT_TRUE(hist.ComputeStateAt(t3, t, histState) == RESULT_OK);
  
  ASSERT_TRUE(histState.GetPose().IsSameAs(p1_by_p2Top3, DIST_EQ_THRESH, ANGLE_EQ_THRESH));
  
  
  // 4) Check that there are no computed poses in history
  HistRobotState *hrs = nullptr;
  ASSERT_TRUE(hist.GetComputedStateAt(t3, &hrs) == RESULT_FAIL);
  
  // Compute pose at t3 again but this time insert it as well
  ASSERT_TRUE(hist.ComputeAndInsertStateAt(t3, t, &hrs) == RESULT_OK);
  ASSERT_TRUE(hrs != nullptr);
  
  // Get the computed pose.
  // Should be the exact same as rps.
  HistRobotState *hrs2 = nullptr;
  ASSERT_TRUE(hist.GetComputedStateAt(t3, &hrs2) == RESULT_OK);
  ASSERT_TRUE(hrs == hrs2);
  
  // 5) Get latest vision only pose
  ASSERT_TRUE(hist.GetLatestVisionOnlyState(t, histState) == RESULT_OK);
  ASSERT_TRUE(histState.GetPose() == p1);
}

TEST(RobotStateHistory, CullToWindowSizeTest)
{
  using namespace Anki;
  using namespace Cozmo;
  
  RobotStateHistory hist;
  
  const Pose3d p(0, Z_AXIS_3D(), Vec3f(0,0,0), origin );
  RobotState state(Robot::GetDefaultRobotState());
  HistRobotState histState(p, state, proxSensorValid, noCliffDetectedFlags);

  // Verify that culling on empty history doesn't cause a crash
  hist.CullToWindowSize();  // Keeps the latest 300ms and removes the rest

  // Fill history with 6 seconds
  for (TimeStamp_t t = 0; t < 6000; t += 100) {
    hist.AddRawOdomState(t, histState);
    
    // Don't add any visStates so as to test possible bad erase conditions in CullToWindowSize()
    
    if (t % 1000 == 0) {
      TimeStamp_t actualTime;
      HistRobotState *statePtr;
      hist.ComputeAndInsertStateAt(t, actualTime, &statePtr);
    }
  }

  printf("CullToWindowSizeTest: raw %zu, vis %zu, comp %zu, keyByTs %zu, tsByKey %zu\n",
         hist._states.size(),
         hist._visStates.size(),
         hist._computedStates.size(),
         hist._keyByTsMap.size(),
         hist._tsByKeyMap.size());

  
  // Verify that history stays at size no larger than 3s
  ASSERT_TRUE(hist._states.size() == 31);
  
  ASSERT_TRUE(hist._visStates.size() == 0);
  ASSERT_TRUE(hist._computedStates.size() == 3);
  ASSERT_TRUE(hist._keyByTsMap.size() == 3);
  ASSERT_TRUE(hist._tsByKeyMap.size() == 3);
  
}
