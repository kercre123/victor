
/**
 * File: testBlockWorld
 *
 * Author: Matt Michini
 * Created: 02/28/2019
 *
 * Description: Tests for BlockWorld
 *
 * Copyright: Anki, Inc. 2019
 *
 * --gtest_filter=BlockWorld.*
 **/

#include "util/helpers/includeGTest.h"

#define private public
#define protected public

#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/charger.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/vision/visionSystem.h"

using namespace Anki;
using namespace Vector;

extern CozmoContext* cozmoContext;

class BlockWorldTest : public testing::Test
{
protected:
  
  virtual void SetUp() override {
    _robot.reset(new Robot(1, cozmoContext));
    _robot->FakeSyncRobotAck();
    
    // Fake a state message update for robot
    FakeRobotState(1);
    
    SetCameraCalibration();
  }
  
private:
  std::unique_ptr<Robot> _robot;
  
  void FakeRobotState(uint32_t atTimestamp) {
    RobotState stateMsg = _robot->GetDefaultRobotState();
    stateMsg.timestamp = atTimestamp;
    
    // Add some fake imu data to ensure that ImuHistory can find data
    for (auto& imuData : stateMsg.imuData) {
      imuData.timestamp = atTimestamp;
    }
    
    bool result = _robot->UpdateFullRobotState(stateMsg);
    ASSERT_EQ(result, RESULT_OK);
  }
  
  void SetCameraCalibration() {
    // Note: These values are approximate Cozmo calibration values, so they may not match the actual robot's calibration
    const u16 HEAD_CAM_CALIB_WIDTH  = 320;
    const u16 HEAD_CAM_CALIB_HEIGHT = 240;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
    const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
    const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
    
    auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                                HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                                HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
    
    _robot->GetVisionComponent().SetCameraCalibration(camCalib);
  }
  
  // Observe the given marker code at the given timestamp. If no 'corners' are specified, there is no guarantee where
  // the marker is in the frame - this just simply 'observes' a marker in some pose.
  void ObserveMarker(const Vision::Marker::Code code,
                     const uint32_t atTimestamp,
                     const Quad2f corners = Quad2f{Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)}) {
    // Fake an observation
    VisionProcessingResult procResult;
    procResult.timestamp = (TimeStamp_t) atTimestamp;
    procResult.observedMarkers.clear();
    procResult.observedMarkers.push_back(Vision::ObservedMarker((TimeStamp_t) atTimestamp, code, corners,
                                                                _robot->GetVisionComponent().GetCamera()));
    
    auto result = _robot->GetVisionComponent().UpdateVisionMarkers(procResult);
    ASSERT_EQ(RESULT_OK, result);
  }
  
};

TEST_F(BlockWorldTest, Create)
{
  ASSERT_TRUE(_robot != nullptr);
}

TEST_F(BlockWorldTest, ObserveMarker)
{
  // Observe a marker of an object, and ensure that we create an instance of that object in Blockworld
  
  // Send a fake robot state
  uint32_t fakeTimestamp_ms = 10;
  FakeRobotState(fakeTimestamp_ms);
  
  // Pretend to see Block_LIGHTCUBE_1
  const ObjectType objectType = ObjectType::Block_LIGHTCUBE1;
  const Block cube(objectType);
  const Vision::Marker::Code code = cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  ObserveMarker(code, fakeTimestamp_ms);
  
  // Check for unobserved markers (should have no effect)
  auto& blockWorld = _robot->GetBlockWorld();
  blockWorld.CheckForUnobservedObjects(fakeTimestamp_ms);
  
  // Ensure that this object has made its way into blockworld
  std::vector<ObservableObject*> objects;
  
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_EQ(1, objects.size());
  
  auto* object = objects.front();
  ASSERT_NE(nullptr, object);
  ASSERT_EQ(objectType, object->GetType());
  ASSERT_TRUE(object->GetID().IsSet());
  ASSERT_EQ(object->GetActiveID(), -1) << "Object should have an invalid activeID, since it's unconnected";
  ASSERT_EQ(_robot->GetWorldOriginID(), object->GetPose().GetRootID()) << "Object should be in robot's world origin";
  ASSERT_EQ(PoseState::Known, object->GetPoseState());
}


