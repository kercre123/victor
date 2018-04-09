/**
 * File: testObjectInteractionInfoCache
 *
 * Author: Kevin M. Karol
 * Created: 5/11/17
 *
 * Description: Unit tests for the ObjectInteractionInfoCache
 *
 * Copyright: Anki, Inc. 2017
 *
 * --gtest_filter=ObjectInteractionInfoCache*
 **/


#include "gtest/gtest.h"


#include "coretech/common/engine/utils/timer.h"

#include "engine/activeObject.h"
#include "engine/activeObjectHelpers.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"


using namespace Anki;
using namespace Anki::Cozmo;

extern CozmoContext* cozmoContext;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// TODO refactor this because it's also in blockworld unit tests. This should be a helper
ObservableObject* CreateObjectLocatedAtOrigin(Robot& robot, ObjectType objectType)
{
  // matching activeID happens through objectID automatically on addition
  const ActiveID activeID = -1;
  const FactoryID factoryID = "";
  
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
TEST(ObjectInteractionInfoCache, BestObjectConsistency)
{
  Robot robot(0, cozmoContext);
  ObjectID objID1, objID2;

  // Add two objects
  ObservableObject* object1 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(nullptr != object1);
  objID1 = object1->GetID();
  
  ObservableObject* object2 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(nullptr != object2);
  objID2 = object2->GetID();

  const int xOffsetObj1 = 100;
  
  // put the first cube close to the robot, and put the second cube much further away
  {
    const Pose3d obj1Pose(0.0f, Z_AXIS_3D(), {xOffsetObj1, 0, 0}, robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, obj1Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }
  {
    const Pose3d obj2Pose(0.0f, Z_AXIS_3D(), {xOffsetObj1 *100, 0, 0}, robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, obj2Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }
  
  
  static const float arbitrary_time_increase = 100000000.0f;
  static float incrementEngineTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  incrementEngineTime_ns += arbitrary_time_increase;
  BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime_ns);
  
  // Use pickup object - details should not be important for this test, we're looking
  // to test default expected behavior
  ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  ObjectInteractionInfoCache& objCache = robot.GetAIComponent().GetComponent<ObjectInteractionInfoCache>();
  
  {
    // both objects should be valid for pickup
    auto validObjs = objCache.GetValidObjectsForIntention(intent);
    EXPECT_EQ(2, validObjs.size());
  }
  
  {
    // The closer object should be selected as the best object for pickup
    ObjectID bestObj = objCache.GetBestObjectForIntention(intent);
    EXPECT_EQ(object1->GetID(), bestObj);
  }
    
  // Move object 2 in closer - object 1 should still be the best object because
  {
    const Pose3d obj2Pose(0.0f, Z_AXIS_3D(), {xOffsetObj1/2, 0, 0}, robot.GetPose());
    auto result = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, obj2Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, result);
  }
  
  {
    incrementEngineTime_ns += arbitrary_time_increase;
    BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime_ns);
    ObjectID bestObj = objCache.GetBestObjectForIntention(intent);
    EXPECT_EQ(object1->GetID(), bestObj);
  }
  
  // Now remove object 1 - object 2 should become the best object
  {
    BlockWorld& blockWorld = robot.GetBlockWorld();
    blockWorld.ClearLocatedObject(object1);
    BlockWorldFilter deleteObj1Filter;
    deleteObj1Filter.AddAllowedID(object1->GetID());
    blockWorld.DeleteLocatedObjects(deleteObj1Filter);
  }
  {
    incrementEngineTime_ns += arbitrary_time_increase;
    BaseStationTimer::getInstance()->UpdateTime(incrementEngineTime_ns);
  }
  {
    auto validObjs = objCache.GetValidObjectsForIntention(intent);
    EXPECT_EQ(1, validObjs.size());
  }
  {
    ObjectID bestObj = objCache.GetBestObjectForIntention(intent);
    EXPECT_EQ(object2->GetID(), bestObj);
  }
}

