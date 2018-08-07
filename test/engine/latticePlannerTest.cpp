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

#include "engine/latticePlanner.h"
#include "engine/robot.h"
#include "engine/cozmoContext.h"
#include "engine/components/pathComponent.h"

using namespace Anki;
using namespace Vector;

extern Anki::Vector::CozmoContext* cozmoContext;

class LatticePlannerTest : public testing::Test
{
protected:

  virtual void SetUp() override {
    _robot = new Robot(1, cozmoContext);

    _planner.reset(new LatticePlanner(_robot, cozmoContext->GetDataPlatform()) );
    ASSERT_TRUE(_planner != nullptr);

    // default planner to run in main thread
    _planner->SetIsSynchronous(true);
  }

  virtual void TearDown() override {
    Util::SafeDelete(_robot);
  }

  // "busy sleep" while suggesting that other threads run
  // for a small amount of time
  void little_sleep(std::chrono::microseconds us) {
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    auto end = start + us;
    do {
      std::this_thread::yield();
    } while (high_resolution_clock::now() < end);
  }

  void ExpectPlanCompleteInThread(int maxTimeMs) {
    bool done = false;
    int ms = 0;

    using namespace std::chrono;

    high_resolution_clock::time_point wallStart = high_resolution_clock::now();

    for(ms = 0; ms < maxTimeMs; ms++) {
      little_sleep(std::chrono::milliseconds(1));
      switch( _planner->CheckPlanningStatus() ) {
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

    high_resolution_clock::time_point wallEnd = high_resolution_clock::now();
    duration<double> time_d = duration_cast<duration<double>>(wallEnd - wallStart);
    double time = time_d.count();

    ASSERT_TRUE(done) << "planner didn't finish in " << ms << "ms. Wall time " << time << " sec";

    std::cout<<"planner took "<<ms<<"ms wall time " << time << " sec\n";

    EXPECT_LT(ms, maxTimeMs) << "shouldn't use all the available time";
  }

  Robot* _robot = nullptr;
  std::unique_ptr<LatticePlanner> _planner = nullptr;

};

TEST_F(LatticePlannerTest, Create)
{
  EXPECT_EQ(_planner->CheckPlanningStatus(), EPlannerStatus::Error);
}


TEST_F(LatticePlannerTest, PlanOnceEmpty)
{
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = _planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  EXPECT_EQ(_planner->CheckPlanningStatus(), EPlannerStatus::CompleteWithPlan);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = _planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path";
}

TEST_F(LatticePlannerTest, PlanTwiceEmpty)
{
  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = _planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  EXPECT_EQ(_planner->CheckPlanningStatus(), EPlannerStatus::CompleteWithPlan);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = _planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  int firstPathLength = path.GetNumSegments();

  EXPECT_GT(firstPathLength, 1) << "should be more than one action in the path";

  Pose3d start2(0, Z_AXIS_3D(), Vec3f(17,-15,0) );
  Pose3d goal2(0, Z_AXIS_3D(), Vec3f(2000,10,0) );

  ret = _planner->ComputePath(start2, goal2);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  EXPECT_EQ(_planner->CheckPlanningStatus(), EPlannerStatus::CompleteWithPlan);

  selectedTargetIdx = 200;

  hasPath = _planner->GetCompletePath(start2, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  int secondPathLength = path.GetNumSegments();

  EXPECT_GT(secondPathLength, 1) << "should be more than one action in the path";

  // NOTE: just checking length because I'm lazy. If the motion primitives change, this test could break. If
  // so, just change start2 or goal2 to see if that fixes it
  EXPECT_NE(firstPathLength, secondPathLength) << "should have a different path the second time";
}

TEST_F(LatticePlannerTest, PlanWhilePlanning)
{
  // use planner thread for this test, but make it super slow
  _planner->SetIsSynchronous(false);
  _planner->SetArtificialPlannerDelay_ms(10000);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal1(0, Z_AXIS_3D(), Vec3f(20,1000,0) );
  Pose3d goal2(0, Z_AXIS_3D(), Vec3f(-520,400,0) );

  EComputePathStatus ret1 = _planner->ComputePath(start, goal1);
  // NOTE: technically, if we context switch here and don't run this thread for really really long, this test
  // could break...
  EComputePathStatus ret2 = _planner->ComputePath(start, goal2);

  EXPECT_EQ(ret1, EComputePathStatus::Running);
  EXPECT_EQ(ret2, EComputePathStatus::Error);

  // clear artificial delay to let planner finish
  _planner->SetArtificialPlannerDelay_ms(0);

  // should still be able to finish the original plan
  ExpectPlanCompleteInThread(2000);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = _planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path";

  // check that it planned to the right goal
  float endX = 0.0f;
  float endY = 0.0f;
  float endTheta = 0.0f;
  path.GetSegmentConstRef( path.GetNumSegments() - 1 ).GetEndPose(endX, endY, endTheta);
  EXPECT_NEAR( endX, goal1.GetTranslation().x(), 10.0f ) << "planned to wrong goal";
  EXPECT_NEAR( endY, goal1.GetTranslation().y(), 10.0f ) << "planned to wrong goal";
}

TEST_F(LatticePlannerTest, PlanWhilePlanning2)
{
  // same test as above, but add an extra delay between the ComputePath calls to allow the worker thread to
  // run a bit

  // use planner thread for this test, but make it super slow
  _planner->SetIsSynchronous(false);
  _planner->SetArtificialPlannerDelay_ms(10000);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal1(0, Z_AXIS_3D(), Vec3f(20,1000,0) );
  Pose3d goal2(0, Z_AXIS_3D(), Vec3f(-520,400,0) );

  EComputePathStatus ret1 = _planner->ComputePath(start, goal1);
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  EComputePathStatus ret2 = _planner->ComputePath(start, goal2);

  EXPECT_EQ(ret1, EComputePathStatus::Running);
  EXPECT_EQ(ret2, EComputePathStatus::Error);

  // clear artificial delay to let planner finish
  _planner->SetArtificialPlannerDelay_ms(0);

  // should still be able to finish the original plan
  ExpectPlanCompleteInThread(2000);

  Planning::GoalID selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = _planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path";

  // check that it planned to the right goal
  float endX = 0.0f;
  float endY = 0.0f;
  float endTheta = 0.0f;
  path.GetSegmentConstRef( path.GetNumSegments() - 1 ).GetEndPose(endX, endY, endTheta);
  EXPECT_NEAR( endX, goal1.GetTranslation().x(), 10.0f ) << "planned to wrong goal";
  EXPECT_NEAR( endY, goal1.GetTranslation().y(), 10.0f ) << "planned to wrong goal";
}

TEST_F(LatticePlannerTest, StopPlanning)
{
  // use planner thread for this test, but slow it down a bit
  _planner->SetIsSynchronous(false);
  _planner->SetArtificialPlannerDelay_ms(1000);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = _planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  bool done = false;
  bool calledStop = false;
  int maxTimeMs = 100;
  const int kMsToSendStop = 4;
  int ms;
  for(ms = 0; ms < maxTimeMs; ms++) {
    little_sleep(std::chrono::milliseconds(1));
    switch( _planner->CheckPlanningStatus() ) {
      case EPlannerStatus::Error:
        printf("%d: err\n", ms);
        done = true;
        break;

      case EPlannerStatus::Running:
        printf("%d: running\n", ms);
        if(ms == kMsToSendStop) {
          _planner->StopPlanning();
          printf("%d: CALLING STOP\n", ms);
          calledStop = true;
        }

        break;

      case EPlannerStatus::CompleteWithPlan:
        printf("%d: CompleteWithPlan\n", ms);
        FAIL() << "_planner finished with a plan! Should have error";
        break;

      case EPlannerStatus::CompleteNoPlan:
        printf("%d: CompleteNoPlan\n", ms);
        FAIL() << "_planner finished with no plan! Should have error";
        break;
    }

    if(done)
      break;
  }

  EXPECT_GT(ms, 3) << "planner should have returned 'running' for at least a few ms";

  ASSERT_TRUE(calledStop) << "planner finished too quickly, this shouldn't happen anymore";
  ASSERT_TRUE(done);
  printf("planner stopped in %d ms\n", ms);

  if( _planner->CheckPlanningStatus() != EPlannerStatus::Error ) {
    little_sleep(std::chrono::milliseconds(50));
  }

  EXPECT_EQ( _planner->CheckPlanningStatus(), EPlannerStatus::Error );

  Planning::Path path;
  EXPECT_FALSE( _planner->GetCompletePath(start, path) ) << "shouldn't have path";

  // now try again, letting it finish this time
  _planner->SetArtificialPlannerDelay_ms(0);
  ret = _planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanCompleteInThread(1000);

  Planning::GoalID selectedTargetIdx = 100;

  bool hasPath = _planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path";
}


TEST_F(LatticePlannerTest, MotionProfileSimple1)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(50, 0, 100, 0, 50, 10, 10);
  path.AppendLine(100, 0, 140, 0, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {50, 42.42f, 28.28f, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileSimple2)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;
  motionProfile.pointTurnSpeed_rad_per_sec = 2;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);
  path.AppendPointTurn(50, 0, 0.f, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(50, 0, 90, 0, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {50, 2, 50};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileMedium1)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(50, 0, 80, 0, 50, 10, 10);
  path.AppendPointTurn(80, 0, 0.f, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(80, 0, 120, 0, 50, 10, 10);
  path.AppendPointTurn(120, 0, 0.f, DEG_TO_RAD(20), 1, 1, 1, DEG_TO_RAD(1), true);
  path.AppendLine(120, 0, 140, 0, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {24.49f, 24.49f, 2, 50, 2, 50};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileSimple3)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {31.62f};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}



TEST_F(LatticePlannerTest, MotionProfileMedium3)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);
  path.AppendArc(-20, 50, 20, DEG_TO_RAD(180), DEG_TO_RAD(90), 1, 1, 1);
  path.AppendLine(-20, 70, -50, 70, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {35.04f, 24.49f, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileSplitLine)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 50;
  motionProfile.accel_mmps2 = 10;
  motionProfile.decel_mmps2 = 10;

  Planning::Path path;
  path.AppendLine(0, 0, 50, 0, 50, 10, 10);
  path.AppendLine(50, 0, 200, 0, 50, 10, 10);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {50, 50, IPathPlanner::finalPathSegmentSpeed_mmps};
  f32 expectedLen[] = {50, 25, 125};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_TRUE(expectedLen[i] == seg.GetLength());
  }
}

TEST_F(LatticePlannerTest, MotionProfileMedium2)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;

  Planning::Path path;
  path.AppendLine(-140.000000f, 340.000000f, -122.360680f, 340.000000f, 100, 200, 20);
  path.AppendArc(-122.360680f, 245.278641f, 94.721359f, 1.570796f, -0.463648f, 100, 200, 20);
  path.AppendLine(-80.000000f, 330.000000f, 262.111450f, 158.944275f, 100, 200, 20);
  path.AppendArc(300.000000f, 234.721359f, 84.721359f, -2.034444f, 0.463648f, 100, 200, 20);

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {100, 100, 100, 39.63f, IPathPlanner::finalPathSegmentSpeed_mmps};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileComplex1)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;