TEST_F(BlockWorldTest, ObserveConnectedBlock)
{
  // Pretend to connect to a block, disconnect from it, connect to it again, then observe that block later and ensure
  // that the object IDs are properly matched.
  
  auto& blockWorld = _robot->GetBlockWorld();
  
  ActiveID activeId = 1;
  FactoryID factoryId = "aa:bb:cc:dd:ee:ff";
  ObjectType objectType = ObjectType::Block_LIGHTCUBE1;
  
  ObjectID connectedObjectId = blockWorld.AddConnectedBlock(activeId, factoryId, objectType);
  ASSERT_TRUE(connectedObjectId.IsSet());
  ASSERT_EQ(1, blockWorld._connectedObjects.size());
  
  // Remove the connected (simulate disconnection)
  auto removedObjectId = blockWorld.RemoveConnectedBlock(activeId);
  ASSERT_EQ(removedObjectId, connectedObjectId);
  ASSERT_EQ(0, blockWorld._connectedObjects.size());
  
  // 'Connect' to the block again. It should have the same ObjectID.
  auto connectedObjectId_2 = blockWorld.AddConnectedBlock(activeId, factoryId, objectType);
  ASSERT_EQ(connectedObjectId, connectedObjectId_2);
  
  // Send a fake robot state
  uint32_t fakeTimestamp_ms = 10;
  FakeRobotState(fakeTimestamp_ms);
  
  // Now pretend to observe an object of the same type. It should inherit the ObjectID of the connected block.
  std::shared_ptr<ObservableObject> obsObject;
  obsObject.reset(new Block(objectType));
  Pose3d fakePose;
  fakePose.SetParent(_robot->GetWorldOrigin());
  obsObject->SetPose(fakePose, 10.f, PoseState::Known);
  auto objectVec = std::vector<std::shared_ptr<ObservableObject>>({obsObject});
  blockWorld.ProcessVisualObservations(objectVec, fakeTimestamp_ms);
  
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
  ASSERT_EQ(obsObject->GetID(), connectedObjectId);
  ASSERT_EQ(obsObject->GetActiveID(), activeId);
  
  // Pretend to disconnect from the block, and ensure we remove the connected instance and keep the located instance
  const auto removedObjID = _robot->GetBlockWorld().RemoveConnectedBlock(activeId);
  ASSERT_NE(nullptr, obsObject);
  ASSERT_EQ(obsObject->GetID(), removedObjID);
  ASSERT_EQ(0, blockWorld._connectedObjects.size());
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
}

TEST_F(BlockWorldTest, ConnectToObservedBlock)
{
  // Pretend to observe a block then connect to it and ensure that the object IDs are properly matched.
  
  auto& blockWorld = _robot->GetBlockWorld();
  
  // Send a fake robot state
  uint32_t fakeTimestamp_ms = 10;
  FakeRobotState(fakeTimestamp_ms);
  
  ObjectType objectType = ObjectType::Block_LIGHTCUBE1;
  
  // Fake an observation of a Block_LIGHTCUBE1
  std::shared_ptr<ObservableObject> obsObject;
  obsObject.reset(new Block(objectType));
  Pose3d fakePose;
  fakePose.SetParent(_robot->GetWorldOrigin());
  obsObject->SetPose(fakePose, 10.f, PoseState::Known);
  auto objectVec = std::vector<std::shared_ptr<ObservableObject>>({obsObject});
  blockWorld.ProcessVisualObservations(objectVec, fakeTimestamp_ms);
  
  ASSERT_TRUE(obsObject->GetID().IsSet());
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
  
  // Send a fake robot state
  fakeTimestamp_ms += 10;
  FakeRobotState(fakeTimestamp_ms);
  
  // Pretend to 'connect' to a block of the same type. The connected object should inherit the objectID of the observed
  // block.
  ActiveID activeId = 1;
  FactoryID factoryId = "aa:bb:cc:dd:ee:ff";

  ObjectID connectedObjectId = blockWorld.AddConnectedBlock(activeId, factoryId, objectType);
  ASSERT_EQ(1, blockWorld._connectedObjects.size());
  
  ASSERT_EQ(connectedObjectId, obsObject->GetID());
  
  // Pretend to disconnect from the block, and ensure we remove the connected instance and keep the located instance
  const auto removedObjID = _robot->GetBlockWorld().RemoveConnectedBlock(activeId);
  ASSERT_NE(nullptr, obsObject);
  ASSERT_EQ(obsObject->GetID(), removedObjID);
  ASSERT_EQ(0, blockWorld._connectedObjects.size());
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
}

