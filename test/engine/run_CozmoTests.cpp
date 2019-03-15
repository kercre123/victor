#define private public
#define protected public

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/shared/types.h"

#include "camera/cameraService.h"
#include "osState/osState.h"
#include "cubeBleClient/cubeBleClient.h"

#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/charger.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/robotToEngineImplMessaging.h"
#include "engine/vision/visionSystem.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/factory/emrHelper.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "gtest/gtest.h"
#include "json/json.h"
#include "test/engine/helpers/cubePlacementHelper.h"
#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include <array>
#include <fstream>
#include <unistd.h>

#if !defined(ANKICONFIGROOT)
#error "You must define a default ANKICONFIGROOT when compiling"
#endif

#if !defined(ANKIWORKROOT)
#error "You must define a default ANKIWORKROOT when compiling"
#endif

using namespace Anki::Vector;

Anki::Vector::CozmoContext* cozmoContext = nullptr; // This is externed and used by tests

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test listener that fails the test if it ends with the global error flag set to true
class ListenerFailOnErrFlag : public ::testing::EmptyTestEventListener {
private:

  virtual void OnTestStart(const ::testing::TestInfo& test_info) {
    _errG = Anki::Util::sGetErrG();
  }
  virtual void OnTestEnd(const ::testing::TestInfo& test_info)
  {
    if( Anki::Util::sGetErrG() ) {
      // unset it if this test started without it set. The only way errG will start off set is if another
      // thread sets it, in which case we preserve errG so that the return value of main() will be nonzero
      if( !_errG ) {
        Anki::Util::sUnSetErrG();
      }

      GTEST_FAIL() << "Test "
        << test_info.test_case_name()
        << "." << test_info.name()
        << " or a concurrent thread called PRINT_NAMED_ERROR or ANKI_VERIFY";
    }
  }