  Planning::Path path;
  path.AppendPointTurn(250, -80, 0.f, -2.03444f, -2.5, 100, 100, DEG_TO_RAD(1), true);
  path.AppendArc(210.000000f, -60.000000, 44.721359f, -0.463648f, -0.643501f, 100, 200, 20);
  path.AppendLine(230, -100, -210, -320, 100, 200, 20);
  path.AppendPointTurn(-210, -320, 0.f, -2.356194f, 2.5, 100, 100, DEG_TO_RAD(1), true);
  path.AppendLine(-210, -320, -212.928925f, -322.928925f, 100, 200, 20);
  path.AppendArc(-195.857864f, -340, 24.142136f, 2.356194f, 0.785398f, 100, 200, 20);
  path.AppendPointTurn(-220, -340, 0.f, 0, 2.5, 100, 100, DEG_TO_RAD(1), true);

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {-2, 100, 100, 20, 2, 27.53f, 27.53f, 2};
  f32 expectedLen[] = {0, 28.77f, 241.93f, 249.99f, 0, 4.14f, 18.96f, 0};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_NEAR(expectedLen[i], seg.GetLength(), 0.1);
  }
}


TEST_F(LatticePlannerTest, MotionProfileComplex2)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 20;

  Planning::Path path;
  path.AppendPointTurn(-80, -410, 0.f, 0.463648f, 2, 100, 100, DEG_TO_RAD(1), true);
  path.AppendArc(-100.000000, -370.000000, 44.721359f, -1.107149f, 0.643501f, 100, 200, 20);
  path.AppendLine(-60.000000, -390.000000, 171.055725f, 72.111458f, 100, 200, 20);
  path.AppendArc(95.278641f, 110.000000, 84.721359f, -0.463648f, 0.463648f, 100, 200, 20);
  path.AppendLine(180.000000, 110.000000, 180.000000, 245.857864f, 100, 200, 20);
  path.AppendArc(214.142136f, 245.857864f, 34.142136f, 3.141593f, -0.785398f, 100, 200, 20);
  path.AppendLine(190.000000, 270.000000, 192.928925f, 272.928925f, 100, 200, 20);
  path.AppendArc(210.000000, 255.857864f, 24.142136f, 2.356194f, -0.785398f, 100, 200, 20);

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {2, 100, 100, 94.88f, 86.20f, 44.68f, 30.39f, 27.53f, IPathPlanner::finalPathSegmentSpeed_mmps};
  f32 expectedLen[] = {0, 28.77f, 491.71f, 24.94f, 39.28f, 135.85f, 26.81f, 4.14f, 18.96f};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
    ASSERT_NEAR(expectedLen[i], seg.GetLength(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileZeroLengthSeg)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 500;

  Planning::Path path;
  path.AppendLine(178.592758f, -99.807365f, 176.896408f, -80.959740f, 100, 200, 500);
  path.AppendLine(176.896408f, -80.959740f, 176.000000, -71.000000, 100, 200, 500);
  path.AppendPointTurn(176.000000, -71.000000, 0.f, 1.570796f, -2, 100, 500, DEG_TO_RAD(2.f), true);

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

  f32 expectedSpeeds[] = {100, 100, -2};
  for(int i=0; i<path2.GetNumSegments();i++)
  {
    Anki::Planning::PathSegment seg = path2.GetSegmentConstRef(i);
    ASSERT_NEAR(expectedSpeeds[i], seg.GetTargetSpeed(), 0.1);
  }
}

TEST_F(LatticePlannerTest, MotionProfileBackwardsForwards)
{
  PathMotionProfile motionProfile = PathMotionProfile();
  motionProfile.speed_mmps = 100;
  motionProfile.accel_mmps2 = 200;
  motionProfile.decel_mmps2 = 500;

  Planning::Path path;
  path.AppendLine(0, 0, -10, 0, -100, 200, 500);
  path.AppendPointTurn(-10, 0, 0.f, -0.463648f, -2, 100, 500, DEG_TO_RAD(2.f), true);
  path.AppendLine(-10, 0, -30, 9.999996f, -100, 200, 500);
  path.AppendArc(10, 10, 40, DEG_TO_RAD(180), DEG_TO_RAD(40), 100, 200, 500);
  path.AppendLine(-20.641773f, -15.711510f, -33.541773f, -31.01151f, 100, 200, 500);

  Planning::Path path2;
  _planner->ApplyMotionProfile(path, motionProfile, path2);

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
