/**
 * File: behaviorPlaypenCameraCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 07/25/17
 *
 * Description: Calibrates Cozmo's camera by using a single calibration target
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"

#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/customObject.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/robot.h"

#include "engine/vision/cameraCalibrator.h"

#include "util/console/consoleSystem.h"

namespace Anki {
namespace Cozmo {

namespace {

#ifdef COZMO_V2
#ifdef SIMULATOR
// V2 Simulated camera calibration
static const Vision::CameraCalibration kApproxCalib(720, 1280,
                                                    507, 507,
                                                    639, 359,
                                                    0.f,
                                                    std::vector<f32>({-0.07f, -0.2f, 0.001f, 0.001f, 0.1f,
                                                                      0.f, 0.f, 0.f}));
#else
// V2 Physical camera calibration
static const Vision::CameraCalibration kApproxCalib(360, 640,
                                                    362, 364,
                                                    303, 196,
                                                    0.f,
                                                    std::vector<f32>({-0.1, -0.1, 0.00005, -0.0001, 0.05,
                                                                      0.f, 0.f, 0.f}));
#endif
#else
// 1.5/1.0 Physical camera calibration
static const Vision::CameraCalibration kApproxCalib(240, 320,
                                                    290, 290,
                                                    160, 120,
                                                    0.f,
                                                    std::vector<f32>({0.f, 0.f, 0.f, 0.f, 0.f,
                                                                      0.f, 0.f, 0.f}));
#endif
}

BehaviorPlaypenCameraCalibration::BehaviorPlaypenCameraCalibration(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({{EngineToGameTag::CameraCalibration,
                    EngineToGameTag::RobotObservedObject}});
}

Result BehaviorPlaypenCameraCalibration::InternalInitInternal(Robot& robot)
{
  // Set CameraCalibrator's CalibTargetType console var to what the playpen config says
  NativeAnkiUtilConsoleSetValueWithString("CalibTargetType", std::to_string(PlaypenConfig::kPlaypenCalibTarget).c_str());

  // Define a custom object with marker kMarkerToTriggerCalibration so we can know when we are seeing the
  // calibration target via a RobotObservedObject message
  CustomObject* customCube = CustomObject::CreateCube(ObjectType::CustomType00,
                                                      PlaypenConfig::kMarkerToTriggerCalibration,
                                                      PlaypenConfig::kCalibMarkerCubeSize_mm,
                                                      PlaypenConfig::kCalibMarkerSize_mm,
                                                      PlaypenConfig::kCalibMarkerSize_mm,
                                                      true);
  robot.GetBlockWorld().DefineObject(std::unique_ptr<CustomObject>(customCube));

  // Set fake calibration if not already set so that we can actually run
  // calibration from images
  if (!robot.GetVisionComponent().IsCameraCalibrationSet())
  {
    PRINT_NAMED_INFO("BehaviorPlaypenCameraCalibration.SettingFakeCalib", "");
    robot.GetVisionComponent().SetCameraCalibration(kApproxCalib);
  }
  
  robot.GetVisionComponent().ClearCalibrationImages();
  
  // Move head and lift so we can see the target
  CompoundActionParallel* action = new CompoundActionParallel(robot, {new MoveHeadToAngleAction(robot, PlaypenConfig::kHeadAngleToSeeTarget),
                                                                      new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK)});
  
  StartActing(action, [this]() {
    AddTimer(PlaypenConfig::kTimeoutWaitingForTarget_ms, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::NOT_SEEING_CALIB_TARGET_TIMEOUT); });
  });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenCameraCalibration::InternalUpdateInternal(Robot& robot)
{
  if(!_computingCalibration)
  {
    // If we are seeing the target and our head is not moving and we haven't yet taken
    // a calibration image then store the next image for camera calibration
    if(!IsActing() &&
       !robot.GetMoveComponent().IsHeadMoving() &&
       robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == 0 &&
       _seeingTarget)
    {
      robot.GetVisionComponent().StoreNextImageForCameraCalibration();
    }
    // Otherwise if we have a calibration image, start computing calibration
    else if(robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == 1)
    {
      robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
      _computingCalibration = true;
      AddTimer(PlaypenConfig::kTimeoutForComputingCalibration_ms, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_TIMED_OUT); });
    }
  }
  
  return BehaviorStatus::Running;
}

void BehaviorPlaypenCameraCalibration::StopInternal(Robot& robot)
{
  robot.GetVisionComponent().ClearCalibrationImages();
  
  // Clear all objects from blockworld since calib target contains markers it will
  // cause objects to get added to the world
  BlockWorldFilter filter;
  filter.SetFilterFcn(nullptr);
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  robot.GetBlockWorld().DeleteLocatedObjects(filter);
  
  _computingCalibration = false;
  
  _seeingTarget = false;
}

void BehaviorPlaypenCameraCalibration::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  if(event.GetData().GetTag() == EngineToGameTag::CameraCalibration)
  {
    HandleCameraCalibration(robot, event.GetData().Get_CameraCalibration());
  }
  else if(event.GetData().GetTag() == EngineToGameTag::RobotObservedObject)
  {
    HandleRobotObservedObject(robot, event.GetData().Get_RobotObservedObject());
  }
}

void BehaviorPlaypenCameraCalibration::HandleCameraCalibration(Robot& robot,
                                                               const CameraCalibration& calibMsg)
{
  Vision::CameraCalibration camCalib(calibMsg.nrows, calibMsg.ncols,
                                     calibMsg.focalLength_x, calibMsg.focalLength_y,
                                     calibMsg.center_x, calibMsg.center_y,
                                     calibMsg.skew,
                                     calibMsg.distCoeffs);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  
  // Write camera calibration to log
  PLAYPEN_TRY(GetLogger().Append(calibMsg), FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  // Write camera calibration to robot
  u8 buf[calibMsg.Size()];
  size_t numBytes = calibMsg.Pack(buf, sizeof(buf));
  WriteToStorage(robot, NVStorage::NVEntryTag::NVEntry_CameraCalib, buf, numBytes,
                 FactoryTestResultCode::CAMERA_CALIB_WRITE_FAILED);
  
  
  // Get calibration image data
  // TODO include how many/which markers were seen
  std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetCalibrationImageJpegData(nullptr);
  
  // Verify number of images
  const u32 NUM_CAMERA_CALIB_IMAGES = 1;
  bool tooManyCalibImages = rawJpegData.size() > NUM_CAMERA_CALIB_IMAGES;
  if (tooManyCalibImages)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.TooManyCalibImagesFound",
                        "%zu images found. Why?", rawJpegData.size());
    rawJpegData.resize(NUM_CAMERA_CALIB_IMAGES);
  }
  
  // Write calibration image to log
  PLAYPEN_TRY(GetLogger().AddFile("calibImage.jpg", *rawJpegData.begin()),
              FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  // Write calibration image to robot
  WriteToStorage(robot, NVStorage::NVEntryTag::NVEntry_CalibImage1,
                 rawJpegData.begin()->data(),
                 rawJpegData.begin()->size(),
                 FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED);
  
  // Check if calibration values are sane
  #define CHECK_OOR(value, min, max) (value < min || value > max)
  if (CHECK_OOR(calibMsg.focalLength_x,
                kApproxCalib.GetFocalLength_x() - PlaypenConfig::kFocalLengthTolerance,
                kApproxCalib.GetFocalLength_x() + PlaypenConfig::kFocalLengthTolerance) ||
      CHECK_OOR(calibMsg.focalLength_y,
                kApproxCalib.GetFocalLength_y() - PlaypenConfig::kFocalLengthTolerance,
                kApproxCalib.GetFocalLength_y() + PlaypenConfig::kFocalLengthTolerance) ||
      CHECK_OOR(calibMsg.center_x,
                kApproxCalib.GetCenter_x() - PlaypenConfig::kCenterTolerance,
                kApproxCalib.GetCenter_x() + PlaypenConfig::kCenterTolerance) ||
      CHECK_OOR(calibMsg.center_y,
                kApproxCalib.GetCenter_y() - PlaypenConfig::kCenterTolerance,
                kApproxCalib.GetCenter_y() + PlaypenConfig::kCenterTolerance) ||
      calibMsg.nrows != kApproxCalib.GetNrows() ||
      calibMsg.ncols != kApproxCalib.GetNcols())
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.Intrinsics.OOR",
                        "focalLength (%f, %f), center (%f, %f)",
                        calibMsg.focalLength_x, calibMsg.focalLength_y, calibMsg.center_x, calibMsg.center_y);
    
    PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_VALUES_OOR);
  }
  
  if(CHECK_OOR(calibMsg.distCoeffs[0],
               kApproxCalib.GetDistortionCoeffs()[0] - PlaypenConfig::kRadialDistortionTolerance,
               kApproxCalib.GetDistortionCoeffs()[0] + PlaypenConfig::kRadialDistortionTolerance) ||
     CHECK_OOR(calibMsg.distCoeffs[1],
               kApproxCalib.GetDistortionCoeffs()[1] - PlaypenConfig::kRadialDistortionTolerance,
               kApproxCalib.GetDistortionCoeffs()[1] + PlaypenConfig::kRadialDistortionTolerance) ||
     CHECK_OOR(calibMsg.distCoeffs[2],
               kApproxCalib.GetDistortionCoeffs()[2] - PlaypenConfig::kTangentialDistortionTolerance,
               kApproxCalib.GetDistortionCoeffs()[2] + PlaypenConfig::kTangentialDistortionTolerance) ||
     CHECK_OOR(calibMsg.distCoeffs[3],
               kApproxCalib.GetDistortionCoeffs()[3] - PlaypenConfig::kTangentialDistortionTolerance,
               kApproxCalib.GetDistortionCoeffs()[3] + PlaypenConfig::kTangentialDistortionTolerance) ||
     CHECK_OOR(calibMsg.distCoeffs[4],
               kApproxCalib.GetDistortionCoeffs()[4] - PlaypenConfig::kRadialDistortionTolerance,
               kApproxCalib.GetDistortionCoeffs()[4] + PlaypenConfig::kRadialDistortionTolerance) ||
     calibMsg.distCoeffs[5] != 0.f ||
     calibMsg.distCoeffs[6] != 0.f ||
     calibMsg.distCoeffs[7] != 0.f)
  {
    std::stringstream ss;
    for(const auto& coeff : calibMsg.distCoeffs)
    {
      ss << coeff << ", ";
    }
    PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.Distortions.OOR",
                        "%s",
                        ss.str().c_str());
    
    PLAYPEN_SET_RESULT(FactoryTestResultCode::DISTORTION_VALUES_OOR);
  }
  
  // Calibration completed so move head to zero and and complete
  StartActing(new MoveHeadToAngleAction(robot, 0), [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS); });
}

void BehaviorPlaypenCameraCalibration::HandleRobotObservedObject(Robot& robot,
                                                                 const ExternalInterface::RobotObservedObject& msg)
{
  if(msg.objectType == ObjectType::CustomType00)
  {
    _seeingTarget = true;
  }
}

}
}

