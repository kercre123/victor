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

// Access protected factory functions for test purposes
#define protected public

#include "gtest/gtest.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/activeObject.h"
#include "engine/activeObjectHelpers.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorStackBlocks.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Cozmo;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CreateStackBehavior(Robot& robot, ICozmoBehaviorPtr& stackBehavior, BehaviorExternalInterface& behaviorExternalInterface)
{
  ASSERT_TRUE(stackBehavior == nullptr) << "test bug: should not have behavior yet";

  BehaviorContainer& behaviorContainer = robot.GetAIComponent().GetBehaviorContainer();
  
  // Build PICKUP ID
  {
    // Arbitrarily using the Wait ID - no effect on implementation details
    const std::string& configStr =
      R"({
        "behaviorClass": "PickUpCube",
        "behaviorID": "PickupCube"
    
      })";

    Json::Value config;
    Json::Reader reader;
    bool parseOK = reader.parse( configStr.c_str(), config);
    ASSERT_TRUE(parseOK) << "failed to parse JSON, bug in the test";
    const bool createdOK = behaviorContainer.CreateAndStoreBehavior(config);
    ASSERT_TRUE(createdOK);
    
  }
  

  
  // Arbitrarily using the Wait ID - no effect on implementation details
  const std::string& configStr =
    R"({
         "behaviorClass": "StackBlocks",
         "behaviorID": "Wait_TestInjectable",
         "pickupBehaviorID": "PickupCube"
       })";

  Json::Value config;   
  Json::Reader reader;
  bool parseOK = reader.parse( configStr.c_str(), config);
  ASSERT_TRUE(parseOK) << "failed to parse JSON, bug in the test";

  const bool createdOK = behaviorContainer.CreateAndStoreBehavior(config);
  ASSERT_TRUE(createdOK);
  stackBehavior = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(Wait_TestInjectable));
  stackBehavior->Init(behaviorExternalInterface);
  stackBehavior->OnEnteredActivatableScope();
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
void SetupStackTest(Robot& robot, ICozmoBehaviorPtr& stackBehavior,
                    TestBehaviorFramework& testBehaviorFramework,
                    ObjectID& objID1, ObjectID& objID2)
{
  DependencyManagedEntity<RobotComponentID> dependencies;

  auto& aiComponent = testBehaviorFramework.GetAIComponent();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();

  CreateStackBehavior(robot, stackBehavior, behaviorExternalInterface);

  ASSERT_FALSE(stackBehavior->WantsToBeActivated()) << "behavior should not be activatable without cubes";
  
  IncrementBaseStationTimerTicks();
  aiComponent.UpdateDependent(dependencies);
  IncrementBaseStationTimerTicks();
  aiComponent.UpdateDependent(dependencies);
  IncrementBaseStationTimerTicks();
  aiComponent.UpdateDependent(dependencies);
  ASSERT_FALSE(stackBehavior->WantsToBeActivated()) << "behavior should not be activatable without cubes after update";

  auto& blockWorld = robot.GetBlockWorld();
  blockWorld.AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  blockWorld.AddConnectedActiveObject(1, 1, ObjectType::Block_LIGHTCUBE2);

  IncrementBaseStationTimerTicks();
  aiComponent.UpdateDependent(dependencies);
  ASSERT_FALSE(stackBehavior->WantsToBeActivated()) << "behavior should not be activatable with unknown cubes";

  // Add two objects
  ObservableObject* object1 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(nullptr != object1);
  objID1 = object1->GetID();
  
  ObservableObject* object2 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(nullptr != object2);
  objID2 = object2->GetID();

  // put two cubes in front of the robot
  {
    const Pose3d obj1Pose(0.0f, Z_AXIS_3D(), {100, 0, 0}, robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, obj1Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }
  {
    const Pose3d obj2Pose(0.0f, Z_AXIS_3D(), {100, 55, 0}, robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, obj2Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }

  static float incrementEngineTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  incrementEngineTime_ns += 100000000.0f;
  BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime_ns);
  ASSERT_TRUE(stackBehavior->WantsToBeActivated()) << "now behavior should be activatable";

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StackBlocksBehavior, InitBehavior)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  ICozmoBehaviorPtr stackBehavior = nullptr;
  ObjectID objID1, objID2;
  SetupStackTest(robot, stackBehavior, testBehaviorFramework, objID1, objID2);
  
  stackBehavior->OnActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(StackBlocksBehavior, DeleteCubeCrash)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  auto& blockWorld = robot.GetBlockWorld();
  auto& aiComponent = robot.GetAIComponent();

  ICozmoBehaviorPtr stackBehavior = nullptr;
  ObjectID objID1, objID2;
  SetupStackTest(robot, stackBehavior, testBehaviorFramework,
                 objID1, objID2);

  {
    ObservableObject* object1 = blockWorld.GetLocatedObjectByID(objID1);
    ASSERT_TRUE(object1 != nullptr);
  }
  robot.GetCarryingComponent().SetCarryingObject(objID1, Vision::MARKER_INVALID);
    
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
    
  DependencyManagedEntity<RobotComponentID> dependencies;
  aiComponent.UpdateDependent(dependencies);
  //auto result =
  stackBehavior->WantsToBeActivated();
  stackBehavior->OnActivated();
  //EXPECT_EQ(RESULT_OK, result);

  static float incrementEngineTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  incrementEngineTime_ns += 100000000.0f;
  BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime_ns);
  
  //stackBehavior->UpdateInternal();
}
