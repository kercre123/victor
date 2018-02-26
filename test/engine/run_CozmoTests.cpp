#define private public
#define protected public

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/shared/types.h"

#include "camera/cameraService.h"
#include "osState/osState.h"
#include "cubeBleClient/cubeBleClient.h"

#include "engine/activeObject.h"
#include "engine/activeCube.h"
#include "engine/activeObjectHelpers.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/ramp.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/robotToEngineImplMessaging.h"
#include "engine/vision/visionSystem.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "gtest/gtest.h"
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <array>
#include <fstream>
#include <unistd.h>

Anki::Cozmo::CozmoContext* cozmoContext = nullptr; // This is externed and used by tests

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(DataPlatform, ReadWrite)
{
  ASSERT_TRUE(cozmoContext->GetDataPlatform() != nullptr);
  Json::Value config;
  const bool readSuccess = cozmoContext->GetDataPlatform()->readAsJson(
    Anki::Util::Data::Scope::Resources,
    "config/engine/configuration.json",
    config);
  EXPECT_TRUE(readSuccess);

  config["blah"] = 7;
  const bool writeSuccess = cozmoContext->GetDataPlatform()->writeAsJson(Anki::Util::Data::Scope::Cache, "someRandomFolder/A/writeTest.json", config);
  EXPECT_TRUE(writeSuccess);

  std::string someRandomFolder = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Cache, "someRandomFolder");
  Anki::Util::FileUtils::RemoveDirectory(someRandomFolder);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, DISABLED_AddAndRemoveObject)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  {
    // There should be nothing in BlockWorld yet
    BlockWorldFilter filter;
    std::vector<ObservableObject*> objects;
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_TRUE(objects.empty());
  }
  
  {
    // no connected objects either
    BlockWorldFilter filter;
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(filter, activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }

  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);

  // Fake an observation of a block:
  const ObjectType testType = ObjectType::Block_LIGHTCUBE1;
  Block_Cube1x1 testCube(testType);
  Vision::Marker::Code testCode = testCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  
  /***************************************************************************
   *
   *                          Camera Calibration
   *
   **************************************************************************/
  
  // Calibration values from Sept 1, 2015 - on 4.1 robot headboard with SSID 3a97
  const u16 HEAD_CAM_CALIB_WIDTH  = 400;
  const u16 HEAD_CAM_CALIB_HEIGHT = 296;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 278.065116921f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 278.867229568f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 197.801561858f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 151.672492176f;
  /*
  const f32 HEAD_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] = {
    0.11281163f,
    -0.31673507f,
    -0.00226334f,
    0.00200109f
  };
  */
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  const f32 halfHeight = 0.25f*static_cast<f32>(camCalib->GetNrows());
  const f32 halfWidth = 0.25f*static_cast<f32>(camCalib->GetNcols());
  const f32 xcen = camCalib->GetCenter_x();
  const f32 ycen = camCalib->GetCenter_y();

  // stretch observed bottom side slightly to make computed pose definitely not flat (so it doesn't get clamped)
  const f32 kNotFlatFraction = 1.03f;
  
  Quad2f corners;
  const f32 markerHalfSize = std::min(halfHeight, halfWidth);

  corners[Quad::TopLeft]    = {xcen - markerHalfSize, ycen - markerHalfSize};
  corners[Quad::BottomLeft] = {xcen - markerHalfSize*kNotFlatFraction, ycen + markerHalfSize};
  corners[Quad::TopRight]   = {xcen + markerHalfSize, ycen - markerHalfSize};
  corners[Quad::BottomRight]= {xcen + markerHalfSize*kNotFlatFraction, ycen + markerHalfSize};
  Vision::ObservedMarker marker(stateMsg.timestamp, testCode, corners, robot.GetVisionComponent().GetCamera());
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  // Queue the marker in VisionComponent, which will in turn queue it for processing
  // by BlockWorld
  
  // Tick BlockWorld, which will use the queued marker
  std::list<Vision::ObservedMarker> markers{marker};
  lastResult = robot.GetBlockWorld().UpdateObservedMarkers(markers);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  {
    // There should be nothing in BlockWorld yet because we have only seen it once (it's unconfirmed)
    BlockWorldFilter filter;
    std::vector<ObservableObject*> objects;
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_TRUE(objects.empty());
  }

  {
    // still no connected objects
    BlockWorldFilter filter;
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(filter, activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // After seeing three times, ...
  const s32 kNumObservations = 3;
  for(s32 i=0; i<kNumObservations; ++i)
  {
    // Tick the robot, which will tick the BlockWorld, which will use the queued marker
    stateMsg.timestamp+=10;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    Vision::ObservedMarker marker(stateMsg.timestamp, testCode, corners, robot.GetVisionComponent().GetCamera());
    std::list<Vision::ObservedMarker> markers{marker};
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(markers);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  ObjectID firstObjID;
  
  // There should now be an object of the right type, with the right ID in BlockWorld
  {
    BlockWorldFilter filter;
    std::vector<ObservableObject*> objects;
    filter.SetAllowedTypes({testType});
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_EQ(1, objects.size());
    auto objByIdIter = objects.begin();
    ASSERT_NE(objByIdIter, objects.end());
    ASSERT_NE(*objByIdIter, nullptr);
    
    ObservableObject* object = *objByIdIter;
    ASSERT_EQ(object->GetType(), testType);
    firstObjID = object->GetID();
    
    ASSERT_EQ(PoseState::Known, object->GetPoseState());
  
    // Make sure the corners used above resulted in a non-flat pose so clamping doesn't occur
    // and we can expect the reprojected corners to closely match below
    ASSERT_FALSE(object->IsRestingFlat(DEG_TO_RAD(object->GetRestingFlatTolForLocalization_deg())));
    
    // Projected corners of the marker should match the corners of the fake marker
    // we queued above
    std::vector<const Vision::KnownMarker*> observedMarkers;
    object->GetObservedMarkers(observedMarkers, 1);
    ASSERT_EQ(1, observedMarkers.size()); // Should only have seen one
  
    Pose3d markerPoseWrtCamera;
    const bool poseResult = observedMarkers[0]->GetPose().GetWithRespectTo(robot.GetVisionComponent().GetCamera().GetPose(), markerPoseWrtCamera);
    ASSERT_TRUE(poseResult);
  
    auto obsCorners3d = observedMarkers[0]->Get3dCorners(markerPoseWrtCamera);
    Quad2f obsCorners;
    robot.GetVisionComponent().GetCamera().Project3dPoints(obsCorners3d, obsCorners);
    const f32 kCornerTol_pix = 0.75f;
    const bool cornersMatch = IsNearlyEqual(obsCorners, corners, kCornerTol_pix);
    ASSERT_TRUE(cornersMatch);
  }

  // After NOT seeing the object three times, it should still be known
  // because we don't yet have evidence that actually isn't there
  std::list<Vision::ObservedMarker> emptyMarkersList;
  for(s32 i=0; i<kNumObservations; ++i)
  {
    stateMsg.timestamp+=10;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    robot.GetVisionComponent().FakeImageProcessed(stateMsg.timestamp);
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  {
    // should be known now
    ObservableObject* object = blockWorld.GetLocatedObjectByID(firstObjID);
    ASSERT_NE(object, nullptr);
    ASSERT_EQ(PoseState::Known, object->GetPoseState());
  
    // Now fake an object moved message
    object->SetIsMoving(true, 0);
    const bool propagateStack = false;
    robot.GetObjectPoseConfirmer().MarkObjectDirty(object, propagateStack);
  }
  
  // Now after not seeing the object three times, it should be Unknown
  // because it was dirty
  for(s32 i=0; i<kNumObservations; ++i)
  {
    stateMsg.timestamp+=10;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    robot.GetVisionComponent().FakeImageProcessed(stateMsg.timestamp);
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  {
    // should be unknown now. In the new code Unknown objects are removed from their origin
    ObservableObject* object = blockWorld.GetLocatedObjectByID(firstObjID);
    ASSERT_EQ(object, nullptr);
  }
  
  // See the object again, but from "far" away, which after a few ticks should make it
  // Dirty again.
  corners.Scale(0.15f);
  
  for(s32 i=0; i<kNumObservations; ++i)
  {
    // Tick the robot, which will tick the BlockWorld, which will use the queued marker
    stateMsg.timestamp+=10;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    Vision::ObservedMarker markerFar(stateMsg.timestamp, testCode, corners, robot.GetVisionComponent().GetCamera());
    std::list<Vision::ObservedMarker> markersFar{markerFar};
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(markersFar);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  // Find the object again (because it may have been deleted while unknown since
  // it had no active ID set)
  ObjectID secondObjID;
  {
    BlockWorldFilter filter;
    filter.SetAllowedTypes({testType});
    std::vector<ObservableObject*> objects;
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_EQ(objects.size(), 1);
    auto objByIdIter = objects.begin();
    ASSERT_NE(objByIdIter, objects.end());
    ASSERT_NE(*objByIdIter, nullptr);
    ObservableObject* object = *objByIdIter;
    secondObjID = object->GetID();
  
    ASSERT_NE(nullptr, object);
    ASSERT_EQ(PoseState::Dirty, object->GetPoseState());
  }
  
  {
    // Now try clearing the object, and make sure we can't still get it using the second ID
    ObservableObject* objectBeforeClear = blockWorld.GetLocatedObjectByID(secondObjID);
    ASSERT_NE(objectBeforeClear, nullptr);
    
    blockWorld.ClearLocatedObject(objectBeforeClear);
    
    ObservableObject* objectAfterClear = blockWorld.GetLocatedObjectByID(secondObjID);
    ASSERT_NE(objectAfterClear, nullptr);
    
    BlockWorldFilter filter;
    filter.AddAllowedID(secondObjID);
    blockWorld.DeleteLocatedObjects(filter);
    
    ObservableObject* objectAfterDelete = blockWorld.GetLocatedObjectByID(secondObjID);
    ASSERT_EQ(objectAfterDelete, nullptr);
  }
  
} // BlockWorld.AddAndRemoveObject

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Helper method for BlockWorld.UpdateObjectOrigins Test
static Anki::Result ObserveMarkerHelper(const s32 kNumObservations,
                                        std::list<std::pair<Anki::Vision::Marker::Code, Anki::Quad2f>>&& codesAndCorners,
                                        Anki::TimeStamp_t& fakeTime,
                                        Anki::Cozmo::Robot& robot,
                                        Anki::Cozmo::RobotState& stateMsg,
                                        Anki::Cozmo::VisionProcessingResult& procResult)
{
  using namespace Anki;
  
  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    stateMsg.timestamp = fakeTime;
    stateMsg.pose_frame_id = robot.GetPoseFrameID();
    stateMsg.pose_origin_id = robot.GetPoseOriginList().GetCurrentOriginID();
    Result lastResult = robot.UpdateFullRobotState(stateMsg);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("ObservedMarkerHelper.UpdateFullRobotStateFailed", "i=%d fakeTime=%u", i, fakeTime);
      return lastResult;
    }
   
    procResult.timestamp = fakeTime;
    procResult.observedMarkers.clear();
    for(auto & codeAndCorners : codesAndCorners)
    {
      procResult.observedMarkers.push_back(Vision::ObservedMarker(fakeTime, codeAndCorners.first, codeAndCorners.second,
                                                                  robot.GetVisionComponent().GetCamera()));
    }
    
    lastResult = robot.GetVisionComponent().UpdateVisionMarkers(procResult);
    
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("ObservedMarkerHelper.UpdateVisionMarkersFailed", "i=%d fakeTime=%u", i, fakeTime);
      return lastResult;
    }
  }
 
  return RESULT_OK;
}

// Helper method for BlockWorld.UpdateObjectOrigins Test
static Anki::Result FakeRobotMovement(Anki::Cozmo::Robot& robot,
                                      Anki::Cozmo::RobotState& stateMsg,
                                      Anki::TimeStamp_t& fakeTime)
{
  using namespace Anki;
  using namespace Cozmo;
  
  stateMsg.timestamp = fakeTime;
  stateMsg.status |= (u16)RobotStatusFlag::ARE_WHEELS_MOVING; // Set moving flag
  Result lastResult = robot.UpdateFullRobotState(stateMsg);
  stateMsg.status &= ~(u16)RobotStatusFlag::ARE_WHEELS_MOVING; // Unset moving flag
  fakeTime += 10;
  
  return lastResult;
}

TEST(BlockWorld, UpdateObjectOrigins)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  {
    // no connected objects either
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(BlockWorldFilter(), activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // For faking observations of two blocks, one closer, one far
  const ObjectType farType = ObjectType::Block_LIGHTCUBE1;
  const ObjectType closeType = ObjectType::Block_LIGHTCUBE2;
  const Block_Cube1x1 farCube(farType);
  const Block_Cube1x1 closeCube(closeType);
  const Vision::Marker::Code farCode   = farCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code closeCode = closeCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  const Quad2f farCorners{
    Point2f(213,111),  Point2f(214,158),  Point2f(260,111),  Point2f(258,158)
  };
  
  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  const ObjectID farID = robot.GetBlockWorld().AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(farID.IsSet());
  
  const ObjectID closeID = robot.GetBlockWorld().AddConnectedActiveObject(1, 1, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(closeID.IsSet());
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);

  VisionProcessingResult procResult;
  
  // See far block and localize to it.
  TimeStamp_t fakeTime = 10;
  
  // After seeing three times, should be Known and localizable
  const s32 kNumObservations = 5;
  
  lastResult = ObserveMarkerHelper(kNumObservations, {{farCode, farCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have a "far" object present in block world now
  filter.SetAllowedTypes({farType});
  const ObservableObject* matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  
  const ObjectID obsFarID = matchingObject->GetID();
  ASSERT_EQ(farID, obsFarID);

  // active match
  ActiveObject* connectedFar = robot.GetBlockWorld().GetConnectedActiveObjectByID(farID);
  ASSERT_NE(nullptr, connectedFar);
  ASSERT_EQ(connectedFar->GetActiveID(), matchingObject->GetActiveID());
  
  // Should be localized to "far" object
  ASSERT_EQ(farID, robot.GetLocalizedTo());
  
  // Delocalize the robot
  const bool isCarryingObject = false;
  robot.Delocalize(isCarryingObject);
  
  // Now just see the close block by itself
  lastResult = ObserveMarkerHelper(kNumObservations, {{closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have a "close" object present
  filter.SetAllowedTypes({closeType});
  matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  
  const ObjectID obsCloseID = matchingObject->GetID();
  ASSERT_EQ(closeID, obsCloseID);
  
  // active match
  ActiveObject* connectedClose = robot.GetBlockWorld().GetConnectedActiveObjectByID(closeID);
  ASSERT_NE(nullptr, connectedClose);
  ASSERT_EQ(connectedClose->GetActiveID(), matchingObject->GetActiveID());
  
  // Should be localized to "close" object
  ASSERT_EQ(closeID, robot.GetLocalizedTo());
  
  // Now let the robot see both objects at the same time
  lastResult = ObserveMarkerHelper(kNumObservations, {{farCode, farCorners}, {closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have both objects
  filter.SetAllowedTypes({farType, closeType});
  std::vector<ObservableObject*> matchingObjects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects);
  ASSERT_EQ(2, matchingObjects.size());
  
  // Far object should now be Known pose state
  matchingObject = robot.GetBlockWorld().GetLocatedObjectByID(farID);
  ASSERT_NE(nullptr, matchingObject);
  ASSERT_TRUE(matchingObject->IsPoseStateKnown());
  
  // "Move" the robot so it will relocalize
  lastResult = FakeRobotMovement(robot, stateMsg, fakeTime);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Seeing both objects again, now that we've moved, should relocalize (and not crash!)
  lastResult = ObserveMarkerHelper(1, {{farCode, farCorners}, {closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should end up localized to the close object
  ASSERT_EQ(closeID, robot.GetLocalizedTo());
  
} // BlockWorld.UpdateObjectOrigins

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// helper for device connection messages, for example when cubes connect/disconnect. Note implementation directly
// calls the robot handler, rather than simulating actually sending a message
using namespace Anki::Cozmo;
void FakeRecvConnectionMessage(Robot& robot, double time, uint32_t activeID, uint32_t factoryID, Anki::Cozmo::ObjectType objectType, bool connected)
{
  DEV_ASSERT(IsValidLightCube(objectType, false), "FaceRecvConnectionMessage.UnsupportedObjectType");
  
  if (connected) {
    robot.GetBlockWorld().AddConnectedActiveObject(activeID, factoryID, objectType);
  } else {
    robot.GetBlockWorld().RemoveConnectedActiveObject(activeID);
  }
}

// helper for move messages
void FakeRecvMovedMessage(Robot& robot, double time, Anki::TimeStamp_t timestamp, uint32_t activeID)
{
  const auto& msg = ObjectMoved(timestamp, activeID, ActiveAccel(1,1,1), Anki::Cozmo::UpAxis::ZPositive );
  robot.GetRobotToEngineImplMessaging().HandleActiveObjectMoved(msg, &robot);
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, PoseUpdates)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  {
    // no connected objects either
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(BlockWorldFilter(), activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // Fake a state message update for robot
  RobotState stateMsg = robot.GetDefaultRobotState();
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  /*
    + See unique    object1 (non-localizable) from close, seeing same object in *different pose from far
      | - It should override the pose
    + See connected object2 (localizable), seeing same object in *different pose from far
      | - It should NOT override the pose
    + See connected object3 (localizable), seeing same object in *different pose from far
      | - It should NOT override the pose
    + Move object2 & see object 2
      | - It should override the pose
    + Move object3
      | - It should NOT override the pose
    + Disconnect active object3 & see object3
      | - It should override the pose
  */
  
  // For faking observations of blocks
  const ObjectType obj1Type = ObjectType::Block_LIGHTCUBE1;
  const ObjectType obj2Type = ObjectType::Block_LIGHTCUBE2;
  const ObjectType obj3Type = ObjectType::Block_LIGHTCUBE3;
  const Block_Cube1x1 obj1Cube(obj1Type);
  const Block_Cube1x1 obj2Cube(obj2Type);
  const Block_Cube1x1 obj3Cube(obj3Type);
  const Vision::Marker::Code obj1Code = obj1Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code obj2Code = obj2Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code obj3Code = obj3Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
//  const Quad2f farCornersLocalizable {
//    Point2f(213,111),  Point2f(214,158),  Point2f(260,111),  Point2f(258,158)
//  };

  // These parameters create corners beyond localizable distance
  const f32 halfHeight = 0.05f*static_cast<f32>(camCalib->GetNrows());
  const f32 halfWidth = 0.05f*static_cast<f32>(camCalib->GetNcols());
  const f32 xcen = camCalib->GetCenter_x();
  const f32 ycen = camCalib->GetCenter_y();
  Quad2f farCorners;
  const f32 markerHalfSize = std::min(halfHeight, halfWidth);
  farCorners[Quad::TopLeft]    = {xcen - markerHalfSize, ycen - markerHalfSize};
  farCorners[Quad::BottomLeft] = {xcen - markerHalfSize, ycen + markerHalfSize};
  farCorners[Quad::TopRight]   = {xcen + markerHalfSize, ycen - markerHalfSize};
  farCorners[Quad::BottomRight]= {xcen + markerHalfSize, ycen + markerHalfSize};
  
  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  // - - - Connect to some cubes

  // Do not connect object1
  //  const ObjectID connObj1 = robot.GetBlockWorld().AddConnectedActiveObject(0, 100, ObjectType::Block_LIGHTCUBE1);
  //  ASSERT_TRUE(connObj1.IsSet());
  
  const ObjectID connObj2 = robot.GetBlockWorld().AddConnectedActiveObject(1, 101, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(connObj2.IsSet());
  
  const ObjectID connObj3 = robot.GetBlockWorld().AddConnectedActiveObject(2, 102, ObjectType::Block_LIGHTCUBE3);
  ASSERT_TRUE(connObj3.IsSet());

  // - - - See all objects from close so that their poses are Known
  
  VisionProcessingResult procResult;
  TimeStamp_t fakeTime = 10;
  
  // After seeing at least 2 times, should be Known
  const s32 kNumObservations = 5;
  
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj1Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj3Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  fakeTime += 10;
  
  // see object 1 from far, it should update its pose, since it's not localizable
  {
    filter.SetAllowedTypes({obj1Type});
    const ObservableObject* matchingObject1 = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    ASSERT_NE(nullptr, matchingObject1);
    
    const Pose3d prevObj1Pose = matchingObject1->GetPose();
    
    lastResult = ObserveMarkerHelper(kNumObservations, {{obj1Code, farCorners}}, fakeTime, robot, stateMsg, procResult);
    const Pose3d& newObj1Pose = matchingObject1->GetPose();
    const bool isSameNewToPrev = newObj1Pose.IsSameAs(prevObj1Pose, matchingObject1->GetSameDistanceTolerance(), matchingObject1->GetSameAngleTolerance());
    ASSERT_FALSE(isSameNewToPrev); // DIFFERENT
  }

  // see object 2 from far, it should NOT update its pose, since it's localizable
  {
    filter.SetAllowedTypes({obj2Type});
    const ObservableObject* matchingObject2 = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    ASSERT_NE(nullptr, matchingObject2);
    const ObjectID& conMatch2 = matchingObject2->GetID();
    ASSERT_EQ(conMatch2, connObj2);

    // should become localize to 2
    const bool localizedTo2 = (robot.GetLocalizedTo() == conMatch2);
    ASSERT_TRUE(localizedTo2);
  
    const Pose3d prevObj2Pose = matchingObject2->GetPose();

    lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, farCorners}}, fakeTime, robot, stateMsg, procResult);
    const Pose3d& newObj2Pose = matchingObject2->GetPose();
    const bool isSameNewToPrev = newObj2Pose.IsSameAs(prevObj2Pose, matchingObject2->GetSameDistanceTolerance(), matchingObject2->GetSameAngleTolerance());
    ASSERT_TRUE(isSameNewToPrev); // SAME
  }
  
  // see object 3 from far, it should NOT update its pose, since it's localizable
  {
    filter.SetAllowedTypes({obj3Type});
    const ObservableObject* matchingObject3 = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    ASSERT_NE(nullptr, matchingObject3);
    const ObjectID& conMatch3 = matchingObject3->GetID();
    ASSERT_EQ(conMatch3, connObj3);
  
    const Pose3d prevObj3Pose = matchingObject3->GetPose();

    lastResult = ObserveMarkerHelper(kNumObservations, {{obj3Code, farCorners}}, fakeTime, robot, stateMsg, procResult);
    const Pose3d& newObj3Pose = matchingObject3->GetPose();
    const bool isSameNewToPrev = newObj3Pose.IsSameAs(prevObj3Pose, matchingObject3->GetSameDistanceTolerance(), matchingObject3->GetSameAngleTolerance());
    ASSERT_TRUE(isSameNewToPrev); // SAME
  }

  // MOVE object2
  {
    const ActiveObject* con2 = robot.GetBlockWorld().GetConnectedActiveObjectByID( connObj2 );
    FakeRecvMovedMessage(robot, fakeTime, fakeTime, con2->GetActiveID());
    ++fakeTime;
  }
  
  // move object 2, and do the same observations. If moved, then the pose should be updated, since it's not localizable
  {
    filter.SetAllowedTypes({obj2Type});
    const ObservableObject* matchingObject2 = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    ASSERT_NE(nullptr, matchingObject2);
    const ObjectID& conMatch2 = matchingObject2->GetID();
    ASSERT_EQ(conMatch2, connObj2);
    
    const Pose3d prevObj2Pose = matchingObject2->GetPose();

    lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, farCorners}}, fakeTime, robot, stateMsg, procResult);
    const Pose3d& newObj2Pose = matchingObject2->GetPose();
    const bool isSameNewToPrev = newObj2Pose.IsSameAs(prevObj2Pose, matchingObject2->GetSameDistanceTolerance(), matchingObject2->GetSameAngleTolerance());
    ASSERT_FALSE(isSameNewToPrev); // DIFFERENT
  }

  // DISCONNECT object3
  {
    const ActiveObject* con3 = robot.GetBlockWorld().GetConnectedActiveObjectByID( connObj3 );
    FakeRecvConnectionMessage(robot, fakeTime, con3->GetActiveID(), con3->GetFactoryID(), Anki::Cozmo::ObjectType::Block_LIGHTCUBE2, false);
    ++fakeTime;
  }
  
  // disconnect object 3, and do the same observatins. If disconnected, the pose should be updated, since it's not localizable
  {
    filter.SetAllowedTypes({obj3Type});
    const ObservableObject* matchingObject3 = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
    ASSERT_NE(nullptr, matchingObject3);
    const ObjectID& conMatch3 = matchingObject3->GetID();
    ASSERT_EQ(conMatch3, connObj3);
  
    const Pose3d prevObj3Pose = matchingObject3->GetPose();

    lastResult = ObserveMarkerHelper(kNumObservations, {{obj3Code, farCorners}}, fakeTime, robot, stateMsg, procResult);
    const Pose3d& newObj3Pose = matchingObject3->GetPose();
    const bool isSameNewToPrev = newObj3Pose.IsSameAs(prevObj3Pose, matchingObject3->GetSameDistanceTolerance(), matchingObject3->GetSameAngleTolerance());
    ASSERT_FALSE(isSameNewToPrev); // DIFFERENT
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, RejiggerAndObserveAtSameTick)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  {
    // no connected objects either
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(BlockWorldFilter(), activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  // For faking observations of blocks
  const Block_Cube1x1 obj1Cube(ObjectType::Block_LIGHTCUBE1);
  const Block_Cube1x1 obj2Cube(ObjectType::Block_LIGHTCUBE2);
  const Block_Cube1x1 obj3Cube(ObjectType::Block_LIGHTCUBE3);
  const Vision::Marker::Code obj1Code = obj1Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code obj2Code = obj2Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code obj3Code = obj3Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  // - - - Connect to some cubes
  
  const ObjectID connObj1 = robot.GetBlockWorld().AddConnectedActiveObject(0, 100, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(connObj1.IsSet());
  
  const ObjectID connObj2 = robot.GetBlockWorld().AddConnectedActiveObject(1, 101, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(connObj2.IsSet());
  
  const ObjectID connObj3 = robot.GetBlockWorld().AddConnectedActiveObject(2, 102, ObjectType::Block_LIGHTCUBE3);
  ASSERT_TRUE(connObj3.IsSet());

  // - - - See all objects from close so that their poses are Known
  
  VisionProcessingResult procResult;
  TimeStamp_t fakeTime = 10;
  
  // After seeing at least 2 times, should be Known
  const s32 kNumObservations = 5;
  
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj1Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  // Do not add object 3 so that we need to confirm it later
  //  lastResult = ObserveMarkerHelper(kNumObservations, {{obj3Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  //  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Observe a face
  const Vision::FaceID_t faceID = 123;
  {
    Vision::TrackedFace face;
    Pose3d headPose(0, Z_AXIS_3D(), {300.f, 300.f, 300.f});
    face.SetID(faceID);
    face.SetTimeStamp(fakeTime);
    face.SetHeadPose(headPose);
    
    std::list<Vision::TrackedFace> faces{std::move(face)};
    
    // Need a state message for the observation time first
    stateMsg.timestamp = fakeTime;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    
    lastResult = robot.GetFaceWorld().Update(faces);
    ASSERT_EQ(RESULT_OK, lastResult);
    ASSERT_TRUE(robot.GetFaceWorld().HasAnyFaces());
    ASSERT_NE(nullptr, robot.GetFaceWorld().GetFace(faceID));
    ASSERT_EQ(1, robot.GetFaceWorld().GetFaceIDs().size());
    ASSERT_EQ(1, robot.GetFaceWorld().GetFaceIDsObservedSince(fakeTime).size());
    ASSERT_TRUE(robot.GetFaceWorld().GetFaceIDsObservedSince(fakeTime + 10).empty());
  }
  
  fakeTime += 10;
  
  // - - - Delocalize
  
  const bool isCarryingObject = false;
  robot.Delocalize(isCarryingObject);
  ++fakeTime;
  
  // FaceWorld should no longer return anything because we have a new origin
  {
    ASSERT_FALSE(robot.GetFaceWorld().HasAnyFaces());
    ASSERT_EQ(nullptr, robot.GetFaceWorld().GetFace(faceID));
    ASSERT_TRUE(robot.GetFaceWorld().GetFaceIDs().empty());
  }
  
  // See all objects from close so they all have unconfirmed observations
  lastResult = ObserveMarkerHelper(1,
                                   {{obj1Code, closeCorners}, {obj2Code, closeCorners}, {obj3Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // now see each cube separately
  
  // 1 will rejigger
  lastResult = ObserveMarkerHelper(1,{{obj1Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have face again because we are back in its coordinate frame
  {
    ASSERT_TRUE(robot.GetFaceWorld().HasAnyFaces());
    ASSERT_NE(nullptr, robot.GetFaceWorld().GetFace(faceID));
    ASSERT_FALSE(robot.GetFaceWorld().GetFaceIDs().empty());
  }
  
  // we should have objects 1 and 2, but not 3, since 3 was not in the world before delocalizing
  const ObservableObject* obj1Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj1);
  ASSERT_NE(obj1Ptr, nullptr);
  const ObservableObject* obj2Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj2);
  ASSERT_NE(obj2Ptr, nullptr);
  const ObservableObject* obj3Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj3);
  ASSERT_EQ(obj3Ptr, nullptr);
  
  // Add 1 confirmation for 2 and 3
  lastResult = ObserveMarkerHelper(1,{{obj2Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  lastResult = ObserveMarkerHelper(1,{{obj3Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // still should have 1 and 2, but not 3
  obj1Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj1);
  ASSERT_NE(obj1Ptr, nullptr);
  obj2Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj2);
  ASSERT_NE(obj2Ptr, nullptr);
  obj3Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj3);
  ASSERT_EQ(obj3Ptr, nullptr);
  
  // Add 1 confirmation for 2 and 3
  lastResult = ObserveMarkerHelper(1,{{obj2Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  lastResult = ObserveMarkerHelper(1,{{obj3Code, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // now 3 should be added (assumes we need 2 visual observations to confirm)
  obj1Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj1);
  ASSERT_NE(obj1Ptr, nullptr);
  obj2Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj2);
  ASSERT_NE(obj2Ptr, nullptr);
  obj3Ptr = robot.GetBlockWorld().GetLocatedObjectByID(connObj3);
  ASSERT_NE(obj3Ptr, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, RejiggerAndFlatten)
{
  // See object 1 in origin A
  // Delocalize, now in origin B
  // See object 2 in origin B
  // Delocalize, now in origin C
  // Create pose w.r.t. C
  // See object 2 again, now in origin B
  // See object 1 again, now in origin A
  // C should now be flattened w.r.t. A
  // Receiving a state message referencing old origin C should not cause issues
  
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  {
    // no connected objects either
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(BlockWorldFilter(), activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  // For faking observations of blocks
  const Block_Cube1x1 obj1Cube(ObjectType::Block_LIGHTCUBE1);
  const Block_Cube1x1 obj2Cube(ObjectType::Block_LIGHTCUBE2);

  const Vision::Marker::Code obj1Code = obj1Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code obj2Code = obj2Cube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  // Connect to some cubes
  
  const ObjectID connObj1 = robot.GetBlockWorld().AddConnectedActiveObject(0, 100, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(connObj1.IsSet());
  
  const ObjectID connObj2 = robot.GetBlockWorld().AddConnectedActiveObject(1, 101, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(connObj2.IsSet());
  
  // - - - See all objects from close so that their poses are Known and we can localize to them
  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  VisionProcessingResult procResult;
  TimeStamp_t fakeTime = 10;
  
  const PoseOriginID_t originA = robot.GetWorldOriginID();
  
  ASSERT_EQ(1, robot.GetPoseOriginList().GetSize());
  
  // See object 1
  const s32 kNumObservations = 5; // After seeing at least 2 times, should be Known
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj1Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should be localized to object 1, still in originA
  ASSERT_EQ(connObj1, robot.GetLocalizedTo());
  ASSERT_EQ(originA, robot.GetWorldOriginID());
  
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // Delocalize
  const bool isCarryingObject = false;
  robot.Delocalize(isCarryingObject);
  ++fakeTime;
  
  const PoseOriginID_t originB = robot.GetWorldOriginID();
  ASSERT_NE(originA, originB);
  ASSERT_EQ(2, robot.GetPoseOriginList().GetSize());
  ASSERT_NE(connObj1, robot.GetLocalizedTo());
  
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // See object 2
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should now be localized to object 2, still in origin B
  ASSERT_EQ(connObj2, robot.GetLocalizedTo());
  ASSERT_EQ(originB, robot.GetWorldOriginID());
  
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // Delocalize again
  robot.Delocalize(isCarryingObject);
  ++fakeTime;
  
  const PoseOriginID_t originC = robot.GetWorldOriginID();
  ASSERT_NE(originA, originC);
  ASSERT_NE(originB, originC);
  ASSERT_NE(connObj2, robot.GetLocalizedTo());
  ASSERT_EQ(3, robot.GetPoseOriginList().GetSize());
  
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // Create some arbitrary pose in this origin
  const Pose3d somePose(DEG_TO_RAD(45), Z_AXIS_3D(), {100.f, 200.f, 300.f}, robot.GetWorldOrigin());
  
  // See object 2 again, should be back in originB
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj2Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  ASSERT_EQ(originB, robot.GetWorldOriginID());
  ASSERT_EQ(connObj2, robot.GetLocalizedTo());
  ASSERT_EQ(3, robot.GetPoseOriginList().GetSize());
  
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // After rejigger, origin C should now have B as its parent
  ASSERT_TRUE(robot.GetPoseOriginList().GetOriginByID(originC).IsChildOf(robot.GetPoseOriginList().GetOriginByID(originB)));
  
  // "Move" the robot so it will relocalize
  lastResult = FakeRobotMovement(robot, stateMsg, fakeTime);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // See object 1 again, should be back in origin A
  lastResult = ObserveMarkerHelper(kNumObservations, {{obj1Code, closeCorners}}, fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  ASSERT_EQ(originA, robot.GetWorldOriginID());
  ASSERT_EQ(connObj1, robot.GetLocalizedTo());
  ASSERT_EQ(3, robot.GetPoseOriginList().GetSize());
  
  // We should have flattened C to be w.r.t. A now. B will be rejiggered to be parented to A as well.
  ASSERT_TRUE(robot.GetPoseOriginList().GetOriginByID(originC).IsChildOf(robot.GetPoseOriginList().GetOriginByID(originA)));
  ASSERT_TRUE(robot.GetPoseOriginList().GetOriginByID(originB).IsChildOf(robot.GetPoseOriginList().GetOriginByID(originA)));
  ASSERT_TRUE(robot.GetPoseOriginList().SanityCheckOwnership());
  
  // Our arbitrary pose should still have a valid parent
  ASSERT_EQ(originC, somePose.GetParent().GetID());
  ASSERT_EQ(originA, somePose.GetRootID());
  
  // Getting robot state will create a PoseStruct3d, which should work just fine, even after rejiggering/flattening
  ASSERT_EQ(originA, robot.GetRobotState().pose.originID);
  
  // Receive state message with pose information referencing old origin C
  stateMsg.pose_origin_id = originC;
  stateMsg.timestamp = fakeTime;
  
  // Using that message should still be kosher
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  ASSERT_EQ(originA, robot.GetPose().GetRootID());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, LocalizedObjectDisconnect)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // For faking observations of a cube
  const ActiveID closeActiveID = 1;
  const FactoryID closeFactoryID = 1;
  const ObjectType closeType = ObjectType::Block_LIGHTCUBE2;
  const Block_Cube1x1 closeCube(closeType);
  const Vision::Marker::Code closeCode = closeCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  TimeStamp_t fakeTime = 10;

  // connect to cube
  FakeRecvConnectionMessage(robot, fakeTime, closeActiveID, closeFactoryID, closeType, true);
  ++fakeTime;
  
  // Should have a "close" object connected
  filter.SetAllowedTypes({closeType});
  filter.SetFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
  const ObservableObject* matchingObject = robot.GetBlockWorld().FindConnectedActiveMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  ASSERT_TRUE(matchingObject->GetID().IsSet());
  ASSERT_FALSE(matchingObject->HasValidPose());
  ASSERT_EQ(closeActiveID, matchingObject->GetActiveID());
  
  // capture the ID so we can compare later
  const ObjectID blockObjectID = matchingObject->GetID();
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);

  VisionProcessingResult procResult;
  
  // After seeing three times, should be Known and localizable
  const s32 kNumObservations = 5;
  
  // see the close block by itself
  lastResult = ObserveMarkerHelper(kNumObservations, {{closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should be localized to "close" object
  filter.SetAllowedTypes({closeType});
  matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  ASSERT_EQ(blockObjectID, matchingObject->GetID());
  ASSERT_EQ(closeActiveID, matchingObject->GetActiveID());
  ASSERT_TRUE(matchingObject->IsPoseStateKnown());
  ASSERT_EQ(blockObjectID, robot.GetLocalizedTo());
  
  // disconnect from the cube
  FakeRecvConnectionMessage(robot, fakeTime, closeActiveID, closeFactoryID, closeType, false);
  ++fakeTime;
  
  // delocalize while the cube is disconnected
  // this causes the memory map to be destroyed, since there are no localizable cubes available
  const bool isCarryingObject = false;
  robot.Delocalize(isCarryingObject);
  ++fakeTime;
  
  // reconnect to the cube
  FakeRecvConnectionMessage(robot, fakeTime, closeActiveID, closeFactoryID, closeType, true);
  ++fakeTime;
  
  // see the cube again
  lastResult = ObserveMarkerHelper(kNumObservations, {{closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have the object back
  filter.SetAllowedTypes({closeType});
  std::vector<ObservableObject*> matchingObjects;
  robot.GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects);
  ASSERT_EQ(1, matchingObjects.size());

  // Close object should now be Known pose state
  matchingObject = robot.GetBlockWorld().GetLocatedObjectByID(blockObjectID);
  ASSERT_NE(nullptr, matchingObject);
  ASSERT_TRUE(matchingObject->IsPoseStateKnown());
  
  // "Move" the robot so it will relocalize
  lastResult = FakeRobotMovement(robot, stateMsg, fakeTime);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Seeing both objects again, now that we've moved, should relocalize (and not crash!)
  lastResult = ObserveMarkerHelper(kNumObservations, {{closeCode, closeCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should end up localized to the close object
  ASSERT_EQ(blockObjectID, robot.GetLocalizedTo());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// Helpers can't have TEST assertions
ObservableObject* CreateObjectLocatedAtOrigin(Robot& robot, ObjectType objectType)
{
  // matching activeID happens through objectID automatically on addition
  const ActiveID activeID = -1;
  const FactoryID factoryID = 0;

  BlockWorld& blockWorld = robot.GetBlockWorld();
  Anki::Cozmo::ObservableObject* objectPtr = CreateActiveObjectByType(objectType, activeID, factoryID);
  DEV_ASSERT(nullptr != objectPtr, "CreateObjectLocatedAtOrigin.CreatedNull");
  
  // check it currently doesn't exist in BlockWorld
  {
    BlockWorldFilter filter;
    filter.SetFilterFcn(nullptr); // TODO Should not be needed by default
    filter.SetAllowedTypes( {objectPtr->GetType()} );
    filter.SetAllowedFamilies( {objectPtr->GetFamily()} );
    ObservableObject* sameBlock = blockWorld.FindLocatedMatchingObject(filter);
    DEV_ASSERT(nullptr == sameBlock, "CreateObjectLocatedAtOrigin.TypeAlreadyInUse");
  }
  
  // set initial pose & ID (that's responsibility of the system creating the object)
  DEV_ASSERT(!objectPtr->GetID().IsSet(), "CreateObjectLocatedAtOrigin.IDSet");
  DEV_ASSERT(!objectPtr->HasValidPose(), "CreateObjectLocatedAtOrigin.HasValidPose");
  objectPtr->SetID();
  Anki::Pose3d originPose;
  originPose.SetParent( robot.GetWorldOrigin() );
  objectPtr->InitPose( originPose, Anki::PoseState::Known); // posestate could be something else
  
  // now they can be added to the world
  blockWorld.AddLocatedObject(std::shared_ptr<ObservableObject>(objectPtr));

  // need to pretend we observed this object
  robot.GetObjectPoseConfirmer().AddInExistingPose(objectPtr); // this has to be called after AddLocated just because
  
  // verify they are there now
  DEV_ASSERT(objectPtr->GetID().IsSet(), "CreateObjectLocatedAtOrigin.IDNotset");
  DEV_ASSERT(objectPtr->HasValidPose(), "CreateObjectLocatedAtOrigin.InvalidPose");
  ObservableObject* objectInWorld = blockWorld.GetLocatedObjectByID( objectPtr->GetID() );
  DEV_ASSERT(objectInWorld == objectPtr, "CreateObjectLocatedAtOrigin.NotSameObject");
  DEV_ASSERT(nullptr != objectInWorld, "CreateObjectLocatedAtOrigin.NullWorldPointer");
  return objectInWorld;
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, CubeStacks)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  {
    // no connected objects either
    BlockWorldFilter filter;
    std::vector<ActiveObject*> activeObjects;
    blockWorld.FindConnectedActiveMatchingObjects(filter, activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }
  
  // Create three objects
  ObservableObject* object1 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  ObservableObject* object2 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE2);
  ObservableObject* object3 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE3);
  
  {
    // verify they are there now
    BlockWorldFilter filter;
    objects.clear();
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_EQ(3, objects.size());
  }
  
  // Put object 2 on top of object 1 and check that FindOnTopOf and FindUnderneath
  // return the right things. We use ObjectPoseConfirmer because we can't set
  // poses of the objects directly. This should work for lots of combinations of
  // rotations and shifts.
  // Object 3 will be next to the object on bottom and it should never be returned
  // as on top of or underneath.
  const std::vector<RotationVector3d> TestInPlaneRotations{
    {DEG_TO_RAD(0),  Z_AXIS_3D()},
    {DEG_TO_RAD(10), Z_AXIS_3D()},
    {DEG_TO_RAD(45), Z_AXIS_3D()},
    {DEG_TO_RAD(90), Z_AXIS_3D()},
  };
  
  const std::vector<RotationVector3d> TestOutOfPlaneRotations{
    {DEG_TO_RAD(0),   X_AXIS_3D()},  // None
    {DEG_TO_RAD(5),   X_AXIS_3D()},  // Slightly rotated around X
    {DEG_TO_RAD(7.5f), Y_AXIS_3D()},  // Slightly rotated around Y
    {DEG_TO_RAD(88),  X_AXIS_3D()},  // Z not pointing up
    {DEG_TO_RAD(182), Y_AXIS_3D()},  // Z pointing down
  };
  
  const std::vector<Vec3f> TestBottomTranslations{
    {100.f,  0.f, 22.f},
    {106.f, -8.f, 21.f},
    {98.f, -2.3f, 18.f},
  };
  
  const std::vector<Vec3f> TestTopTranslations{
    {100.f, 0.f,  66.f},                             // Centered, right on top
    {105.f, -5.f, 66.f+.25f*STACKED_HEIGHT_TOL_MM},  // Slightly displaced, a little high
    {90.f, -8.3f, 66.f-.25f*STACKED_HEIGHT_TOL_MM},  // Signigicantly displaced, a little low
  };
  
  const std::vector<Vec3f> TestNextToTranslations{
    {144.f,   0.f, 21.f},  // On one side in X direction
    {66.f,    0.f, 20.f},  // On other side in X direction
    {100.f,  44.f, 23.f},  // On one side in Y direction
    {100.f, -44.f, 24.f},  // On other side in Y direction
  };

  // rsam found a test case in which 2 cubes would find the other one to be on top, potentially causing
  // a loop trying to build stacks. The poses below are a a set that caused the issue.
  {
    Vec3f trans1{100,0,0};
    Rotation3d rotZ1 = {DEG_TO_RAD(0),  Z_AXIS_3D()};
    Rotation3d rotX1 = {DEG_TO_RAD(-45),  X_AXIS_3D()};
    const Pose3d object1Pose(rotZ1 * rotX1, trans1, robot.GetPose() );
    lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, object1Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, lastResult);

    Vec3f trans2{100,0,0};
    Rotation3d rotZ2 = {DEG_TO_RAD(0),  Z_AXIS_3D()};
    Rotation3d rotX2 = {DEG_TO_RAD(-45),  X_AXIS_3D()};
    const Pose3d object2Pose(rotZ2 * rotX2, trans2, robot.GetPose() );
    lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, object2Pose, PoseState::Known);
    ASSERT_EQ(RESULT_OK, lastResult);

    ObservableObject* foundObject1 = blockWorld.FindLocatedObjectOnTopOf(*object1, 2*STACKED_HEIGHT_TOL_MM);
    ASSERT_EQ(nullptr, foundObject1);
    ASSERT_NE(object2, foundObject1);

    ObservableObject* foundObject2 = blockWorld.FindLocatedObjectOnTopOf(*object2, 2*STACKED_HEIGHT_TOL_MM);

    ASSERT_EQ(nullptr, foundObject2);
    ASSERT_NE(object1, foundObject2);
  }
  
  for(auto & btmRot1 : TestInPlaneRotations)
  {
    for(auto & btmRot2 : TestOutOfPlaneRotations)
    {
      for(auto & btmTrans : TestBottomTranslations)
      {
        const Pose3d bottomPose(Rotation3d(btmRot1) * Rotation3d(btmRot2), btmTrans, robot.GetPose() );
        
        lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, bottomPose, PoseState::Known);
        ASSERT_EQ(RESULT_OK, lastResult);
        
        for(auto & topRot1 : TestInPlaneRotations)
        {
          for(auto & topRot2 : TestOutOfPlaneRotations)
          {
            for(auto & topTrans : TestTopTranslations)
            {
              const Pose3d topPose(Rotation3d(topRot1) * Rotation3d(topRot2), topTrans, robot.GetPose() );
              
              // For help debugging when there are failures:
              //bottomPose.Print("Unnamed", "Bottom");
              //topPose.Print("Unnamed", "Top");

              lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, topPose, object1);
              ASSERT_EQ(RESULT_OK, lastResult);
              
              ObservableObject* foundObject = blockWorld.FindLocatedObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
              ASSERT_NE(nullptr, foundObject);
              ASSERT_EQ(object2, foundObject);
              
              foundObject = blockWorld.FindLocatedObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
              ASSERT_NE(nullptr, foundObject);
              ASSERT_EQ(object1, foundObject);
            }
            
            for(auto & nextToTrans : TestNextToTranslations)
            {
              const Pose3d nextToPose(Rotation3d(topRot1) * Rotation3d(topRot2), nextToTrans, robot.GetPose());
              
              lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, nextToPose, object1);
              ASSERT_EQ(RESULT_OK, lastResult);
              
              ObservableObject* foundObject = blockWorld.FindLocatedObjectOnTopOf(*object3, STACKED_HEIGHT_TOL_MM);
              ASSERT_EQ(nullptr, foundObject);
              
              foundObject = blockWorld.FindLocatedObjectUnderneath(*object3, STACKED_HEIGHT_TOL_MM);
              ASSERT_EQ(nullptr, foundObject);
            }
          }
        }
      }
    }
  }
  
  
  // Put Object 2 above Object 1, but too high, so this Find should fail:
  const Pose3d bottomPose(0, Z_AXIS_3D(), {100.f, 0.f, 22.f}, robot.GetPose(), "BottomPose" );
  lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, bottomPose, PoseState::Known);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  const Pose3d tooHighPose(0, Z_AXIS_3D(), {100.f, 0.f, 66.f + 1.5f*STACKED_HEIGHT_TOL_MM}, robot.GetPose(), "TooHighPose" );
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, tooHighPose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  ObservableObject* foundObject = blockWorld.FindLocatedObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindLocatedObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);

  // Two objects in roughly the same place should also fail
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, bottomPose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  foundObject = blockWorld.FindLocatedObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindLocatedObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  
  // Put Object 2 at the right height to be on top of Object 1,
  // but move it off to the side so that the quads don't intersect
  const Pose3d notAbovePose(0, Z_AXIS_3D(), {130.f, -45.f, 66.f}, robot.GetPose(), "NotAbovePose");
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, notAbovePose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  foundObject = blockWorld.FindLocatedObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindLocatedObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
} // BlockWorld.CubeStacks

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// VIC-12: Disabled because test does not pass reliably
//
TEST(BlockWorld, DISABLED_UnobserveCubeStack)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  std::vector<ObservableObject*> objects;

  // Create objects
  ObservableObject* object1 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  ObservableObject* object2 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE2);
  
  {
    // verify they are there now
    BlockWorldFilter filter;
    objects.clear();
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_EQ(2, objects.size());
  }
  
  Vec3f testBottomTranslation{175.f,  0.f, 22.f};
  Vec3f testTopTranslation{175.f, 0.f,  66.f};
  
  Rotation3d rotZ1 = {DEG_TO_RAD(0),  Z_AXIS_3D()};
  const Pose3d object1Pose(rotZ1, testBottomTranslation, robot.GetPose() );
  lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, object1Pose, PoseState::Known);
  ASSERT_EQ(RESULT_OK, lastResult);

  const Pose3d object2Pose(rotZ1, testTopTranslation, robot.GetPose() );
  lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object2, object2Pose, PoseState::Known);
  ASSERT_EQ(RESULT_OK, lastResult);

  ObservableObject* foundObject1 = blockWorld.FindLocatedObjectOnTopOf(*object1, 2*STACKED_HEIGHT_TOL_MM);
  ASSERT_NE(nullptr, foundObject1);
  ASSERT_EQ(object2, foundObject1);

  ObservableObject* foundObject2 = blockWorld.FindLocatedObjectOnTopOf(*object2, 2*STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject2);
  ASSERT_NE(object1, foundObject2);
  
  // mark bottom as dirty, propagating up
  robot.GetObjectPoseConfirmer().MarkObjectDirty(object1, true);
  
  // unobserve objects, expecting them to be deleted
  std::list<Vision::ObservedMarker> emptyMarkersList;
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  for(s32 i=0; i<5; ++i)
  {
    // udpate robot
    stateMsg.timestamp+=10;
    lastResult = robot.UpdateFullRobotState(stateMsg);
    ASSERT_EQ(RESULT_OK, lastResult);
    
    // fake an image
    robot.GetVisionComponent().FakeImageProcessed(stateMsg.timestamp);
    
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  {
    // verify they are gone
    BlockWorldFilter filter;
    objects.clear();
    blockWorld.FindLocatedMatchingObjects(filter, objects);
    ASSERT_EQ(0, objects.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, CopyObjectsFromZombieOrigins)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  ASSERT_EQ(blockWorld.GetNumAliveOrigins(), 0);
  
  // Only connected objects can localize, so add connected
  const ObjectID objID1 = blockWorld.AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  const ObjectID objID2 = blockWorld.AddConnectedActiveObject(1, 1, ObjectType::Block_LIGHTCUBE2);
  
  // make object2 localizable in the current world by adding visual observations
  {
    // Add observation (note observations now require to be shared pointers)
    ActiveObject* object2 = blockWorld.GetConnectedActiveObjectByID(objID2);
    ASSERT_NE(nullptr, object2);

    Pose3d fakePose;
    fakePose.SetParent(robot.GetPose().GetParent());
    
    ObservableObject* observation1 = object2->CloneType();
    observation1->InitPose(fakePose, PoseState::Known);
    observation1->CopyID(object2);
    bool isConfirmed = robot.GetObjectPoseConfirmer().AddVisualObservation(std::shared_ptr<ObservableObject>(observation1), nullptr, false, 10);
    
    // should not be confirmed yet in this origin
    ASSERT_FALSE(isConfirmed);
    ObservableObject* confirmedObject = blockWorld.GetLocatedObjectByID(objID2);
    ASSERT_EQ(nullptr, confirmedObject);

    ObservableObject* observation2 = object2->CloneType();
    observation2->InitPose(fakePose, PoseState::Known);
    observation2->CopyID(object2);
    isConfirmed = robot.GetObjectPoseConfirmer().AddVisualObservation(std::shared_ptr<ObservableObject>(observation2), confirmedObject, false, 10);

    // now it should have been confirmed
    ASSERT_TRUE(isConfirmed);
    confirmedObject = blockWorld.GetLocatedObjectByID(objID2);
    ASSERT_NE(nullptr, confirmedObject);
    
    confirmedObject->SetIsMoving(false, 0);
    confirmedObject->SetLastObservedTime(10);
  }
  
  // Make object2 able to be localized to
  ObservableObject* locatedObj2 = blockWorld.GetLocatedObjectByID(objID2);
  ASSERT_NE(nullptr, locatedObj2);
  ASSERT_EQ(locatedObj2->GetPoseState(), PoseState::Known);
  ASSERT_EQ(locatedObj2->IsActive(), true);
  ASSERT_EQ(locatedObj2->CanBeUsedForLocalization(), true);
  ASSERT_EQ(blockWorld.GetNumAliveOrigins(), 1);
  
  // Delocalizing will create a new frame but the old frame will still exist since
  // object2 is localizable
  robot.Delocalize(false);
  
  // We won't be able to get objects1 and 2 by id since they aren't in the current frame
  ObservableObject* obj1 = blockWorld.GetLocatedObjectByID(objID1);
  ASSERT_EQ(nullptr, obj1);
  
  // Storing this to a new pointer because I use the old object2 pointer later to set poseState
  ObservableObject* obj2 = blockWorld.GetLocatedObjectByID(objID2);
  ASSERT_EQ(nullptr, obj2);
  
  // Add a new object to this currently empty frame
  ObservableObject* object3 = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE3);
  ASSERT_NE(nullptr, object3);
  const ObjectID objID3 = object3->GetID();
  ASSERT_EQ(blockWorld.GetNumAliveOrigins(), 2);

  // Mark object2 in previous frame as Dirty so that frame will become a zombie (not localizable anymore)
  const bool propagateStack = false;
  robot.GetObjectPoseConfirmer().MarkObjectDirty(locatedObj2, propagateStack);
  
  // Delocalizing will create a new frame and delete our 2 zombie frames
  // One of the frames has object1 and 2 the other has object3
  // Since our new frame has no objects, they are all lost. However, the connected instances of 1 and 2 should
  // still be available
  robot.Delocalize(false);
  ASSERT_EQ(blockWorld.GetNumAliveOrigins(), 0); // there are no origins with objects after deloc
  
  obj1 = blockWorld.GetLocatedObjectByID(objID1);
  ASSERT_EQ(nullptr, obj1);
  
  obj2 = blockWorld.GetLocatedObjectByID(objID2);
  ASSERT_EQ(nullptr, obj2);
  
  object3 = blockWorld.GetLocatedObjectByID(objID3);
  ASSERT_EQ(nullptr, object3);
  
  ActiveObject* active1 = blockWorld.GetConnectedActiveObjectByID(objID1);
  ASSERT_NE(nullptr, active1);
  ActiveObject* active2 = blockWorld.GetConnectedActiveObjectByID(objID1);
  ASSERT_NE(nullptr, active2);
} // BlockWorld.CopyObjectsFromZombieOrigins


// This test object allows us to reuse the TEST_P below with different
// Json filenames as a parameter
class BlockWorldTest : public ::testing::TestWithParam<const char*>
{
  
}; // class BlockWorldTest

#define DISPLAY_ERRORS 0

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(Localization, LocalizationDistance)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // For faking observations of two blocks, one closer, one far
  const ObjectType firstType = ObjectType::Block_LIGHTCUBE1;
  const ObjectType secondType = ObjectType::Block_LIGHTCUBE2;
  const Block_Cube1x1 firstCube(firstType);
  const Block_Cube1x1 secondCube(secondType);
  const Vision::Marker::Code firstCode  = firstCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code secondCode = secondCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  const Quad2f farCorners{
    Point2f(159,109),  Point2f(158,129),  Point2f(178,109),  Point2f(178,129)
  };
  
  const Quad2f closeCorners1{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  const Quad2f closeCorners2{
    Point2f( 167,117),  Point2f( 170,185),  Point2f(236,116),  Point2f(237,184)
  };
  
  const ObjectID firstID = robot.GetBlockWorld().AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(firstID.IsSet());
  
  const ObjectID secondID = robot.GetBlockWorld().AddConnectedActiveObject(1, 1, ObjectType::Block_LIGHTCUBE2);
  ASSERT_TRUE(secondID.IsSet());
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  VisionProcessingResult procResult;
  TimeStamp_t fakeTime = 10;
  const s32 kNumObservations = 5;
  f32 observedDistance_mm = -1.f;
  bool success = false;
  
  // After first seeing three times, should be Known and localizable
  lastResult = ObserveMarkerHelper(kNumObservations, {{firstCode, closeCorners1}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have the first object present in block world now
  filter.SetAllowedTypes({firstType});
  const ObservableObject* matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  
  // Make sure we're seeing the first object from close enough to localize to it
  success = ComputeDistanceBetween(robot.GetPose(), matchingObject->GetPose(), observedDistance_mm);
  ASSERT_EQ(true, success);
  ASSERT_LE(observedDistance_mm, matchingObject->GetMaxLocalizationDistance_mm());
  
  const ObjectID obsFirstID = matchingObject->GetID();
  ASSERT_EQ(firstID, obsFirstID);
  
  // Should be localized to "first" object
  ASSERT_EQ(firstID, robot.GetLocalizedTo());
  
  auto FakeMovement = [](RobotState& stateMsg, Robot& robot, TimeStamp_t& fakeTime) -> Result
  {
    // "Move" the robot with a fake state message indicating movement
    stateMsg.status |= (s32)RobotStatusFlag::ARE_WHEELS_MOVING;
    stateMsg.timestamp = fakeTime;
    fakeTime += 10;
    Result lastResult = robot.UpdateFullRobotState(stateMsg);
    if(RESULT_OK == lastResult)
    {
      
      // Stop
      stateMsg.status &= ~(s32)RobotStatusFlag::ARE_WHEELS_MOVING;
      stateMsg.timestamp = fakeTime;
      fakeTime += 10;
      lastResult = robot.UpdateFullRobotState(stateMsg);
      
    }
    return lastResult;
  };
  
  // Need to move and stop the robot so it's willing to localize again
  lastResult = FakeMovement(stateMsg, robot, fakeTime);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // Now observe the second object in close position
  lastResult = ObserveMarkerHelper(kNumObservations, {{secondCode, closeCorners2}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have a the second object present in block world now
  filter.SetAllowedTypes({secondType});
  matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  
  // Make sure we're seeing the second object from close enough to localize to it
  success = ComputeDistanceBetween(robot.GetPose(), matchingObject->GetPose(), observedDistance_mm);
  ASSERT_EQ(true, success);
  ASSERT_LE(observedDistance_mm, matchingObject->GetMaxLocalizationDistance_mm());
  
  const ObjectID obsSecondID = matchingObject->GetID();
  ASSERT_EQ(secondID, obsSecondID);
  
  // Should be localized to "second" object
  ASSERT_EQ(secondID, robot.GetLocalizedTo());
  
  // Need to move and stop the robot so it's willing to localize again
  stateMsg.status |= (s32)RobotStatusFlag::ARE_WHEELS_MOVING;
  stateMsg.timestamp = fakeTime;
  fakeTime += 10;
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Stop in a new pose
  stateMsg.status &= ~(s32)RobotStatusFlag::ARE_WHEELS_MOVING;
  stateMsg.timestamp = fakeTime;
  stateMsg.pose.angle = DEG_TO_RAD(90);
  fakeTime += 10;
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Now observe the first object again, but from far away
  lastResult = ObserveMarkerHelper(kNumObservations, {{firstCode, farCorners}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Make sure the first object's pose is "far"
  matchingObject = robot.GetBlockWorld().GetLocatedObjectByID(firstID);
  ASSERT_NE(nullptr, matchingObject);
  ASSERT_GE(matchingObject->GetLastObservedTime(), stateMsg.timestamp);
  
  // Note we can't check the object's new pose, because it will not update since it was Known before (from being seen
  // from close). We refuse to update it to the new, likely inaccurate pose now.
  //success = ComputeDistanceBetween(robot.GetPose(), matchingObject->GetPose(), observedDistance_mm);
  //ASSERT_EQ(true, success);
  //ASSERT_GT(observedDistance_mm, matchingObject->GetMaxLocalizationDistance_mm());
  
  // Should still be localized to the second object, since we see the first one from too far away
  ASSERT_EQ(secondID, robot.GetLocalizedTo());
  
} // TEST(Localization, LocalizationDistance)


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(FactoryTest, IdealCameraPose)
{
  using namespace Anki;
  using namespace Cozmo;
  
  auto calib = std::make_shared<Vision::CameraCalibration>(240, 320, 290.f, 290.f, 160.f, 120.f);
  const f32 kDotSpacingX_mm = 40.f;
  const f32 kDotSpacingY_mm = 27.f;
  const f32 kTargetDist_mm  = 60.f;
  const Quad3f kTargetDotCenters(Point3f(-0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0),
                                 Point3f(-0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0),
                                 Point3f( 0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0),
                                 Point3f( 0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0));
  
  const std::vector<f32> kPitchAngle_deg = {0, 0.5f, 1, 2, 4, 5};
  const std::vector<f32> kYawAngle_deg   = {0, 0.5f, 1, 2, 4, 5};
  const std::vector<f32> kRollAngle_deg  = {0, 0.1f, 0.5f, 1};
  
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  Vision::Image image;
  ExternalInterface::RobotCompletedFactoryDotTest msg;
  Pose3d pose;
  Quad2f imgQuad;
  
  
  for(f32 pitch_deg : kPitchAngle_deg) {
    for(f32 yaw_deg : kYawAngle_deg) {
      for(f32 roll_deg : kRollAngle_deg) {
        
        Pose3d targetPose(RotationMatrix3d(Radians(DEG_TO_RAD(-pitch_deg)),
                                           Radians(DEG_TO_RAD(-yaw_deg)),
                                           Radians(DEG_TO_RAD(-roll_deg))),
                          Vec3f(0.f, 0.f, kTargetDist_mm));
        Quad3f targetDotCenters;
        targetPose.ApplyTo(kTargetDotCenters, targetDotCenters);
        
        robot.GetVisionComponent().SetCameraCalibration(calib);
        robot.GetVisionComponent().GetCamera().Project3dPoints(targetDotCenters, imgQuad);
      
        Result result = robot.GetVisionComponent().ComputeCameraPoseVsIdeal(imgQuad, pose);
        ASSERT_EQ(result, RESULT_OK);
        
        {
          const f32 kPositionTol_mm = 1.f;
          EXPECT_NEAR(pose.GetTranslation().x(), 0.f, kPositionTol_mm);
          EXPECT_NEAR(pose.GetTranslation().y(), 0.f, kPositionTol_mm);
          EXPECT_NEAR(pose.GetTranslation().z(), 0.f, kPositionTol_mm);
          
          const f32 kAngleTol_deg = 1.f;
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundXaxis().getDegrees(), pitch_deg, kAngleTol_deg);
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundYaxis().getDegrees(), yaw_deg,   kAngleTol_deg);
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundZaxis().getDegrees(), roll_deg,  kAngleTol_deg);
        }
        
        PRINT_NAMED_INFO("FactoryTest.IdealCameraPose.IdealHeadPose",
                         "position=(%.1f,%.1f,%.1f)mm Roll=%.1fdeg (vs %.1f) Pitch=%.1fdeg (vs %.1f), Yaw=%.1fdeg (vs %.1f)",
                         pose.GetTranslation().x(),
                         pose.GetTranslation().y(),
                         pose.GetTranslation().z(),
                         pose.GetRotation().GetAngleAroundZaxis().getDegrees(), roll_deg,
                         pose.GetRotation().GetAngleAroundXaxis().getDegrees(), pitch_deg,
                         pose.GetRotation().GetAngleAroundYaxis().getDegrees(), yaw_deg);
        
        
        // Imperfect calibration:
        calib->SetFocalLength(288, 287);
        calib->SetCenter(Point2f(155.f, 114.f));
        robot.GetVisionComponent().SetCameraCalibration(calib);
        
        result = robot.GetVisionComponent().ComputeCameraPoseVsIdeal(imgQuad, pose);
        ASSERT_EQ(result, RESULT_OK);
        
        {
          const f32 kPositionTol_mm = 2.f;
          EXPECT_NEAR(pose.GetTranslation().x(), 0.f, kPositionTol_mm);
          EXPECT_NEAR(pose.GetTranslation().y(), 0.f, kPositionTol_mm);
          EXPECT_NEAR(pose.GetTranslation().z(), 0.f, kPositionTol_mm);
          
          const f32 kAngleTol_deg = 2.f;
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundXaxis().getDegrees(), pitch_deg, kAngleTol_deg);
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundYaxis().getDegrees(), yaw_deg,   kAngleTol_deg);
          EXPECT_NEAR(pose.GetRotation().GetAngleAroundZaxis().getDegrees(), roll_deg,  kAngleTol_deg);
        }
        
        PRINT_NAMED_INFO("FactoryTest.IdealCameraPose.ImperfectCalib",
                         "position=(%.1f,%.1f,%.1f)mm Roll=%.1fdeg (vs %.1f) Pitch=%.1fdeg (vs %.1f), Yaw=%.1fdeg (vs %.1f)",
                         pose.GetTranslation().x(),
                         pose.GetTranslation().y(),
                         pose.GetTranslation().z(),
                         pose.GetRotation().GetAngleAroundZaxis().getDegrees(), roll_deg,
                         pose.GetRotation().GetAngleAroundXaxis().getDegrees(), pitch_deg,
                         pose.GetRotation().GetAngleAroundYaxis().getDegrees(), yaw_deg);
        
      } // for each pitch
    } // for each yaw
  } // for each roll

  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(FactoryTest, FindDotsInImages)
{
  using namespace Anki;
  using namespace Cozmo;

  struct TestData
  {
    std::string filename;
    std::shared_ptr<Vision::CameraCalibration> calib;
  };
  
  // TODO: Fill in actual calibration data for each test image
  std::vector<TestData> tests = {
    {
      .filename = "test/factoryTests/factoryDotTarget.png",
      .calib = std::make_shared<Vision::CameraCalibration>(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/factoryDotTarget_trulycam2.png",
      .calib = std::make_shared<Vision::CameraCalibration>(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/rocky1.png",
      .calib = std::make_shared<Vision::CameraCalibration>(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/rocky2.png",
      .calib = std::make_shared<Vision::CameraCalibration>(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
  };
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  Vision::Image image;
  ExternalInterface::RobotCompletedFactoryDotTest msg;
  
  for(auto & test : tests)
  {
    const std::string testFilename = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                     test.filename);
    
    robot.GetVisionComponent().SetCameraCalibration(test.calib);
    
    lastResult = image.Load(testFilename);
    ASSERT_EQ(lastResult, RESULT_OK);
    ASSERT_EQ(image.GetNumRows(), test.calib->GetNrows());
    ASSERT_EQ(image.GetNumCols(), test.calib->GetNcols());
    
    lastResult = robot.GetVisionComponent().FindFactoryTestDotCentroids(image, msg);
    ASSERT_EQ(lastResult, RESULT_OK);
    
    ASSERT_TRUE(msg.success);
    
    std::string centroidStr;
    for(s32 iDot=0; iDot<4; ++iDot)
    {
      centroidStr += "(" + std::to_string(msg.dotCenX_pix[iDot]) + "," + std::to_string(msg.dotCenY_pix[iDot]) + ") ";
    }
    PRINT_NAMED_INFO("FactoryTest.FindDotsInImages.Centroids", "'%s':%s",
                     test.filename.c_str(), centroidStr.c_str());
    PRINT_NAMED_INFO("FactoryTest.FindDotsInImages.HeadPose",
                     "'%s'[%s]: position=(%.1f,%.1f,%.1f)mm Roll=%.1fdeg Pitch=%.1fdeg, Yaw=%.1fdeg",
                     test.filename.c_str(), msg.success ? "SUCCESS" : "FAIL",
                     msg.camPoseX_mm, msg.camPoseY_mm, msg.camPoseZ_mm,
                     RAD_TO_DEG(msg.camPoseRoll_rad),
                     RAD_TO_DEG(msg.camPosePitch_rad),
                     RAD_TO_DEG(msg.camPoseYaw_rad));
    
    // TODO: Check the rest of the message contents for sane values
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(ActionableObject, PreActionPoseCaching)
{
  using namespace Anki;
  using namespace Cozmo;

  const Pose3d origin(Rotation3d(0, {0,0,1}), {0,0,0});
  const Pose3d robotPose(Rotation3d(0, {0,0,1}), {0,0,0}, origin);
  const Pose3d cubePose1(Rotation3d(0, {0,0,1}), {10,10,0}, origin);
  const Pose3d cubePose2(Rotation3d(0, {0,0,1}), {20,20,0}, origin);

  ActiveCube cube(ObjectType::Block_LIGHTCUBE1);
  cube.InitPose(cubePose1, PoseState::Known);
  
  std::vector<PreActionPose> preActionPoses;
  // First time getting the preAction poses they should be generated
  ASSERT_EQ(cube.GetCurrentPreActionPoses(preActionPoses, robotPose, {PreActionPose::ActionType::DOCKING}), true);
  
  std::vector<PreActionPose> cachedPreActionPoses;
  // Should be using the cached preAction poses
  ASSERT_EQ(cube.GetCurrentPreActionPoses(cachedPreActionPoses, robotPose, {PreActionPose::ActionType::DOCKING}), false);
  
  std::vector<PreActionPose> flippingPoses;
  // Make sure that only DOCKING preAction poses were cached, all other ActionTypes should still need to be generated
  ASSERT_EQ(cube.GetCurrentPreActionPoses(flippingPoses, robotPose, {PreActionPose::ActionType::FLIPPING}), true);
  
  // Check that the cached poses and generated poses are all the same
  ASSERT_EQ(preActionPoses.size(), cachedPreActionPoses.size());
  for(int i = 0; i < preActionPoses.size(); ++i)
  {
    ASSERT_EQ(preActionPoses[i].GetPose().IsSameAs(cachedPreActionPoses[i].GetPose(), 0, 0), true);
  }
  
  // Change the cube's pose, should clear cached preAction poses
  cube.SetPose(cubePose2, -1, PoseState::Known);
  
  preActionPoses.clear();
  // Should need to regenerate preAction poses since our cached poses were cleared
  ASSERT_EQ(cube.GetCurrentPreActionPoses(preActionPoses, robotPose, {PreActionPose::ActionType::DOCKING}), true);
  
  ASSERT_EQ(preActionPoses.size(), cachedPreActionPoses.size());
  // Old cached poses should not be the same as the newly generated poses
  for(int i = 0; i < preActionPoses.size(); ++i)
  {
    ASSERT_EQ(preActionPoses[i].GetPose().IsSameAs(cachedPreActionPoses[i].GetPose(), 0, 0), false);
  }
}

TEST(BlockWorld, ObjectRobotCollisionCheck)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  
  ObservableObject* object = CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);
  
  ASSERT_NE(nullptr, object);
  const ObjectID& objID = object->GetID();
  ASSERT_TRUE(objID.IsSet());
  
  TimeStamp_t fakeTime = 10;
  
  // Put the robot somewhere "random" (to avoid possible special case of reasoning near origin)
  const Pose3d robotPose(M_PI_4_F, Z_AXIS_3D(), {123.f, 456.f, 0.f}, robot.GetWorldOrigin());
  robot.SetPose(robotPose);
  
  // Helper lambda for doing test with object at different heights
  auto CheckPoseStateAtHeight = [&robot,object,&fakeTime](f32 objHeight) -> PoseState
  {
    // Object is always colocated with robot, but at different heights
    const Point2f robotPosition( robot.GetPose().GetTranslation() );
    Pose3d pose(0.f, Z_AXIS_3D(), {robotPosition.x(), robotPosition.y(), objHeight});
    
    // Set object's new pose with fake observations
    {
      robot.GetVisionComponent().FakeImageProcessed(fakeTime);
      
      object->SetIsMoving(false, 0);
      object->SetLastObservedTime(fakeTime);
      pose.SetParent(robot.GetPose().GetParent());
      
      // Add enough observations to fully update object's pose and make it Known
      std::shared_ptr<ObservableObject> observation( object->CloneType() );
      observation->InitPose(pose, PoseState::Known);
      observation->CopyID(object);
      for(s32 i=0; i<2; ++i)
      {
        robot.GetObjectPoseConfirmer().AddVisualObservation(observation, object, false, 10);
      }
      
      if(!ANKI_VERIFY(object->GetPoseState() == PoseState::Known,
                      "BlockWorld.ObjectRobotCollisionCheck.HelperLambda.PoseNotKnown",
                      "PoseState:%s", EnumToString(object->GetPoseState())))
      {
        return PoseState::Invalid;
      }
    }
    
    fakeTime+=10;
    
    // "Unobserve" the object and update BlockWorld to trigger a collision check
    robot.GetVisionComponent().FakeImageProcessed(fakeTime);
    fakeTime+=10;
    robot.GetBlockWorld().UpdateObservedMarkers({});
    
    return object->GetPoseState();
  };
  
  // Robot right on top of object should result in object being marked Dirty
  ASSERT_EQ(PoseState::Dirty, CheckPoseStateAtHeight(object->GetSize().z() * 0.5f));
  
  // Object directly above robot should leave object Known
  ASSERT_EQ(PoseState::Known, CheckPoseStateAtHeight(ROBOT_BOUNDING_Z + object->GetSize().z()));
  
  // Object center below ground but still sticking up above ground should be Dirty
  ASSERT_EQ(PoseState::Dirty, CheckPoseStateAtHeight(-0.5f*object->GetSize().z() + 5.f));
  
  // Object completely below ground under the robot should remain Known
  ASSERT_EQ(PoseState::Known, CheckPoseStateAtHeight(-1.5*object->GetSize().z()));
  
  // Object slightly off the ground, but not fully above robot should be Dirty
  ASSERT_EQ(PoseState::Dirty, CheckPoseStateAtHeight(0.75f*ROBOT_BOUNDING_Z));

}

TEST(Localization, UnexpectedMovement)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Result lastResult;
  
  Robot robot(1, cozmoContext);
  robot.FakeSyncTimeAck();
  robot.SetPhysicalRobot(true);
  
  BlockWorld& blockWorld = robot.GetBlockWorld();
  
  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );
  
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);
  
  // For faking observations of a block
  const ObjectType firstType = ObjectType::Block_LIGHTCUBE1;
  const Block_Cube1x1 firstCube(firstType);
  const Vision::Marker::Code firstCode  = firstCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  
  const Quad2f closeCorners1{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };
  
  const ObjectID firstID = robot.GetBlockWorld().AddConnectedActiveObject(0, 0, ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(firstID.IsSet());
  
  // Camera calibration
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  VisionProcessingResult procResult;
  TimeStamp_t fakeTime = 10;
  const s32 kNumObservations = 5;
  
  // After first seeing three times, should be Known and localizable
  lastResult = ObserveMarkerHelper(kNumObservations, {{firstCode, closeCorners1}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  // Should have the first object present in block world now
  filter.SetAllowedTypes({firstType});
  const ObservableObject* matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);
  
  const ObjectID obsFirstID = matchingObject->GetID();
  ASSERT_EQ(firstID, obsFirstID);
  
  // Should be localized to "first" object
  ASSERT_EQ(firstID, robot.GetLocalizedTo());
  
  auto FakeUnexpectedMovement = [](RobotState& stateMsg, Robot& robot, TimeStamp_t& fakeTime) -> Result
  {
    // "Move" the robot with a fake state message indicating movement
    stateMsg.status |= (s32)RobotStatusFlag::ARE_WHEELS_MOVING;
    stateMsg.timestamp = fakeTime;
    stateMsg.lwheel_speed_mmps = 50;
    stateMsg.rwheel_speed_mmps = 0;
    fakeTime += 10;
    Result lastResult = robot.UpdateFullRobotState(stateMsg);
    return lastResult;
  };
  
  // Fake unexpected movement until we are one state message away from triggering it
  for(int i = 0; i < robot.GetMoveComponent().GetMaxUnexpectedMovementCount(); ++i)
  {
    FakeUnexpectedMovement(stateMsg, robot, fakeTime);
  }
  
  // Delocalize the robot to create a new origin
  robot.Delocalize(false);
  
  // Fake one more unexpected movement which used to cause unexpected movement to fully trigger
  // setting the robot's pose to a pose in history (when the unexpected movement started). This
  // would be from an old origin, before the delocalize, causing the robot's pose's origin to be
  // different from the robot's world origin
  FakeUnexpectedMovement(stateMsg, robot, fakeTime);
  
  ASSERT_TRUE(robot.IsPoseInWorldOrigin(robot.GetPose()));
}

#define CONFIGROOT "ANKICONFIGROOT"
#define WORKROOT "ANKIWORKROOT"

int main(int argc, char ** argv)
{
  //LEAKING HERE
  Anki::Util::PrintfLoggerProvider* loggerProvider = new Anki::Util::PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
  Anki::Util::gLoggerProvider = loggerProvider;


  std::string configRoot;
  char* configRootChars = getenv(CONFIGROOT);
  if (configRootChars != NULL) {
    configRoot = configRootChars;
  }

  std::string workRoot;
  char* workRootChars = getenv(WORKROOT);
  if (workRootChars != NULL)
    workRoot = workRootChars;

  std::string resourcePath;
  std::string persistentPath;
  std::string cachePath;

  if (configRoot.empty() || workRoot.empty()) {
    char cwdPath[1256];
    getcwd(cwdPath, 1255);
    PRINT_NAMED_INFO("CozmoTests.main","cwdPath %s", cwdPath);
    PRINT_NAMED_INFO("CozmoTests.main","executable name %s", argv[0]);
/*  // still troubleshooting different run time environments,
    // need to find a way to detect where the resources folder is located on disk.
    // currently this is relative to the executable.
    // Another option is to pass it in through the environment variables.
    // Get the last position of '/'
    std::string aux(argv[0]);
#if defined(_WIN32) || defined(WIN32)
    size_t pos = aux.rfind('\\');
#else
    size_t pos = aux.rfind('/');
#endif
    std::string path = aux.substr(0,pos);
*/
    std::string path = cwdPath;
    resourcePath = path + "/../../data/assets/cozmo_resources";
    persistentPath = path + "/persistent";
    cachePath = path + "/cache";
  } else {
    // build server specifies configRoot and workRoot
    resourcePath = configRoot + "/resources";
    persistentPath = workRoot + "/persistent";
    cachePath = workRoot + "/cache";
  }
  
  // Suppress break-on-error for duration of these tests
  Anki::Util::_errBreakOnError = false;

  // Initialize CameraService singleton without supervisor
  CameraService::SetSupervisor(nullptr);
  
  // Initialize OSState singleton without supervisor
  OSState::SetSupervisor(nullptr);

  // Initialize CubeBleClient singleton without supervisor
  CubeBleClient::SetSupervisor(nullptr);

  //LEAKING HERE
  Anki::Util::Data::DataPlatform* dataPlatform = new Anki::Util::Data::DataPlatform(persistentPath, cachePath, resourcePath);
  UiMessageHandler handler(0, nullptr);
  cozmoContext = new Anki::Cozmo::CozmoContext(dataPlatform, &handler);
  
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  cozmoContext->GetDataLoader()->LoadNonConfigData();
  
  //// should we do this here? clean previously dirty folders?
  //std::string cache = dataPlatform->pathToResource(Anki::Cozmo::Data::Scope::Cache, "");
  //Anki::Util::FileUtils::RemoveDirectory(cache);
  //std::string files = dataPlatform->pathToResource(Anki::Cozmo::Data::Scope::Output, "");
  //Anki::Util::FileUtils::RemoveDirectory(files);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