  bool _errG = false;
};

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
TEST(Cozmo, WhiskeyTest)
{
  using namespace Anki::Vector;

  UNIT_TEST_WHISKEY;

  ASSERT_TRUE(IsWhiskey());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(Cozmo, NotWhiskeyTest)
{
  using namespace Anki::Vector;
  ASSERT_FALSE(IsWhiskey());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(RobotState, ImuDataSize)
{
  using namespace Anki::Vector;
  RobotState state;
  ASSERT_EQ(state.imuData.size(), IMUConstants::IMU_FRAMES_PER_ROBOT_STATE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(BlockWorld, AddAndRemoveObject)
{
  using namespace Anki;
  using namespace Vector;

  Result lastResult;

  Robot robot(1, cozmoContext);
  robot.FakeSyncRobotAck();

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
    std::vector<Block*> activeObjects;
    blockWorld.FindConnectedMatchingBlocks(filter, activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }

  // Fake a state message update for robot
  auto fakeRobotState = [](const uint32_t atTimestamp) {
    RobotState stateMsg = Robot::GetDefaultRobotState();
    stateMsg.timestamp = atTimestamp;
    for (auto& imuData : stateMsg.imuData) {
      imuData.timestamp = atTimestamp;
    }
    return stateMsg;
  };
  
  uint32_t stateMsgTimestamp = 1;
  lastResult = robot.UpdateFullRobotState(fakeRobotState(stateMsgTimestamp));
  ASSERT_EQ(lastResult, RESULT_OK);

  // Fake an observation of a block:
  const ObjectType testType = ObjectType::Block_LIGHTCUBE1;
  Block testCube(testType);
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
  Vision::ObservedMarker marker(stateMsgTimestamp, testCode, corners, robot.GetVisionComponent().GetCamera());

  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileRotatingFast(true);

  // Queue the marker in VisionComponent, which will in turn queue it for processing
  // by BlockWorld

  // Tick BlockWorld, which will use the queued marker
  std::list<Vision::ObservedMarker> markers{marker};
  lastResult = robot.GetBlockWorld().UpdateObservedMarkers(markers);
  ASSERT_EQ(lastResult, RESULT_OK);

  {
    // should be no connected objects
    BlockWorldFilter filter;
    std::vector<Block*> activeObjects;
    blockWorld.FindConnectedMatchingBlocks(filter, activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }

  ObjectID firstObjID;

  // There should be an object of the right type, with the right ID in BlockWorld
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
  const s32 kNumObservations = 3;
  for(s32 i=0; i<kNumObservations; ++i)
  {
    stateMsgTimestamp += 10;
    lastResult = robot.UpdateFullRobotState(fakeRobotState(stateMsgTimestamp));
    ASSERT_EQ(RESULT_OK, lastResult);
    robot.GetVisionComponent().FakeImageProcessed(stateMsgTimestamp);
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
    blockWorld.MarkObjectDirty(object);
  }

  // Now after not seeing the object three times, it should be Unknown
  // because it was dirty
  for(s32 i=0; i<kNumObservations; ++i)
  {
    stateMsgTimestamp += 10;
    lastResult = robot.UpdateFullRobotState(fakeRobotState(stateMsgTimestamp));
    ASSERT_EQ(RESULT_OK, lastResult);
    robot.GetVisionComponent().FakeImageProcessed(stateMsgTimestamp);
    lastResult = robot.GetBlockWorld().UpdateObservedMarkers(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  blockWorld.CheckForUnobservedObjects(stateMsgTimestamp);

  {
    // should be unknown now. In the new code Unknown objects are removed from their origin
    ObservableObject* object = blockWorld.GetLocatedObjectByID(firstObjID);
    ASSERT_EQ(object, nullptr);
  }

  // See the object again, but from "far" away
  corners.Scale(0.15f);

  for(s32 i=0; i<kNumObservations; ++i)
  {
    // Tick the robot, which will tick the BlockWorld, which will use the queued marker
    stateMsgTimestamp += 10;
    lastResult = robot.UpdateFullRobotState(fakeRobotState(stateMsgTimestamp));
    ASSERT_EQ(RESULT_OK, lastResult);
    Vision::ObservedMarker markerFar(stateMsgTimestamp, testCode, corners, robot.GetVisionComponent().GetCamera());
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

    // Should have used the same ID for the cube
    ASSERT_EQ(secondObjID, firstObjID);
    
    ASSERT_NE(nullptr, object);
    ASSERT_EQ(PoseState::Known, object->GetPoseState());
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
                                        Anki::RobotTimeStamp_t& fakeTime,
                                        Anki::Vector::Robot& robot,
                                        Anki::Vector::RobotState& stateMsg,
                                        Anki::Vector::VisionProcessingResult& procResult)
{
  using namespace Anki;

  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    stateMsg.timestamp = (TimeStamp_t)fakeTime;
    stateMsg.pose_frame_id = robot.GetPoseFrameID();
    stateMsg.pose_origin_id = robot.GetPoseOriginList().GetCurrentOriginID();
    for (auto& imuData : stateMsg.imuData) {
      imuData.timestamp = (uint32_t) fakeTime;
    }
    Result lastResult = robot.UpdateFullRobotState(stateMsg);
    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("ObservedMarkerHelper.UpdateFullRobotStateFailed", "i=%d fakeTime=%u", i, (TimeStamp_t)fakeTime);
      return lastResult;
    }

    procResult.timestamp = (TimeStamp_t)fakeTime;
    procResult.observedMarkers.clear();
    for(auto & codeAndCorners : codesAndCorners)
    {
      procResult.observedMarkers.push_back(Vision::ObservedMarker((TimeStamp_t)fakeTime, codeAndCorners.first, codeAndCorners.second,
                                                                  robot.GetVisionComponent().GetCamera()));
    }

    lastResult = robot.GetVisionComponent().UpdateVisionMarkers(procResult);

    if(RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("ObservedMarkerHelper.UpdateVisionMarkersFailed", "i=%d fakeTime=%u", i, (TimeStamp_t)fakeTime);
      return lastResult;
    }
  }

  return RESULT_OK;
}

// Helper method for BlockWorld.UpdateObjectOrigins Test
static Anki::Result FakeRobotMovement(Anki::Vector::Robot& robot,
                                      Anki::Vector::RobotState& stateMsg,
                                      Anki::RobotTimeStamp_t& fakeTime)
{
  using namespace Anki;
  using namespace Vector;

  stateMsg.timestamp = (TimeStamp_t)fakeTime;
  stateMsg.status |= (u16)RobotStatusFlag::ARE_WHEELS_MOVING; // Set moving flag
  Result lastResult = robot.UpdateFullRobotState(stateMsg);
  stateMsg.status &= ~(u16)RobotStatusFlag::ARE_WHEELS_MOVING; // Unset moving flag
  fakeTime += 10;

  return lastResult;
}

TEST(BlockWorld, UpdateObjectOrigins)
{
  using namespace Anki;
  using namespace Vector;

  Result lastResult;

  Robot robot(1, cozmoContext);
  robot.FakeSyncRobotAck();

  BlockWorld& blockWorld = robot.GetBlockWorld();

  // There should be nothing in BlockWorld yet
  BlockWorldFilter filter;
  std::vector<ObservableObject*> objects;
  blockWorld.FindLocatedMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());

  {
    // no connected objects either
    std::vector<Block*> activeObjects;
    blockWorld.FindConnectedMatchingBlocks(BlockWorldFilter(), activeObjects);
    ASSERT_TRUE(activeObjects.empty());
  }

  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState() );

  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(lastResult, RESULT_OK);

  // For faking observations of two blocks, one closer, one far
  const ObjectType farType = ObjectType::Block_LIGHTCUBE1;
  const ObjectType closeType = ObjectType::Block_LIGHTCUBE2;
  const Block farCube(farType);
  const Block closeCube(closeType);
  const Vision::Marker::Code farCode   = farCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();
  const Vision::Marker::Code closeCode = closeCube.GetMarker(Block::FaceName::FRONT_FACE).GetCode();

  const Quad2f farCorners{
    Point2f(213,111),  Point2f(214,158),  Point2f(260,111),  Point2f(258,158)
  };

  const Quad2f closeCorners{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };

  const ObjectID farID = robot.GetBlockWorld().AddConnectedBlock(0, "AA:AA:AA:AA:AA:AA", ObjectType::Block_LIGHTCUBE1);
  ASSERT_TRUE(farID.IsSet());

  const ObjectID closeID = robot.GetBlockWorld().AddConnectedBlock(1, "BB:BB:BB:BB:BB:BB", ObjectType::Block_LIGHTCUBE2);
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
  robot.GetVisionComponent().EnableVisionWhileRotatingFast(true);

  VisionProcessingResult procResult;

  // See far block and localize to it.
  RobotTimeStamp_t fakeTime = 10;

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
  auto* connectedFar = robot.GetBlockWorld().GetConnectedBlockByID(farID);
  ASSERT_NE(nullptr, connectedFar);
  ASSERT_EQ(connectedFar->GetActiveID(), matchingObject->GetActiveID());

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
  auto* connectedClose = robot.GetBlockWorld().GetConnectedBlockByID(closeID);
  ASSERT_NE(nullptr, connectedClose);
  ASSERT_EQ(connectedClose->GetActiveID(), matchingObject->GetActiveID());

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
} // BlockWorld.UpdateObjectOrigins

// This test object allows us to reuse the TEST_P below with different
// Json filenames as a parameter
class BlockWorldTest : public ::testing::TestWithParam<const char*>
{

}; // class BlockWorldTest

#define DISPLAY_ERRORS 0

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(FactoryTest, IdealCameraPose)
{
  using namespace Anki;
  using namespace Vector;

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
  robot.FakeSyncRobotAck();

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
  using namespace Vector;

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
  robot.FakeSyncRobotAck();

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
  using namespace Vector;

  const Pose3d origin(Rotation3d(0, {0,0,1}), {0,0,0});
  const Pose3d robotPose(Rotation3d(0, {0,0,1}), {0,0,0}, origin);
  const Pose3d cubePose1(Rotation3d(0, {0,0,1}), {10,10,0}, origin);
  const Pose3d cubePose2(Rotation3d(0, {0,0,1}), {20,20,0}, origin);

  Block cube(ObjectType::Block_LIGHTCUBE1);
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
  using namespace Vector;

  Robot robot(1, cozmoContext);
  robot.FakeSyncRobotAck();

  ObservableObject* object = CubePlacementHelper::CreateObjectLocatedAtOrigin(robot, ObjectType::Block_LIGHTCUBE1);

  ASSERT_NE(nullptr, object);
  const ObjectID& objID = object->GetID();
  ASSERT_TRUE(objID.IsSet());

  RobotTimeStamp_t fakeTime = 10;

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
      object->SetLastObservedTime((TimeStamp_t)fakeTime);
      pose.SetParent(robot.GetPose().GetParent());
      
      auto result = robot.GetBlockWorld().SetObjectPose(object->GetID(), pose, PoseState::Known);
      if(!ANKI_VERIFY(result == RESULT_OK,
                      "BlockWorld.ObjectRobotCollisionCheck.HelperLambda.FailedUpdateBlockworld", ""))
      {
        return PoseState::Invalid;
      }
    }

    fakeTime+=10;

    // "Unobserve" the object and update BlockWorld to trigger a collision check
    robot.GetVisionComponent().FakeImageProcessed(fakeTime);
    fakeTime+=10;
    robot.GetBlockWorld().UpdateObservedMarkers({});
    robot.GetBlockWorld().CheckForRobotObjectCollisions();

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
  using namespace Vector;

  Result lastResult;

  Robot robot(1, cozmoContext);
  robot.FakeSyncRobotAck();

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

  // For faking observations of a charger
  const Charger charger;
  const Vision::Marker::Code code = charger.GetMarker()->GetCode();

  const Quad2f closeCorners1{
    Point2f( 67,117),  Point2f( 70,185),  Point2f(136,116),  Point2f(137,184)
  };

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
  robot.GetVisionComponent().EnableVisionWhileRotatingFast(true);

  VisionProcessingResult procResult;
  RobotTimeStamp_t fakeTime = 10;
  const s32 kNumObservations = 5;

  // After first seeing three times, should be Known and localizable
  lastResult = ObserveMarkerHelper(kNumObservations, {{code, closeCorners1}},
                                   fakeTime, robot, stateMsg, procResult);
  ASSERT_EQ(RESULT_OK, lastResult);

  // Should have the charger present in block world now
  filter.SetAllowedTypes({ObjectType::Charger_Basic});
  const ObservableObject* matchingObject = robot.GetBlockWorld().FindLocatedMatchingObject(filter);
  ASSERT_NE(nullptr, matchingObject);

  const ObjectID obsID = matchingObject->GetID();
  ASSERT_TRUE(obsID.IsSet());

  // Should be localized to charger
  ASSERT_EQ(obsID, robot.GetLocalizedTo());

  auto FakeUnexpectedMovement = [](RobotState& stateMsg, Robot& robot, RobotTimeStamp_t& fakeTime) -> Result
  {
    // "Move" the robot with a fake state message indicating movement
    stateMsg.status |= (s32)RobotStatusFlag::ARE_WHEELS_MOVING;
    stateMsg.timestamp = (TimeStamp_t)fakeTime;
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

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  // Since some tests are testing for changes to the global error flag in a MT environment, block access to
  // the flag in some circumstances
  Anki::Util::_lockErrG = true;

  //LEAKING HERE
  Anki::Util::PrintfLoggerProvider* loggerProvider = new Anki::Util::PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);
  Anki::Util::gLoggerProvider = loggerProvider;

  // Compile-time defaults
  std::string configRoot = ANKICONFIGROOT;
  std::string workRoot = ANKIWORKROOT;

  // Allow environment to override defaults
  const char * configRootChars = getenv("ANKICONFIGROOT");
  if (configRootChars != nullptr) {
    configRoot = configRootChars;
  }

  const char * workRootChars = getenv("ANKIWORKROOT");
  if (workRootChars != nullptr) {
    workRoot = workRootChars;
  }

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

  Anki::Util::_errBreakOnError = true;

  // Initialize CameraService singleton without supervisor
  CameraService::SetSupervisor(nullptr);

  // Initialize CubeBleClient singleton without supervisor
  CubeBleClient::SetSupervisor(nullptr);

  //LEAKING HERE
  Anki::Util::Data::DataPlatform* dataPlatform = new Anki::Util::Data::DataPlatform(persistentPath, cachePath, resourcePath);
  UiMessageHandler handler(0);
  ProtoMessageHandler protoHandler;
  cozmoContext = new Anki::Vector::CozmoContext(dataPlatform, &handler, &protoHandler);

  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  cozmoContext->GetDataLoader()->LoadNonConfigData();

  //// should we do this here? clean previously dirty folders?
  //std::string cache = dataPlatform->pathToResource(Anki::Vector::Data::Scope::Cache, "");
  //Anki::Util::FileUtils::RemoveDirectory(cache);
  //std::string files = dataPlatform->pathToResource(Anki::Vector::Data::Scope::Output, "");
  //Anki::Util::FileUtils::RemoveDirectory(files);



  // add test listener that fails a test whenever the global error flag is set
  {
    ::testing::UnitTest& unit_test = *::testing::UnitTest::GetInstance();
    ::testing::TestEventListeners& listeners = unit_test.listeners();
    listeners.Append(new ListenerFailOnErrFlag); // get cleaned up by gtest
  }

  // all tests must be successful
  const bool testFailed = (RUN_ALL_TESTS() != 0);
  // Tests must not set the global error flag (via calls to PRINT_NAMED_ERROR or ANKI_VERIFY).
  // ListenerFailOnErrFlag checks for this, but unsets the global error flag so that not all tests
  // look like they're failing. This final case for testHadError will catch any threads that set
  // the global error flag
  const bool testHadError = Anki::Util::sGetErrG();
  
  return (testFailed || testHadError);
}
