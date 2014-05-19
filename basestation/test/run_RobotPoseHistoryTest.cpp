#include "gtest/gtest.h"

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"

#define DIST_EQ_THRESH 0.00001
#define ANGLE_EQ_THRESH 0.00001

TEST(RobotPoseHistory, AddGetPose)
{
  using namespace Anki;
  using namespace Cozmo;

  Robot robot(1, nullptr, nullptr, nullptr);
  
  RobotPoseHistory hist;
  RobotPoseStamp p;
  TimeStamp_t t;
  
  PoseFrameId frameID = 0;
  
  // Pose 1, 2, and 3
  const Pose3d p1(0, Vec3f(0,0,1), Vec3f(0,0,0) );
  const Pose3d p2(0.1, Vec3f(0,0,1), Vec3f(1,1,2) );
  const Pose3d p3(-0.5, Vec3f(0,0,1), Vec3f(-2,-2,-3) );
  const Pose3d p1p2avg(0.05, Vec3f(0,0,1), Vec3f(0.5, 0.5, 1) );
  const f32 h1 = 0;
  const f32 h2 = 0.2;
  const f32 h3 = -0.3;
  const TimeStamp_t t1 = 0;
  const TimeStamp_t t2 = 10;
  const TimeStamp_t t3 = 1005;
  
  
  hist.SetTimeWindow(1000);
  
  
  // Get pose from empty history

  ASSERT_TRUE( hist.ComputePoseAt(t1, t, p) == RESULT_FAIL );
  

  // Add and get one pose
  hist.AddRawOdomPose(t1,
                      frameID,
                      p1.get_translation().x(), p1.get_translation().y(), p1.get_translation().z(),
                      p1.get_rotationAngle().ToFloat(),
                      h1);
  
  ASSERT_TRUE(hist.GetNumRawPoses() == 1);
  ASSERT_TRUE(hist.ComputePoseAt(t1, t, p) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == p.GetPose());
  ASSERT_TRUE(h1 == p.GetHeadAngle());
  
  // Add another pose
  hist.AddRawOdomPose(t2,
                      frameID,
                      p2.get_translation().x(), p2.get_translation().y(), p2.get_translation().z(),
                      p2.get_rotationAngle().ToFloat(),
                      h2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.GetNumRawPoses() == 2);
  ASSERT_TRUE(hist.ComputePoseAt(t3, t, p) == RESULT_FAIL);

  // Request in range pose
  ASSERT_TRUE(hist.ComputePoseAt(4, t, p) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == p.GetPose());

  ASSERT_TRUE(hist.ComputePoseAt(6, t, p) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2 == p.GetPose());
  
  // Request in range pose with interpolation
  ASSERT_TRUE(hist.ComputePoseAt(5, t, p, true) == RESULT_OK);
  ASSERT_TRUE(p1p2avg.IsSameAs(p.GetPose(), 0.0001, 0.0001));
  
  
  // Add new pose that should bump off oldest pose
  hist.AddRawOdomPose(t3,
                      frameID,
                      p3.get_translation().x(), p3.get_translation().y(), p3.get_translation().z(),
                      p3.get_rotationAngle().ToFloat(),
                      h3);
  
  ASSERT_TRUE(hist.GetNumRawPoses() == 2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.ComputePoseAt(9, t, p) == RESULT_FAIL);
  
  // This should return p2
  ASSERT_TRUE(hist.ComputePoseAt(11, t, p) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2 == p.GetPose());
  
  
  // Add old pose that is out of time window
  hist.AddRawOdomPose(t1,
                      frameID,
                      p1.get_translation().x(), p1.get_translation().y(), p1.get_translation().z(),
                      p1.get_rotationAngle().ToFloat(),
                      h1);
  
  ASSERT_TRUE(hist.GetNumRawPoses() == 2);
  ASSERT_TRUE(hist.GetOldestTimeStamp() == t2);
  ASSERT_TRUE(hist.GetNewestTimeStamp() == t3);
  
  
  // Clear history
  hist.Clear();
  ASSERT_TRUE(hist.GetNumRawPoses() == 0);
}


TEST(RobotPoseHistory, GroundTruthPose)
{
  
  using namespace Anki;
  using namespace Cozmo;
  
  Robot robot(1, nullptr, nullptr, nullptr);
  
  RobotPoseHistory hist;
  RobotPoseStamp p;
  TimeStamp_t t;
  
  PoseFrameId frameID = 0;
  
  // Pose 1, 2, and 3
  const Pose3d p1(0.25*PI_F, Vec3f(0,0,1), Vec3f(1,0,0) );
  const Pose3d p2(PIDIV2_F, Vec3f(0,0,1), Vec3f(1,2,0) );
  const Pose3d p3(PIDIV2_F - 0.25*PI_F, Vec3f(0,0,1), Vec3f(1 - sqrtf(2),2,0) );
  const Pose3d p1_by_p2Top3(0, Vec3f(0,0,1), Vec3f(0, 1, 0) );
  const f32 h1 = 0;
  const f32 h2 = 0.2;
  const f32 h3 = -0.3;
  const TimeStamp_t t1 = 0;
  const TimeStamp_t t2 = 10;
  const TimeStamp_t t3 = 20;
  
  hist.SetTimeWindow(1000);
  
  // Add all three poses
  p.SetPose(frameID, p1, h1);
  hist.AddRawOdomPose(t1, p);

  p.SetPose(frameID, p2, h2);
  hist.AddRawOdomPose(t2, p);
  
  p.SetPose(frameID, p3, h3);
  hist.AddRawOdomPose(t3, p);
  
  ASSERT_TRUE(hist.GetNumRawPoses() == 3);

  // 1) Add ground truth pose equivalent to p1 at same time t1
  p.SetPose(frameID, p1, h1);
  ASSERT_TRUE(hist.AddVisionOnlyPose(t1, p) == RESULT_OK);
  ASSERT_TRUE(hist.GetNumVisionPoses() == 1);
 
  // Requested pose at t3 should be the same as p3
  ASSERT_TRUE(hist.ComputePoseAt(t3, t, p) == RESULT_OK);
  /*
  printf("Pose p:\n");
  p.GetPose().Print();
  
  printf("Pose p3:\n");
  p3.Print();
  */
  ASSERT_TRUE(p.GetPose().IsSameAs(p3, DIST_EQ_THRESH, ANGLE_EQ_THRESH) );

  
  // 2) Adding ground truth pose equivalent to p1 at time t2
  p.SetPose(frameID, p1, h1);
  hist.AddVisionOnlyPose(t2, p);
  
  // Requested pose at t3 should be pose p1 modified by the pose diff between p2 and p3
  ASSERT_TRUE(hist.ComputePoseAt(t3, t, p) == RESULT_OK);
  /*
  printf("Pose p:\n");
  p.GetPose().Print();
  
  printf("Pose p1_by_p2Top3:\n");
  p1_by_p2Top3.Print();
  */
  ASSERT_TRUE(p.GetPose().IsSameAs(p1_by_p2Top3, DIST_EQ_THRESH, ANGLE_EQ_THRESH));
}

