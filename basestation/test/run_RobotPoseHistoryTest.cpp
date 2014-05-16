#include "gtest/gtest.h"

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/robotPoseHistory.h"


TEST(RobotPoseHistory, AddGetPose)
{
  using namespace Anki;
  using namespace Cozmo;

  RobotPoseHistory hist;
  RobotPoseStamp p;
  TimeStamp_t t;
  
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

  ASSERT_TRUE( hist.GetPoseAt(t1, t, p) == RESULT_FAIL );
  

  // Add and get one pose
  hist.AddPose(t1,
               p1.get_translation().x(), p1.get_translation().y(), p1.get_translation().z(),
               p1.get_rotationAngle().ToFloat(),
               h1);
  
  ASSERT_TRUE(hist.Size() == 1);
  ASSERT_TRUE(hist.GetPoseAt(t1, t, p) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == p.GetPose());
  ASSERT_TRUE(h1 == p.GetHeadAngle());
  
  // Add another pose
  hist.AddPose(t2,
               p2.get_translation().x(), p2.get_translation().y(), p2.get_translation().z(),
               p2.get_rotationAngle().ToFloat(),
               h2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.Size() == 2);
  ASSERT_TRUE(hist.GetPoseAt(t3, t, p) == RESULT_FAIL);

  // Request in range pose
  ASSERT_TRUE(hist.GetPoseAt(4, t, p) == RESULT_OK);
  ASSERT_TRUE(t1 == t);
  ASSERT_TRUE(p1 == p.GetPose());

  ASSERT_TRUE(hist.GetPoseAt(6, t, p) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2 == p.GetPose());
  
  // Request in range pose with interpolation
  ASSERT_TRUE(hist.GetPoseAt(5, t, p, true) == RESULT_OK);
  ASSERT_TRUE(p1p2avg.IsSameAs(p.GetPose(), 0.0001, 0.0001));
  
  
  // Add new pose that should bump off oldest pose
  hist.AddPose(t3,
               p3.get_translation().x(), p3.get_translation().y(), p3.get_translation().z(),
               p3.get_rotationAngle().ToFloat(),
               h3);
  
  ASSERT_TRUE(hist.Size() == 2);
  
  // Request out of range pose
  ASSERT_TRUE(hist.GetPoseAt(9, t, p) == RESULT_FAIL);
  
  // This should return p2
  ASSERT_TRUE(hist.GetPoseAt(11, t, p) == RESULT_OK);
  ASSERT_TRUE(t2 == t);
  ASSERT_TRUE(p2 == p.GetPose());
  
  
  // Add old pose that is out of time window
  hist.AddPose(t1,
               p1.get_translation().x(), p1.get_translation().y(), p1.get_translation().z(),
               p1.get_rotationAngle().ToFloat(),
               h1);
  
  ASSERT_TRUE(hist.Size() == 2);
  ASSERT_TRUE(hist.GetOldestTimeStamp() == t2);
  ASSERT_TRUE(hist.GetNewestTimeStamp() == t3);
  
  
  // Clear history
  hist.Clear();
  ASSERT_TRUE(hist.Size() == 0);
}


TEST(RobotPoseHistory, GroundTruthPose)
{
  // Add a few poses f
  
  
}

