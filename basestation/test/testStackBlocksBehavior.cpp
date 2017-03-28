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
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/activeObjectHelpers.h"
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// TODO refactor this because it's also in blockworld unit tests. This should be a helper
ObservableObject* CreateObjectLocatedAtOrigin(Robot& robot, ObjectType objectType)
{
  // matching activeID happens through objectID automatically on addition
  const ActiveID activeID = -1;
  const FactoryID factoryID = 0;

  BlockWorld& blockWorld = robot.GetBlockWorld();
  Anki::Cozmo::ObservableObject* objectPtr = CreateActiveObjectByType(objectType, activeID, factoryID);
  ANKI_VERIFY(nullptr != objectPtr, "CreateObjectLocatedAtOrigin.CreatedNull", "");
  
  // check it currently doesn't exist in BlockWorld
  {
    BlockWorldFilter filter;
    filter.SetAllowedTypes( {objectPtr->GetType()} );
    filter.SetAllowedFamilies( {objectPtr->GetFamily()} );
    ObservableObject* sameBlock = blockWorld.FindLocatedMatchingObject(filter);
    ANKI_VERIFY(nullptr == sameBlock, "CreateObjectLocatedAtOrigin.TypeAlreadyInUse", "");
  }
  
  // set initial pose & ID (that's responsibility of the system creating the object)
  ANKI_VERIFY(!objectPtr->GetID().IsSet(), "CreateObjectLocatedAtOrigin.IDSet", "");
  ANKI_VERIFY(!objectPtr->HasValidPose(), "CreateObjectLocatedAtOrigin.HasValidPose", "");
  objectPtr->SetID();
  Anki::Pose3d originPose;
  originPose.SetParent( robot.GetWorldOrigin() );
  objectPtr->InitPose( originPose, Anki::PoseState::Known); // posestate could be something else
  
  // now they can be added to the world
  blockWorld.AddLocatedObject(std::shared_ptr<ObservableObject>(objectPtr));

  // need to pretend we observed this object
  robot.GetObjectPoseConfirmer().AddInExistingPose(objectPtr); // this has to be called after AddLocated just because
  
  // verify they are there now
  ANKI_VERIFY(objectPtr->GetID().IsSet(), "CreateObjectLocatedAtOrigin.IDNotset", "");
  ANKI_VERIFY(objectPtr->HasValidPose(), "CreateObjectLocatedAtOrigin.InvalidPose", "");
  ObservableObject* objectInWorld = blockWorld.GetLocatedObjectByID( objectPtr->GetID() );
  ANKI_VERIFY(objectInWorld == objectPtr, "CreateObjectLocatedAtOrigin.NotSameObject", "");
  ANKI_VERIFY(nullptr != objectInWorld, "CreateObjectLocatedAtOrigin.NullWorldPointer", "");
  return objectInWorld;
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SetupStackTest(Robot& robot, IBehavior*& stackBehavior, ObjectID& objID1, ObjectID& objID2)
{
  auto& aiComponent = robot.GetAIComponent();
  
  aiComponent.Init();

  CreateStackBehavior(robot, stackBehavior);

  BehaviorPreReqRobot prereq(robot);
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable without cubes";

  aiComponent.Update();
  aiComponent.Update();
  aiComponent.Update();
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable without cubes after update";

  auto& blockWorld = robot.GetBlockWorld();
  blockWorld.AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  blockWorld.AddConnectedActiveObject(1, 1, ObjectType::Block_LIGHTCUBE2);

  aiComponent.Update();
  ASSERT_FALSE(stackBehavior->IsRunnable(prereq)) << "behavior should not be runnable with unknown cubes";

  // Add two objects
  ObservableObject* object1 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(nullptr != object1);
  objID1 = object1->GetID();
  
  ObservableObject* object2 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(nullptr != object2);
  objID2 = object2->GetID();

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

  static float incrementEngineTime = 0.0f;
  incrementEngineTime += 100000000.0f;
  BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime);
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
    ObservableObject* object1 = blockWorld.GetLocatedObjectByID(objID1);
    ASSERT_TRUE(object1 != nullptr);
  }
  robot.SetCarryingObject(objID1, Vision::MARKER_INVALID);
    
  {
    ObservableObject* object2 = blockWorld.GetLocatedObjectByID(objID2);
    ASSERT_TRUE(object2 != nullptr);
  }
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.AddAllowedID(objID2);
  blockWorld.DeleteLocatedObjects(filter);

  {
    ObservableObject* object1 = blockWorld.GetLocatedObjectByID(objID1);
    ASSERT_TRUE(object1 != nullptr);
  }

  {
    ObservableObject* object2 = blockWorld.GetLocatedObjectByID(objID2);
    ASSERT_TRUE(object2 == nullptr) << "object should have been deleted";
  }

  aiComponent.Update();
  auto result = stackBehavior->Init();
  EXPECT_EQ(RESULT_OK, result);

  aiComponent.Update();
  auto status = stackBehavior->Update();
  EXPECT_NE(BehaviorStatus::Running, status) << "should have stopped running";
}
