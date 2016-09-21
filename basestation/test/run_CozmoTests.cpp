#include <array>
#include <fstream>
#include "gtest/gtest.h"
#include "json/json.h"
#include "anki/common/types.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/fileUtils/fileUtils.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include <unistd.h>

Anki::Cozmo::CozmoContext* cozmoContext = nullptr; // This is externed and used by tests

TEST(DataPlatform, ReadWrite)
{
  ASSERT_TRUE(cozmoContext->GetDataPlatform() != nullptr);
  Json::Value config;
  const bool readSuccess = cozmoContext->GetDataPlatform()->readAsJson(
    Anki::Util::Data::Scope::Resources,
    "config/basestation/config/configuration.json",
    config);
  EXPECT_TRUE(readSuccess);

  config["blah"] = 7;
  const bool writeSuccess = cozmoContext->GetDataPlatform()->writeAsJson(Anki::Util::Data::Scope::Cache, "someRandomFolder/A/writeTest.json", config);
  EXPECT_TRUE(writeSuccess);

  std::string someRandomFolder = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Cache, "someRandomFolder");
  Anki::Util::FileUtils::RemoveDirectory(someRandomFolder);
}


TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}

TEST(BlockWorld, AddAndRemoveObject)
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
  blockWorld.FindMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());

  // Fake a state message update for robot
  RobotState stateMsg;
  stateMsg.timestamp = 1; // the robot should not report at 0, since we are after timeSync and 0 is used by the robot constructor to set everything to 0 
  stateMsg.pose_frame_id = 0;
  stateMsg.pose_origin_id = 1;
  stateMsg.pose.x = 0.0f;
  stateMsg.pose.y = 0.0f;
  stateMsg.pose.z = 0.0f;
  stateMsg.pose.angle = 0.0f;
  stateMsg.pose.pitch_angle = 0.0f;
  stateMsg.lwheel_speed_mmps = 0.0f;
  stateMsg.rwheel_speed_mmps = 0.0f;
  stateMsg.headAngle = 0.0f;
  stateMsg.liftAngle = 0.0f;
  stateMsg.liftHeight = 0.0f;
  stateMsg.rawGyroZ = 0.0f;
  stateMsg.rawAccelY = 0.0f;
  stateMsg.status = (u16)RobotStatusFlag::HEAD_IN_POS | (u16)RobotStatusFlag::LIFT_IN_POS;
  stateMsg.lastPathID = 0;
  stateMsg.currPathSegment = 0;
  stateMsg.numFreeSegmentSlots = 0;
  stateMsg.batteryVoltage = 0.0f;
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
  
  Vision::CameraCalibration camCalib(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                     HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                     HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  const f32 halfHeight = 0.25f*static_cast<f32>(camCalib.GetNrows());
  const f32 halfWidth = 0.25f*static_cast<f32>(camCalib.GetNcols());
  const f32 xcen = camCalib.GetCenter_x();
  const f32 ycen = camCalib.GetCenter_y();
  
  Quad2f corners;
  const f32 markerHalfSize = std::min(halfHeight, halfWidth);
  corners[Quad::TopLeft]    = {xcen - markerHalfSize, ycen - markerHalfSize};
  corners[Quad::BottomLeft] = {xcen - markerHalfSize, ycen + markerHalfSize};
  corners[Quad::TopRight]   = {xcen + markerHalfSize, ycen - markerHalfSize};
  corners[Quad::BottomRight]= {xcen + markerHalfSize, ycen + markerHalfSize};
  TimeStamp_t fakeTime = 1;
  Vision::ObservedMarker marker(fakeTime, testCode, corners, robot.GetVisionComponent().GetCamera());
  
  // Enable "vision while moving" so that we don't have to deal with trying to compute
  // angular velocities, since we don't have real state history to do so.
  robot.GetVisionComponent().EnableVisionWhileMovingFast(true);
  
  // Queue the marker in VisionComponent, which will in turn queue it for processing
  // by BlockWorld
  
  // Tick BlockWorld, which will use the queued marker
  std::list<Vision::ObservedMarker> markers{marker};
  lastResult = robot.GetBlockWorld().Update(markers);
  ASSERT_EQ(lastResult, RESULT_OK);
  ASSERT_EQ(0, markers.size()); // all markers will get used up
  
  // There should now be an object of the right type, with the right ID in BlockWorld
  objects.clear();
  filter.SetFilterFcn(nullptr);
  filter.SetAllowedTypes({testType});
  blockWorld.FindMatchingObjects(filter, objects);
  ASSERT_EQ(1, objects.size());
  auto objByIdIter = objects.begin();
  ASSERT_NE(objByIdIter, objects.end());
  ASSERT_NE(*objByIdIter, nullptr);
  
  ObjectID objID = (*objByIdIter)->GetID();
  ObservableObject* object = blockWorld.GetObjectByID(objID);
  ASSERT_NE(object, nullptr);
  ASSERT_EQ(object->GetID(), objID);
  ASSERT_EQ(object->GetType(), testType);
  
  // Returned object should be dynamically-castable to its base class:
  Block* block = dynamic_cast<Block*>(object);
  ASSERT_NE(block, nullptr);
  
  // Object should start in Unknown pose state, until confirmed
  ASSERT_EQ(PoseState::Unknown, object->GetPoseState());
  
  // After seeing three times, should be Known
  const s32 kNumObservations = 3;
  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    // Tick the robot, which will tick the BlockWorld, which will use the queued marker
    Vision::ObservedMarker marker(fakeTime, testCode, corners, robot.GetVisionComponent().GetCamera());
    std::list<Vision::ObservedMarker> markers{marker};
    lastResult = robot.GetBlockWorld().Update(markers);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
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
  const bool cornersMatch = IsNearlyEqual(obsCorners, corners, .25f);
  ASSERT_TRUE(cornersMatch);

  // After NOT seeing the object three times, it should still be known
  // because we don't yet have evidence that actually isn't there
  std::list<Vision::ObservedMarker> emptyMarkersList;
  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    robot.GetVisionComponent().FakeImageProcessed(fakeTime);
    lastResult = robot.GetBlockWorld().Update(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  ASSERT_EQ(PoseState::Known, object->GetPoseState());
  
  // Now fake an object moved message
  object->SetIsMoving(true, 0);
  object->SetPoseState(PoseState::Dirty);
  
  // Now after not seeing the object three times, it should be Unknown
  // because it was dirty
  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    robot.GetVisionComponent().FakeImageProcessed(fakeTime);
    lastResult = robot.GetBlockWorld().Update(emptyMarkersList);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  ASSERT_EQ(PoseState::Unknown, object->GetPoseState());
  
  // See the object again, but from "far" away, which after a few ticks should make it
  // Dirty again.
  corners.Scale(0.15f);
  
  for(s32 i=0; i<kNumObservations; ++i, fakeTime+=10)
  {
    // Tick the robot, which will tick the BlockWorld, which will use the queued marker
    Vision::ObservedMarker markerFar(fakeTime, testCode, corners, robot.GetVisionComponent().GetCamera());
    std::list<Vision::ObservedMarker> markersFar{markerFar};
    lastResult = robot.GetBlockWorld().Update(markersFar);
    ASSERT_EQ(lastResult, RESULT_OK);
  }
  
  // Find the object again (because it may have been deleted while unknown since
  // it had no active ID set)
  {
    BlockWorldFilter filter;
    filter.SetAllowedTypes({testType});
    std::vector<ObservableObject*> objects;
    blockWorld.FindMatchingObjects(filter, objects);
    ASSERT_EQ(objects.size(), 1);
    auto objByIdIter = objects.begin();
    ASSERT_NE(objByIdIter, objects.end());
    ASSERT_NE(*objByIdIter, nullptr);
    object = *objByIdIter;
    objID = object->GetID();
  }
  
  ASSERT_NE(nullptr, object);
  ASSERT_EQ(PoseState::Dirty, object->GetPoseState());
  
  // Now try deleting the object, and make sure we can't still get it using the old ID
  blockWorld.ClearObject(objID);
  object = blockWorld.GetObjectByID(objID);
  ASSERT_NE(object, nullptr);
  ASSERT_EQ(object->GetPoseState(), PoseState::Unknown);
  
} // BlockWorld.AddAndRemoveObject


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
  blockWorld.FindMatchingObjects(filter, objects);
  ASSERT_TRUE(objects.empty());
  
  // Add three objects
  ObjectID objID1 = blockWorld.AddActiveObject(1, 1, ActiveObjectType::OBJECT_CUBE1);
  ObjectID objID2 = blockWorld.AddActiveObject(2, 2, ActiveObjectType::OBJECT_CUBE2);
  ObjectID objID3 = blockWorld.AddActiveObject(3, 3, ActiveObjectType::OBJECT_CUBE3);
  
  ObservableObject* object1 = blockWorld.GetObjectByID(objID1);
  ASSERT_NE(nullptr, object1);
  
  ObservableObject* object2 = blockWorld.GetObjectByID(objID2);
  ASSERT_NE(nullptr, object2);
  
  ObservableObject* object3 = blockWorld.GetObjectByID(objID3);
  ASSERT_NE(nullptr, object3);
  
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
    {DEG_TO_RAD(7.5), Y_AXIS_3D()},  // Slightly rotated around Y
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
  
  for(auto & btmRot1 : TestInPlaneRotations)
  {
    for(auto & btmRot2 : TestOutOfPlaneRotations)
    {
      for(auto & btmTrans : TestBottomTranslations)
      {
        const Pose3d bottomPose(Rotation3d(btmRot1) * Rotation3d(btmRot2), btmTrans, &robot.GetPose() );
        
        lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, bottomPose, PoseState::Known);
        ASSERT_EQ(RESULT_OK, lastResult);
        
        for(auto & topRot1 : TestInPlaneRotations)
        {
          for(auto & topRot2 : TestOutOfPlaneRotations)
          {
            for(auto & topTrans : TestTopTranslations)
            {
              const Pose3d topPose(Rotation3d(topRot1) * Rotation3d(topRot2), topTrans, &robot.GetPose() );
              
              // For help debugging when there are failures:
              //bottomPose.Print("Unnamed", "Bottom");
              //topPose.Print("Unnamed", "Top");

              lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, topPose, object1);
              ASSERT_EQ(RESULT_OK, lastResult);
              
              ObservableObject* foundObject = blockWorld.FindObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
              ASSERT_NE(nullptr, foundObject);
              ASSERT_EQ(object2, foundObject);
              
              foundObject = blockWorld.FindObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
              ASSERT_NE(nullptr, foundObject);
              ASSERT_EQ(object1, foundObject);
            }
            
            for(auto & nextToTrans : TestNextToTranslations)
            {
              const Pose3d nextToPose(Rotation3d(topRot1) * Rotation3d(topRot2), nextToTrans, &robot.GetPose());
              
              lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, nextToPose, object1);
              ASSERT_EQ(RESULT_OK, lastResult);
              
              ObservableObject* foundObject = blockWorld.FindObjectOnTopOf(*object3, STACKED_HEIGHT_TOL_MM);
              ASSERT_EQ(nullptr, foundObject);
              
              foundObject = blockWorld.FindObjectUnderneath(*object3, STACKED_HEIGHT_TOL_MM);
              ASSERT_EQ(nullptr, foundObject);
            }
          }
        }
      }
    }
  }
  
  
  // Put Object 2 above Object 1, but too high, so this Find should fail:
  const Pose3d bottomPose(0, Z_AXIS_3D(), {100.f, 0.f, 22.f}, &robot.GetPose() );
  lastResult = robot.GetObjectPoseConfirmer().AddRobotRelativeObservation(object1, bottomPose, PoseState::Known);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  const Pose3d tooHighPose(0, Z_AXIS_3D(), {100.f, 0.f, 66.f + 1.5f*STACKED_HEIGHT_TOL_MM}, &robot.GetPose());
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, tooHighPose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  ObservableObject* foundObject = blockWorld.FindObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);

  // Two objects in roughly the same place should also fail
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, bottomPose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  foundObject = blockWorld.FindObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  
  // Put Object 2 at the right height to be on top of Object 1, but with its
  // center of mass _not_ above Object 1. So this should not be considered On Top Of.
  const Pose3d notAbovePose(0, Z_AXIS_3D(), {130.f, -30.f, 66.f}, &robot.GetPose());
  lastResult = robot.GetObjectPoseConfirmer().AddObjectRelativeObservation(object2, notAbovePose, object1);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  foundObject = blockWorld.FindObjectOnTopOf(*object1, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
  foundObject = blockWorld.FindObjectUnderneath(*object2, STACKED_HEIGHT_TOL_MM);
  ASSERT_EQ(nullptr, foundObject);
  
} // BlockWorld.CubeStacks


