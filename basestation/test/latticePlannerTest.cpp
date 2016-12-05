/**
 * File: latticePlannerTest.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-10-05
 *
 * Description: Tests for the lattice planner engine-side object, including threading
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "json/json.h"
#include "util/helpers/includeGTest.h"
#include <chrono>
#include <thread>

#define private public
#define protected public

#include "anki/common/types.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"

using namespace Anki;
using namespace Cozmo;

extern Anki::Cozmo::CozmoContext* cozmoContext;

// Motion profile for test
const f32 defaultPathSpeed_mmps = 60;
const f32 defaultPathAccel_mmps2 = 200;
const f32 defaultPathDecel_mmps2 = 500;
const f32 defaultPathPointTurnSpeed_rad_per_sec = 2;
const f32 defaultPathPointTurnAccel_rad_per_sec2 = 100;
const f32 defaultPathPointTurnDecel_rad_per_sec2 = 500;
const f32 defaultDockSpeed_mmps = 60;
const f32 defaultDockAccel_mmps2 = 200;
const f32 defaultDockDecel_mmps2 = 100;
const f32 defaultReverseSpeed_mmps = 30;
PathMotionProfile defaultMotionProfile(defaultPathSpeed_mmps,
                                       defaultPathAccel_mmps2,
                                       defaultPathDecel_mmps2,
                                       defaultPathPointTurnSpeed_rad_per_sec,
                                       defaultPathPointTurnAccel_rad_per_sec2,
                                       defaultPathPointTurnDecel_rad_per_sec2,
                                       defaultDockSpeed_mmps,
                                       defaultDockAccel_mmps2,
                                       defaultDockDecel_mmps2,
                                       defaultReverseSpeed_mmps,
                                       true);

TEST(LatticePlanner, Create)
{
  Robot robot(1, cozmoContext);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  EXPECT_EQ(planner->CheckPlanningStatus(), EPlannerStatus::Error);

  // planner will be deleted in ~Robot
}

namespace {

// "busy sleep" while suggesting that other threads run 
// for a small amount of time
void little_sleep(std::chrono::microseconds us)
{
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + us;
  do {
    std::this_thread::yield();
  } while (std::chrono::high_resolution_clock::now() < end);
}


void ExpectPlanComplete(int maxTimeMs, LatticePlanner* planner, int minPlanTimeMs = 2)
{
  bool done = false;
  int ms = 0;

  for(ms = 0; ms < maxTimeMs; ms++) {
    little_sleep(std::chrono::milliseconds(1));
    switch( planner->CheckPlanningStatus() ) {
      case EPlannerStatus::Error:
        FAIL() << "planner returned error after "<<ms<<" milliseconds";
        break;

      case EPlannerStatus::Running: break;
      case EPlannerStatus::CompleteWithPlan:
        done = true;
        break;

      case EPlannerStatus::CompleteNoPlan:
        FAIL() << "planner finished with no plan! Should have plan";
        break;
      
    }

    if(done)
      break;
  }

  ASSERT_TRUE(done) << "planner didn't finish";

  std::cout<<"planner took "<<ms<<"ms\n";
  
  EXPECT_GE(ms, minPlanTimeMs) << "the planner might be fast, but not that fast!";
  EXPECT_LT(ms, maxTimeMs) << "shouldn't use all the available time";

  printf("planner finished after %dms\n", ms);
}

}

TEST(LatticePlanner, PlanOnceEmpty)
{
  Robot robot(1, cozmoContext);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(100, planner, 0);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path"; 
}

TEST(LatticePlanner, PlanTwiceEmpty)
{
  Robot robot(1, cozmoContext);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(250, planner, 0);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  int firstPathLength = path.GetNumSegments();

  EXPECT_GT(firstPathLength, 1) << "should be more than one action in the path";

  Pose3d start2(0, Z_AXIS_3D(), Vec3f(17,-15,0) );
  Pose3d goal2(0, Z_AXIS_3D(), Vec3f(2000,10,0) );

  ret = planner->ComputePath(start2, goal2);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(100, planner, 0);

  selectedTargetIdx = 200;

  hasPath = planner->GetCompletePath(start2, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  int secondPathLength = path.GetNumSegments();

  EXPECT_GT(secondPathLength, 1) << "should be more than one action in the path";

  // NOTE: just checking length because I'm lazy. If the motion primitives change, this test could break. If
  // so, just change start2 or goal2 to see if that fixes it
  EXPECT_NE(firstPathLength, secondPathLength) << "should have a different path the second time";
}

// This test is disabled because it relies on timing, which won't always work on e.g. the build server
TEST(LatticePlanner, DISABLED_PlanWhilePlanning)
{
  Robot robot(1, cozmoContext);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,1000,0) );

  EComputePathStatus ret1 = planner->ComputePath(start, goal);
  EComputePathStatus ret2 = planner->ComputePath(start, goal);

  EXPECT_EQ(ret1, EComputePathStatus::Running);
  EXPECT_EQ(ret2, EComputePathStatus::Error);

  // should still be able to finish he original plan

  ExpectPlanComplete(1000, planner);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path"; 
}


TEST(LatticePlanner, StopPlanning)
{

  // NOTE: this test could start failing if the planner gets too fast, and finished before the Stop can go
  // through. If this happens, just make it plan a further distance, or add some complex obstacles to the
  // environment to slow it down
  
  Robot robot(1, cozmoContext);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(200,1000,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  bool done = false;
  bool calledStop = false;
  int maxTimeMs = 100;
  int ms;
  for(ms = 0; ms < maxTimeMs; ms++) {
    little_sleep(std::chrono::milliseconds(1));
    switch( planner->CheckPlanningStatus() ) {
      case EPlannerStatus::Error:
        printf("%d: err\n", ms);
        done = true;
        break;

      case EPlannerStatus::Running:
        printf("%d: running\n", ms);
        if(ms ==  4) {
          planner->StopPlanning();
          printf("%d: CALLING STOP\n", ms);
          calledStop = true;
        }

        break;

      case EPlannerStatus::CompleteWithPlan:
        printf("%d: CompleteWithPlan\n", ms);
        FAIL() << "planner finished with a plan! Should have error";
        break;

      case EPlannerStatus::CompleteNoPlan:
        printf("%d: CompleteNoPlan\n", ms);
        FAIL() << "planner finished with no plan! Should have error";
        break;
    }

    if(done)
      break;
  }

  EXPECT_GT(ms, 3) << "planner should have returned 'running' for at least a few ms";

  ASSERT_TRUE(calledStop) << "planner finished too quickly, maybe it is too fast on this problem?";
  ASSERT_TRUE(done);
  printf("planner stopped in %d ms\n", ms);

  little_sleep(std::chrono::milliseconds(50));

  EXPECT_EQ( planner->CheckPlanningStatus(), EPlannerStatus::Error );

  Planning::Path path;
  EXPECT_FALSE( planner->GetCompletePath(start, path) ) << "shouldn't have path";

  // now try again, letting it finish this time
  ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);
    
  ExpectPlanComplete(1000, planner, 6);

  Planning::GoalID selectedTargetIdx = 100;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path"; 
}


TEST(LatticePlannerMotionProfile, Simple1)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(0, 50, 0, 100, 0, 50, 10, 10);
  path.AppendLine(0, 100, 0, 140, 0, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {50, 42.42, 28.28, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, Simple2)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  motionProfile.pointTurnSpeed_rad_per_sec = 2;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  path.AppendPointTurn(0, 50, 0, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(0, 50, 0, 90, 0, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {50, 2, 50};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, Medium1)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(0, 50, 0, 80, 0, 50, 10, 10);
  path.AppendPointTurn(0, 80, 0, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(0, 80, 0, 120, 0, 50, 10, 10);
  path.AppendPointTurn(0, 120, 0, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(0, 120, 0, 140, 0, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {24.49, 24.49, 2, 50, 2, 50};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, Simple3)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {31.62};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}



TEST(LatticePlannerMotionProfile, Medium3)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  path.AppendArc(0, -20, 50, 20, DEG_TO_RAD(180), DEG_TO_RAD(90), 1, 1, 1);
  path.AppendLine(0, -20, 70, -50, 70, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {35.04, 24.49, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, SplitLine)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(0, 50, 0, 200, 0, 50, 10, 10);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {50, 50, IPathPlanner::finalPathSegmentSpeed_mmps};
  f32 expectedLen[] = {50, 25, 125};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_TRUE(expectedLen[i] == seg.GetLength());
  }
}

TEST(LatticePlannerMotionProfile, Medium2)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;
  
  Planning::Path path;
  path.AppendLine(0, -140.000000, 340.000000, -122.360680, 340.000000, 100, 200, 20);
  path.AppendArc(0, -122.360680, 245.278641, 94.721359, 1.570796, -0.463648, 100, 200, 20);
  path.AppendLine(0, -80.000000, 330.000000, 262.111450, 158.944275, 100, 200, 20);
  path.AppendArc(0, 300.000000, 234.721359, 84.721359, -2.034444, 0.463648, 100, 200, 20);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {100, 100, 100, 39.63, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, Complex1)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;
  
  Planning::Path path;
  path.AppendPointTurn(0, 250, -80, -2.03444, -2.5, 100, 100, DEG_TO_RAD(1), true);
  path.AppendArc(0, 210.000000, -60.000000, 44.721359, -0.463648, -0.643501, 100, 200, 20);
  path.AppendLine(0, 230, -100, -210, -320, 100, 200, 20);
  path.AppendPointTurn(0, -210, -320, -2.356194, 2.5, 100, 100, DEG_TO_RAD(1), true);
  path.AppendLine(0, -210, -320, -212.928925, -322.928925, 100, 200, 20);
  path.AppendArc(0, -195.857864, -340, 24.142136, 2.356194, 0.785398, 100, 200, 20);
  path.AppendPointTurn(0, -220, -340, 0, 2.5, 100, 100, DEG_TO_RAD(1), true);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {-2, 100, 100, 20, 2, 27.53, 27.53, 2};
  f32 expectedLen[] = {0, 28.77, 241.93, 249.99, 0, 4.14, 18.96, 0};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_NEAR(expectedLen[i], seg.GetLength(), 0.1);
  }
}


TEST(LatticePlannerMotionProfile, Complex2)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;
  
  Planning::Path path;
  path.AppendPointTurn(0, -80, -410, 0.463648, 2, 100, 100, DEG_TO_RAD(1), true);
  path.AppendArc(0, -100.000000, -370.000000, 44.721359, -1.107149, 0.643501, 100, 200, 20);
  path.AppendLine(0, -60.000000, -390.000000, 171.055725, 72.111458, 100, 200, 20);
  path.AppendArc(0, 95.278641, 110.000000, 84.721359, -0.463648, 0.463648, 100, 200, 20);
  path.AppendLine(0, 180.000000, 110.000000, 180.000000, 245.857864, 100, 200, 20);
  path.AppendArc(0, 214.142136, 245.857864, 34.142136, 3.141593, -0.785398, 100, 200, 20);
  path.AppendLine(0, 190.000000, 270.000000, 192.928925, 272.928925, 100, 200, 20);
  path.AppendArc(0, 210.000000, 255.857864, 24.142136, 2.356194, -0.785398, 100, 200, 20);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {2, 100, 100, 94.88, 86.20, 44.68, 30.39, 27.53, IPathPlanner::finalPathSegmentSpeed_mmps};
  f32 expectedLen[] = {0, 28.77, 491.71, 24.94, 39.28, 135.85, 26.81, 4.14, 18.96};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_NEAR(expectedLen[i], seg.GetLength(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, ZeroLengthSeg)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 500;
  
  Planning::Path path;
  path.AppendLine(0, 178.592758, -99.807365, 176.896408, -80.959740, 100, 200, 500);
  path.AppendLine(0, 176.896408, -80.959740, 176.000000, -71.000000, 100, 200, 500);
  path.AppendPointTurn(0, 176.000000, -71.000000, 1.570796, -2, 100, 500, DEG_TO_RAD_F32(2), true);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {100, 100, -2};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST(LatticePlannerMotionProfile, BackwardsForwards)
{
  Robot robot(1, cozmoContext);
  
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 500;
  
  Planning::Path path;
  path.AppendLine(0, 0, 0, -10, 0, -100, 200, 500);
  path.AppendPointTurn(0, -10, 0, -0.463648, -2, 100, 500, DEG_TO_RAD_F32(2), true);
  path.AppendLine(0, -10, 0, -30, 9.999996, -100, 200, 500);
  path.AppendArc(0, 10, 10, 40, DEG_TO_RAD(180), DEG_TO_RAD(40), 100, 200, 500);
  path.AppendLine(0, -20.641773, -15.711510, -33.541773, -31.01151, 100, 200, 500);
  
  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);
  
  Planning::Path path2;
  planner->ApplyMotionProfile(path, motionProfile, path2);
  
  f32 expectedSpeeds[] = {-100, -2, -100, 100, 100, 20};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    f32 x, y;
    seg.GetStartPoint(x, y);
    f32 x1, y1, a1;
    seg.GetEndPose(x1, y1, a1);
    PRINT_NAMED_ERROR("", "%f %f %f %f %f", x, y, x1, y1, RAD_TO_DEG(a1));
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}
