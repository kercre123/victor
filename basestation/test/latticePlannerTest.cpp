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

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/types.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"

using namespace Anki;
using namespace Cozmo;

extern Anki::Util::Data::DataPlatform* dataPlatform;

TEST(LatticePlanner, Create)
{

  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);

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


void ExpectPlanComplete(int maxTimeMs, LatticePlanner* planner)
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

  EXPECT_GT(ms, 2) << "the planner might be fast, but not that fast!";
  EXPECT_LT(ms, maxTimeMs) << "shouldn't use all the available time";

  printf("planner finished after %dms\n", ms);
}

}

TEST(LatticePlanner, PlanOnceEmpty)
{
  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(100, planner);

  size_t selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path"; 
}

TEST(LatticePlanner, PlanTwiceEmpty)
{
  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(100, planner);

  size_t selectedTargetIdx = 100;
  Planning::Path path;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  int firstPathLength = path.GetNumSegments();

  EXPECT_GT(firstPathLength, 1) << "should be more than one action in the path";

  Pose3d start2(0, Z_AXIS_3D(), Vec3f(17,-15,0) );
  Pose3d goal2(0, Z_AXIS_3D(), Vec3f(200,10,0) );

  ret = planner->ComputePath(start2, goal2);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ExpectPlanComplete(100, planner);

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

TEST(LatticePlanner, PlanWhilePlanning)
{
  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);

  LatticePlanner* planner = dynamic_cast<LatticePlanner*>(robot._longPathPlanner);
  ASSERT_TRUE(planner != nullptr);

  Pose3d start(0, Z_AXIS_3D(), Vec3f(0,0,0) );
  Pose3d goal(0, Z_AXIS_3D(), Vec3f(20,100,0) );

  EComputePathStatus ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Running);

  ret = planner->ComputePath(start, goal);
  EXPECT_EQ(ret, EComputePathStatus::Error);

  // should still be able to finish he original plan

  ExpectPlanComplete(100, planner);

  size_t selectedTargetIdx = 100;
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

  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);

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
    
  ExpectPlanComplete(1000, planner);

  size_t selectedTargetIdx = 100;

  bool hasPath = planner->GetCompletePath(start, path, selectedTargetIdx);
  ASSERT_TRUE(hasPath);
  EXPECT_EQ(selectedTargetIdx, 0) << "only one target, should have selected it";

  EXPECT_GT(path.GetNumSegments(), 1) << "should be more than one action in the path"; 
}