// This test object allows us to reuse the TEST_P below with different
// Json filenames as a parameter
class BlockWorldTest : public ::testing::TestWithParam<const char*>
{
  
}; // class BlockWorldTest

#define DISPLAY_ERRORS 0

// This is the parameterized test, instantiated with a list of Json files below
TEST_P(BlockWorldTest, BlockAndRobotLocalization)
{
  using namespace Anki;
  using namespace Cozmo;
  
  /*
  // TODO: Tighten/loosen thresholds?
  const float   objectPoseDistThresholdFraction = 0.05f; // within 5% of actual distance
  
  const float   objectPoseAngleThresholdNearDistance = 50.f;
  const float   objectPoseAngleThresholdFarDistance  = 300.f;
  const float   objectPoseAngleNearThreshold = DEG_TO_RAD(5); // at near distance
  const float   objectPoseAngleFarThreshold  = DEG_TO_RAD(25); // at far distance
  */
  
//  const float objectPoseDistThreshold_mm    = 12.f;
//  const Radians objectPoseAngleThreshold    = DEG_TO_RAD(15.f);
  const float   robotPoseDistThreshold_mm  = 10.f;
  const Radians robotPoseAngleThreshold    = DEG_TO_RAD(3.f);
  
  Json::Reader reader;
  Json::Value jsonRoot;
  std::vector<std::string> jsonFileList;
  
  const std::string subPath("test/blockWorldTests/");
  const std::string jsonFilename = subPath + GetParam();

  fprintf(stdout, "\n\nLoading JSON file '%s'\n", jsonFilename.c_str());

  const bool jsonParseResult = cozmoContext->GetDataPlatform()->readAsJson(Anki::Util::Data::Scope::Resources, jsonFilename, jsonRoot);
  ASSERT_TRUE(jsonParseResult);

  // Create the modules we need (and stubs of those we don't)
  RobotManager        robotMgr(nullptr);
 
  robotMgr.AddRobot(0);
  Robot& robot = *robotMgr.GetRobotByID(0);
  
//  Robot robot(0, 0, &blockWorld, 0);    // TODO: Support multiple robots

  ASSERT_TRUE(jsonRoot.isMember("CameraCalibration"));
  Vision::CameraCalibration calib(jsonRoot["CameraCalibration"]);
  robot.GetVisionComponent().SetCameraCalibration(calib);
    
  bool checkRobotPose;
  ASSERT_TRUE(JsonTools::GetValueOptional(jsonRoot, "CheckRobotPose", checkRobotPose));

  // Everything before this is per-world, not per-pose
  
  ASSERT_TRUE(jsonRoot.isMember("Poses"));
  
  const int NumPoses = jsonRoot["Poses"].size();
  
#if DISPLAY_ERRORS
  struct ErrorStruct {
    Vec3f T_robot, T_blockTrue, T_blockObs;
    ErrorStruct(const Vec3f& a, const Vec3f& b, const Vec3f& c)
    : T_robot(a), T_blockTrue(b), T_blockObs(c)
    {
      
    }
  };
  std::vector<ErrorStruct> errorVsDist;
#endif
  
  for(int i_pose=0; i_pose<NumPoses; ++i_pose)
  {
    TimeStamp_t currentTimeStamp = (i_pose+1)*100;

    // Start the robot/world fresh for each pose
    robot.GetBlockWorld().ClearAllExistingObjects();
    ASSERT_TRUE(robot.UpdateCurrPoseFromHistory());
    ASSERT_EQ(robot.AddRawOdomPoseToHistory(currentTimeStamp, robot.GetPoseFrameID(), Pose3d(0, Z_AXIS_3D(), {0, 0, 0}), 0, 0, false), RESULT_OK);
    
    currentTimeStamp += 5;
    
    const Json::Value& jsonData = jsonRoot["Poses"][i_pose];
    
    // Put the robot's head at the right angle *before* queueing up the markers
    float headAngle;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData["RobotPose"], "HeadAngle", headAngle));
    robot.SetHeadAngle(headAngle);
    
    Pose3d trueRobotPose;
    ASSERT_TRUE(JsonTools::GetPoseOptional(jsonData, "RobotPose", trueRobotPose));
    
    // Create a fake robot state message to do a fake RawOdometryPose update.
    // I think we need to make sure we have one for each matPose VisionMarker
    // message.
    RobotState msg;
    msg.timestamp = currentTimeStamp;
    msg.pose_frame_id = robot.GetPoseFrameID();
    msg.headAngle  = headAngle;
    msg.liftAngle  = 0;
    
    // If we're not going to be checking the robot's pose, we need to set it
    // to the ground truth now, *before* queueing up the markers
    if(!checkRobotPose) {
      msg.pose.x = trueRobotPose.GetTranslation().x();
      msg.pose.y = trueRobotPose.GetTranslation().y();
      msg.pose.z = trueRobotPose.GetTranslation().z();
      msg.pose.angle = trueRobotPose.GetRotationAngle<'Z'>().ToFloat();
    } else {
      msg.pose.x = 0;
      msg.pose.y = 0;
      msg.pose.z = 0;
      msg.pose.angle = 0;
    }

    const bool isCarryingObject = (msg.status&(uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK) != 0;
    Pose3d pose( msg.pose.angle, Z_AXIS_3D(), {msg.pose.x, msg.pose.y, msg.pose.y} );
    ASSERT_EQ(robot.AddRawOdomPoseToHistory(msg.timestamp, msg.pose_frame_id, pose, msg.headAngle, msg.liftAngle, isCarryingObject)
      , RESULT_OK
      );
    
    ASSERT_TRUE(robot.UpdateCurrPoseFromHistory());
    

    int NumMarkers;
    ASSERT_TRUE(JsonTools::GetValueOptional(jsonData, "NumMarkers", NumMarkers));
    
    ASSERT_TRUE(jsonData.isMember("VisionMarkers"));
    Json::Value jsonMessages = jsonData["VisionMarkers"];
    ASSERT_EQ(NumMarkers, jsonMessages.size());
    
    
    ASSERT_TRUE(false);
    // THIS NEEDS TO BE UPDATED NOW THAT VisionMarkerMessage IS BOGUS!
    /*
    std::vector<Vision::ObservedMarker> messages;
    messages.reserve(jsonMessages.size());
    
    for(auto & jsonMsg : jsonMessages) {

      // Kludge to convert the string MarkerType stored in the JSON back to an
      // enum value so we can use the (auto-generated) JSON constructor for
      // messages.
      CORETECH_ASSERT(jsonMsg.isMember("markerType"));
      jsonMsg["markerType"] = Vision::StringToMarkerType.at(jsonMsg["markerType"].asString());
      
      MessageVisionMarker msg(jsonMsg);
      msg.timestamp = currentTimeStamp;
      
      Vision::ObservedMarker marker();
      // If we are not checking robot pose, don't queue mat markers
      const bool isMatMarker = !robot.GetBlockWorld().GetObjectLibrary(ObjectFamily::Mat).GetObjectsWithCode(msg.markerType).empty();
      if(!checkRobotPose && isMatMarker) {
        fprintf(stdout, "Skipping mat marker with code = %d ('%s'), since we are not checking robot pose.\n",
                msg.markerType, Vision::MarkerTypeStrings[msg.markerType]);
      } else {
        ASSERT_EQ(robot.QueueObservedMarker(msg), RESULT_OK);
      }
      
    } // for each VisionMarker in the jsonFile
    */
    
    // Process all the markers we've queued
    //uint32_t numObjectsObserved = 0;
    std::list<Vision::ObservedMarker> emptyMarkersList;
    Result result = robot.GetBlockWorld().Update(emptyMarkersList);
    ASSERT_EQ(RESULT_OK, result);
    
    if(checkRobotPose) {
      // TODO: loop over all robots
      
      // Make sure the estimated robot pose matches the ground truth pose
      Vec3f Tdiff;
      const bool robotPoseMatches = trueRobotPose.IsSameAs(robot.GetPose(),
                                                           robotPoseDistThreshold_mm,
                                                           robotPoseAngleThreshold, Tdiff);
      
      fprintf(stdout, "X/Y error in robot pose = %.2fmm, Z error = %.2fmm\n",
              sqrtf(Tdiff.x()*Tdiff.x() + Tdiff.y()*Tdiff.y()), Tdiff.z());
      
      // If the robot's pose is not correct, we can't continue, because
      // all the blocks' poses will also be incorrect
      ASSERT_TRUE(robotPoseMatches);
      
    }
    
    // Use the rest of the VisionMarkers to update the blockworld's pose
    // estimates for the blocks
    //uint32_t numBlocksObserved = blockWorld.UpdateBlockPoses();
    
    
    if(jsonRoot.isMember("Objects"))
    {
      const Json::Value& jsonObject = jsonRoot["Objects"];
      const int numObjectsTrue = jsonObject.size();
      
      //EXPECT_GE(numObjectsObserved, numObjectsTrue); // TODO: Should this be EXPECT_EQ?
      
      //if(numBlocksObserved != numBlocksTrue)
      //  break;
      
      // Check to see that we found and successfully localized each ground truth
      // block
      for(int i_object=0; i_object<numObjectsTrue; ++i_object)
      {
        ObjectType objectType = ObjectType::Invalid;
        std::string objectTypeString;
        ASSERT_TRUE(JsonTools::GetValueOptional(jsonObject[i_object], "Type", objectTypeString));

        // Use the "Name" as the object family in blockworld
        std::string objectFamilyString;
        ASSERT_TRUE(JsonTools::GetValueOptional(jsonObject[i_object], "ObjectName", objectFamilyString));
        ObjectFamily objectFamily;
        if(objectFamilyString == "Block") {
          objectFamily = ObjectFamily::Block;
        } else if(objectFamilyString == "Ramp") {
          objectFamily = ObjectFamily::Ramp;
        }
        
        // TODO: Need a (CLAD-generated) way to lookup ObjectType by string
        //objectType = GetObjectType ObjectType::GetTypeByName(objectTypeString);

        ASSERT_TRUE(objectType != ObjectType::Unknown);
        ASSERT_TRUE(objectType != ObjectType::Invalid);

        const ObservableObject* libObject = NULL;
        
        // TODO: Need a way to get object by type
        //const ObservableObject* libObject = robot.GetBlockWorld().GetObjectLibrary(objectFamily).GetObjectWithType(objectType);

        /*
        int blockTypeAsInt;
        ASSERT_TRUE(JsonTools::GetValueOptional(jsonBlocks[i_block], "Type", blockTypeAsInt));
        const ObjectType blockType(blockTypeAsInt);
        */
        
        // The ground truth block type should be known to the block world
        ASSERT_TRUE(libObject != NULL);
        
        // Set its pose to what is listed in the json file
        Pose3d objectPose;
        ASSERT_TRUE(jsonObject[i_object].isMember("ObjectPose"));
        ASSERT_TRUE(JsonTools::GetPoseOptional(jsonObject[i_object], "ObjectPose", objectPose));
        
        // Make sure this ground truth object was seen and its estimated pose
        // matches the ground truth pose
        
        BlockWorldFilter filter;
        filter.SetAllowedTypes({libObject->GetType()});
        std::vector<ObservableObject*> observedObjects;
        robot.GetBlockWorld().FindMatchingObjects(filter, observedObjects);
        int matchesFound = 0;
        
        /*
        // The threshold will vary with how far away the block actually is
        const float trueDistance = (objectPose.GetTranslation() - trueRobotPose.GetTranslation()).Length();
        const float objectPoseDistThreshold_mm = objectPoseDistThresholdFraction * trueDistance;
        
        Radians objectPoseAngleThreshold;
        if(trueDistance <= objectPoseAngleThresholdNearDistance) {
          objectPoseAngleThreshold = objectPoseAngleNearThreshold;
        } else if(trueDistance >= objectPoseAngleThresholdFarDistance) {
          objectPoseAngleThreshold = objectPoseAngleFarThreshold;
        } else {
          const float slope = ((objectPoseAngleFarThreshold - objectPoseAngleNearThreshold) /
                               (objectPoseAngleThresholdFarDistance - objectPoseAngleThresholdNearDistance));
          objectPoseAngleThreshold = (slope*(trueDistance - objectPoseAngleThresholdNearDistance) +
                                      objectPoseAngleNearThreshold);
        }
         */
        
        for(auto & observedObject : observedObjects)
        {
          objectPose.SetParent(&observedObject->GetPose().FindOrigin());
          
          std::unique_ptr<ObservableObject> groundTruthObject(libObject->CloneType());
          
          groundTruthObject->InitPose(objectPose, PoseState::Unknown); // pose state here shouldn't really matter
          
          
          if(groundTruthObject->IsSameAs(*observedObject))
          {
            if(matchesFound > 0) {
              // We just found multiple matches for this ground truth block
              fprintf(stdout, "Match #%d found for one ground truth %s object.\n",
                      //"T_diff = %.2fmm (vs. %.2fmm), Angle_diff = %.1fdeg (vs. %.1fdeg)\n",
                      matchesFound+1, objectFamilyString.c_str());
                      //P_diff.GetTranslation().Length(),
                      //objectPoseDistThreshold_mm,
                      //P_diff.GetRotationAngle().getDegrees(),
                      //objectPoseAngleThreshold.getDegrees());
              
              /*
              groundTruthObject->IsSameAs(*observedObject.second
                                          objectPoseDistThreshold_mm,
                                          objectPoseAngleThreshold, P_diff);
               */
            }

            if(matchesFound == 0) {
              fprintf(stdout, "Match found for observed %s object.\n", objectFamilyString.c_str());
              /* with "
                      "T_diff = %.2fmm (vs. %.2fmm), Angle_diff = %.1fdeg (vs %.1fdeg)\n",
                      objectFamilyString.c_str(),
                      P_diff.GetTranslation().Length(),
                      objectPoseDistThreshold_mm,
                      P_diff.GetRotationAngle().getDegrees(),
                      objectPoseAngleThreshold.getDegrees());
               */
#if DISPLAY_ERRORS
              const Vec3f& T_true = groundTruthObject->GetPose().GetTranslation();
              
              Vec3f T_dir(T_true - trueRobotPose.GetTranslation());
              const float distance = T_dir.makeUnitLength();
              
              const Vec3f T_error(T_true - observedObject.second->GetPose().GetTranslation());
              /*
               fprintf(stdout, "Block position error = %.1fmm at a distance of %.1fmm\n",
               (T_true - observedBlock.second->GetPose().GetTranslation()).length(),
               ().length());
               */
              //errorVsDist.push_back(std::make_pair( distance, DotProduct(T_error, T_dir) ));
              //errorVsDist.push_back(std::make_pair(distance, (T_true - observedBlock.second->GetPose().GetTranslation()).length()));
              errorVsDist.emplace_back(trueRobotPose.GetTranslation(),
                                       groundTruthObject->GetPose().GetTranslation(), observedObject.second->GetPose().GetTranslation());
#endif
            }

            ++matchesFound;
          } else {
            fprintf(stdout, "Observed type-%d %s object %d at (%.2f,%.2f,%.2f) does not match "
                    "type-%d ground truth at (%.2f,%.2f,%.2f).\n", // T_diff = %2fmm (vs. %.2fmm), "
                    //"Angle_diff = %.1fdeg (vs. %.1fdeg)\n",
                    int(observedObject->GetType()),
                    objectFamilyString.c_str(),
                    int(observedObject->GetID()),
                    observedObject->GetPose().GetTranslation().x(),
                    observedObject->GetPose().GetTranslation().y(),
                    observedObject->GetPose().GetTranslation().z(),
                    int(groundTruthObject->GetType()),
                    groundTruthObject->GetPose().GetTranslation().x(),
                    groundTruthObject->GetPose().GetTranslation().y(),
                    groundTruthObject->GetPose().GetTranslation().z());
                    //P_diff.GetTranslation().Length(),
                    //objectPoseDistThreshold_mm,
                    //P_diff.GetRotationAngle().getDegrees(),
                    //objectPoseAngleThreshold.getDegrees());
          }
          
        } // for each observed object
        
        EXPECT_EQ(matchesFound, 1); // Exactly one observed object should match
        
      } // for each ground truth object
      
    } // IF there are blocks
    else {
      //EXPECT_EQ(0, numObjectsObserved) <<
      //"No objects are defined in the JSON file, but some were observed.";
    }
  
  } // FOR each pose
  