TEST_F(BlockWorldTest, LocalizeToCharger)
{
  // See a charger for the first time - should immediately localize to it
  ASSERT_FALSE(_robot->IsLocalized());

  const auto firstOriginId = _robot->GetWorldOriginID();
  
  // Send a fake robot state
  uint32_t fakeTimestamp_ms = 10;
  FakeRobotState(fakeTimestamp_ms);

  // Fake an observation of a charger
  Charger charger;
  const auto& code = charger.GetMarkers().front().GetCode();
  ObserveMarker(code, fakeTimestamp_ms);
  
  // Robot should be localized now in the original origin, since we've observed the charger
  ASSERT_TRUE(_robot->IsLocalized());
  ASSERT_EQ(firstOriginId, _robot->GetWorldOriginID());
  
  auto& blockWorld = _robot->GetBlockWorld();
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
  
  BlockWorldFilter filter;
  filter.AddAllowedType(ObjectType::Charger_Basic);
  auto* object = blockWorld.FindLocatedMatchingObject(filter);
  
  ASSERT_NE(nullptr, object);

  // the observed object has the correct observation time stamp
  ASSERT_EQ(object->GetLastObservedTime(), fakeTimestamp_ms);
  
  auto localizedToID = _robot->GetLocalizedTo();
  ASSERT_TRUE(localizedToID.IsSet());
  ASSERT_EQ(localizedToID, object->GetID());
  
  // Delocalize. Should no longer have the charger in the current origin.
  fakeTimestamp_ms += 10;
  FakeRobotState(fakeTimestamp_ms);
  
  const bool isCarryingObject = false;
  _robot->Delocalize(isCarryingObject);
  
  ASSERT_FALSE(_robot->IsLocalized());
  
  localizedToID = _robot->GetLocalizedTo();
  ASSERT_FALSE(localizedToID.IsSet());
  ASSERT_NE(firstOriginId, _robot->GetWorldOriginID()) << "Should be in a new origin since we've delocalized";
  
  object = blockWorld.FindLocatedMatchingObject(filter);
  ASSERT_EQ(nullptr, object);
  
  // Now observe the charger again. Should re-localize to the charger in the first origin.
  fakeTimestamp_ms += 10;
  FakeRobotState(fakeTimestamp_ms);
  
  ObserveMarker(code, fakeTimestamp_ms);
  
  ASSERT_TRUE(_robot->IsLocalized());
  ASSERT_EQ(firstOriginId, _robot->GetWorldOriginID()) << "Should have localized in the first origin, since that's where we saw the charger first";
  
  object = blockWorld.FindLocatedMatchingObject(filter);
  
  ASSERT_NE(nullptr, object);

  // the observed object has the correct observation time stamp
  ASSERT_EQ(object->GetLastObservedTime(), fakeTimestamp_ms);
  
  localizedToID = _robot->GetLocalizedTo();
  ASSERT_TRUE(localizedToID.IsSet());
  ASSERT_EQ(localizedToID, object->GetID());
}

TEST_F(BlockWorldTest, ObservationDistance)
{
  ASSERT_FALSE(_robot->IsLocalized());
  
  // Send a fake robot state
  uint32_t fakeTimestamp_ms = 10;
  FakeRobotState(fakeTimestamp_ms);
  
  // Fake an observation of a charger from far away
  const Quad2f farCorners {
    Point2f( 67,117),  Point2f( 70,135),  Point2f(82,116),  Point2f(84,134)
  };
  
  Charger charger;
  const auto& code = charger.GetMarkers().front().GetCode();
  ObserveMarker(code, fakeTimestamp_ms, farCorners);
  
  // This observation should be ignored because the charger is so 'far' away.
  auto& blockWorld = _robot->GetBlockWorld();
  ASSERT_FALSE(_robot->IsLocalized());
  ASSERT_EQ(0, blockWorld._locatedObjects.size());
  
  fakeTimestamp_ms += 10;
  FakeRobotState(fakeTimestamp_ms);
  
  // Now observe the charger from 'close'. This should cause a localization
  const Quad2f closeCorners {
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  ObserveMarker(code, fakeTimestamp_ms, closeCorners);
  
  ASSERT_TRUE(_robot->IsLocalized());
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
  
  BlockWorldFilter filter;
  filter.AddAllowedType(ObjectType::Charger_Basic);
  auto* object = blockWorld.FindLocatedMatchingObject(filter);
  
  ASSERT_NE(nullptr, object);

  // the observed object has the correct observation time stamp
  ASSERT_EQ(object->GetLastObservedTime(), fakeTimestamp_ms);
  
  auto localizedToID = _robot->GetLocalizedTo();
  ASSERT_TRUE(localizedToID.IsSet());
  ASSERT_EQ(localizedToID, object->GetID());
  
  // Now observe the charger in a new location (but still close enough). This should move the charger's pose (not the
  // robot's pose), since the robot has not moved.
  auto firstChargerPose = object->GetPose();
  
  fakeTimestamp_ms += 10;
  FakeRobotState(fakeTimestamp_ms);
  
  const Quad2f closeCorners2 = {
    Point2f( 57,107),  Point2f( 60,175),  Point2f(126,106),  Point2f(127,174)
  };
  
  ObserveMarker(code, fakeTimestamp_ms, closeCorners2);
  
  ASSERT_FALSE(firstChargerPose.IsSameAs(object->GetPose(), 5.f, DEG_TO_RAD(5.f)));
  
  ASSERT_TRUE(_robot->IsLocalized());
  ASSERT_EQ(1, blockWorld._locatedObjects.size());
  localizedToID = _robot->GetLocalizedTo();
  ASSERT_EQ(localizedToID, object->GetID());

  // the observed object has the correct observation time stamp
  ASSERT_EQ(object->GetLastObservedTime(), fakeTimestamp_ms);
}

