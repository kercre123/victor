/**
 * File: testStackBlocksBehavior.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-03-07
 *
 * Description: Unit tests specifically for the stacking behavior
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "gtest/gtest.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorStackBlocks.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "clad/types/behaviorTypes.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"


using namespace Anki;
using namespace Anki::Cozmo;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CreateStackBehavior(Robot& robot, IBehavior*& stackBehavior)
{
  ASSERT_TRUE(stackBehavior == nullptr) << "test bug: should not have behavior yet";

  auto& factory = robot.GetBehaviorManager().GetBehaviorFactory();

  const std::string& configStr =
    R"({
         "behaviorClass": "StackBlocks",
         "name": "TEST_StackBlocks",
         "flatScore": 0.8
       })";

  Json::Value config;   
  Json::Reader reader;
  bool parseOK = reader.parse( configStr.c_str(), config);
  ASSERT_TRUE(parseOK) << "failed to parse JSON, bug in the test";
  
  stackBehavior = factory.CreateBehavior(BehaviorClass::StackBlocks,
                                         robot,
                                         config,
                                         BehaviorFactory::NameCollisionRule::Fail);
  ASSERT_TRUE(stackBehavior != nullptr);
}

void SetupStackTest(Robot& robot, IBehavior*& stackBehavior, ObjectID& objID1, ObjectID& objID2)
{
  auto& blockWorld = robot.GetBlockWorld();
  auto& aiComponent = robot.GetAIComponent();
  
  aiComponent.Init();

  CreateStackBehavior(robot, stackBehavior);

  BehaviorPreReqRobot prereq(robot);
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable without cubes";

  aiComponent.Update();
  aiComponent.Update();
  aiComponent.Update();
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable without cubes after update";

  // Add two objects
  objID1 = blockWorld.AddActiveObject(1, 1, ActiveObjectType::OBJECT_CUBE1);
  objID2 = blockWorld.AddActiveObject(2, 2, ActiveObjectType::OBJECT_CUBE2);

  ObservableObject* object1 = blockWorld.GetObjectByID(objID1);
  ASSERT_TRUE(nullptr != object1);
  
  ObservableObject* object2 = blockWorld.GetObjectByID(objID2);
  ASSERT_TRUE(nullptr != object2);

  aiComponent.Update();
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable with unknown cubes";

  // put two cubes in front of the robot
  {
    const Pose3d obj1Pose(0.0f, Z_AXIS_3D(), {100, 0, 0}, &robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, obj1Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }
  {
    const Pose3d obj2Pose(0.0f, Z_AXIS_3D(), {100, 55, 0}, &robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, obj2Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }

  aiComponent.Update();
  ASSERT_TRUE(stackBehavior->IsRunnable(prereq)) << "now behavior should be runnable";

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StackBlocksBehavior, InitBehavior)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);

  IBehavior* stackBehavior = nullptr;
  ObjectID objID1, objID2;
  SetupStackTest(robot, stackBehavior, objID1, objID2);
  
  auto result = stackBehavior->Init();

  EXPECT_EQ(RESULT_OK, result);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StackBlocksBehavior, DeleteCubeCrash)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  auto& blockWorld = robot.GetBlockWorld();
  auto& aiComponent = robot.GetAIComponent();

  IBehavior* stackBehavior = nullptr;
  ObjectID objID1, objID2;
  SetupStackTest(robot, stackBehavior, objID1, objID2);

  {
    ObservableObject* object1 = blockWorld.GetObjectByID(objID1);
    ASSERT_TRUE(object1 != nullptr);
  }
  robot.SetCarryingObject(objID1);
    
  {
    ObservableObject* object2 = blockWorld.GetObjectByID(objID2);
    ASSERT_TRUE(object2 != nullptr);
  }
  bool deleted = blockWorld.DeleteObject(objID2);
  EXPECT_TRUE(deleted);

  {
    ObservableObject* object1 = blockWorld.GetObjectByID(objID1);
    ASSERT_TRUE(object1 != nullptr);
  }

  {
    ObservableObject* object2 = blockWorld.GetObjectByID(objID2);
    ASSERT_TRUE(object2 == nullptr) << "object should have been deleted";
  }

  aiComponent.Update();
  auto result = stackBehavior->Init();
  EXPECT_EQ(RESULT_OK, result);

  aiComponent.Update();
  auto status = stackBehavior->Update();
  EXPECT_NE(BehaviorStatus::Running, status) << "should have stopped running";
}