#if DISPLAY_ERRORS
  // Plot the error vs distance in Matlab
  //Anki::Embedded::Matlab matlab(false);
  fprintf(stdout, "Paste this into Matlab to get an error vs. distance plot:\n");
  fprintf(stdout, "errorVsDist = [");
  for(auto measurement : errorVsDist) {
    fprintf(stdout, "%f %f %f   %f %f %f   %f %f %f;\n",
            measurement.T_robot.x(), measurement.T_robot.y(), measurement.T_robot.z(),
            measurement.T_blockTrue.x(), measurement.T_blockTrue.y(), measurement.T_blockTrue.z(),
            measurement.T_blockObs.x(), measurement.T_blockObs.y(), measurement.T_blockObs.z());
            
  }
  fprintf(stdout, "];\n");
#endif
  
} // TEST_P(BlockWorldTest, BlockAndRobotLocalization)


// This is the list of JSON files containing vision test worlds:
// TODO: automatically get all available tests from some directory?
const char *visionTestJsonFiles[] = {
  "visionTest_PoseCluster.json",
  "visionTest_VaryingDistance.json",
  "visionTest_MatPoseTest.json",
  "visionTest_TwoBlocksOnePose.json",
  "visionTest_RepeatedBlock.json",
//  "visionTest_SingleRamp.json", // TODO: Re-enable
  "visionTest_OffTheMat.json"
};

TEST(FactoryTest, IdealCameraPose)
{
  using namespace Anki;
  using namespace Cozmo;
  
  Vision::CameraCalibration calib(240, 320, 290.f, 290.f, 160.f, 120.f);
  const f32 kDotSpacingX_mm = 40.f;
  const f32 kDotSpacingY_mm = 27.f;
  const f32 kTargetDist_mm  = 60.f;
  const Quad3f kTargetDotCenters(Point3f(-0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0),
                                 Point3f(-0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0),
                                 Point3f( 0.5f*kDotSpacingX_mm, -0.5f*kDotSpacingY_mm, 0),
                                 Point3f( 0.5f*kDotSpacingX_mm,  0.5f*kDotSpacingY_mm, 0));
  
  const std::vector<f32> kPitchAngle_deg = {0, 0.5, 1, 2, 4, 5};
  const std::vector<f32> kYawAngle_deg   = {0, 0.5, 1, 2, 4, 5};
  const std::vector<f32> kRollAngle_deg  = {0, 0.1, 0.5, 1};
  
  
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
        calib.SetFocalLength(288, 287);
        calib.SetCenter(Point2f(155.f, 114.f));
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

TEST(FactoryTest, FindDotsInImages)
{
  using namespace Anki;
  using namespace Cozmo;

  struct TestData
  {
    std::string filename;
    Vision::CameraCalibration calib;
  };
  
  // TODO: Fill in actual calibration data for each test image
  std::vector<TestData> tests = {
    {
      .filename = "test/factoryTests/factoryDotTarget.png",
      .calib = Vision::CameraCalibration(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/factoryDotTarget_trulycam2.png",
      .calib = Vision::CameraCalibration(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/rocky1.png",
      .calib = Vision::CameraCalibration(240, 320, 290.f, 290.f, 160.f, 120.f),
    },
    {
      .filename = "test/factoryTests/rocky2.png",
      .calib = Vision::CameraCalibration(240, 320, 290.f, 290.f, 160.f, 120.f),
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
    ASSERT_EQ(image.GetNumRows(), test.calib.GetNrows());
    ASSERT_EQ(image.GetNumCols(), test.calib.GetNcols());
    
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
                     RAD_TO_DEG_F32(msg.camPoseRoll_rad),
                     RAD_TO_DEG_F32(msg.camPosePitch_rad),
                     RAD_TO_DEG_F32(msg.camPoseYaw_rad));
    
    // TODO: Check the rest of the message contents for sane values
  }
}

/*
 // This actually creates the set of tests, one for each filename above.
INSTANTIATE_TEST_CASE_P(JsonFileBased, BlockWorldTest,
                        ::testing::ValuesIn(visionTestJsonFiles));
*/

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
  std::string filesPath;
  std::string cachePath;
  std::string externalPath;

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
    resourcePath = path + "/resources";
    filesPath = path + "/files";
    cachePath = path + "/temp";
    externalPath = path + "/temp";
  } else {
    resourcePath = configRoot + "/resources";
    filesPath = workRoot + "/files";
    cachePath = workRoot + "/temp";
    externalPath = workRoot + "/temp";
  }
  //LEAKING HERE
  Anki::Util::Data::DataPlatform* dataPlatform = new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcePath);
  cozmoContext = new Anki::Cozmo::CozmoContext(dataPlatform, nullptr);
  
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
