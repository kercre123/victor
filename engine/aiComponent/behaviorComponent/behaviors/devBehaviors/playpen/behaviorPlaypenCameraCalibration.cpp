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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"

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
namespace Vector {

namespace {

#ifdef SIMULATOR
// V2 Simulated camera calibration
static const std::shared_ptr<Vision::CameraCalibration> kApproxCalib(
  new Vision::CameraCalibration(360, 640,
                                296, 296,
                                319, 179,
                                0.f,
                                std::vector<f32>({0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f})));
#else
// V2 Physical camera calibration
static const std::shared_ptr<Vision::CameraCalibration> kApproxCalib(
  new Vision::CameraCalibration(360, 640,
                                362, 364,
                                303, 196,
                                0.f,
                                std::vector<f32>({-0.1, -0.15, 0.00005, -0.0001, 0.05, 0.f, 0.f, 0.f})));
#endif

  const std::string kWaitingForTargetTimer = "Target";
  const std::string kWaitingForCalibTimer = "Calib";
}

BehaviorPlaypenCameraCalibration::BehaviorPlaypenCameraCalibration(const Json::Value& config)
: IBehaviorPlaypen(config)
{
  SubscribeToTags({{EngineToGameTag::CameraCalibration,
                    EngineToGameTag::RobotObservedObject}});
}

Result BehaviorPlaypenCameraCalibration::OnBehaviorActivatedInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Set CameraCalibrator's CalibTargetType console var to what the playpen config says
  NativeAnkiUtilConsoleSetValueWithString("CalibTargetType",
                                          std::to_string(PlaypenConfig::kPlaypenCalibTarget).c_str());
  
  // Make sure marker detection is enabled for calibration
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);

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
    PRINT_CH_INFO("Behaviors", "BehaviorPlaypenCameraCalibration.SettingFakeCalib", "");
    robot.GetVisionComponent().SetCameraCalibration(kApproxCalib);
  }
  
  robot.GetVisionComponent().ClearCalibrationImages();
  
  // Move head and lift so we can see the target
  CompoundActionParallel* action = new CompoundActionParallel({new MoveHeadToAngleAction(PlaypenConfig::kHeadAngleToSeeTarget_rad),
                                                               new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK)});
  
  DelegateIfInControl(action, [this]() {
    AddTimer(PlaypenConfig::kTimeoutWaitingForTarget_ms, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::NOT_SEEING_CALIB_TARGET_TIMEOUT); }, kWaitingForTargetTimer);
  });
  
  return RESULT_OK;
}

IBehaviorPlaypen::PlaypenStatus BehaviorPlaypenCameraCalibration::PlaypenUpdateInternal()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(!_computingCalibration)
  {
    // If we are seeing the target and our head is not moving and we haven't yet taken
    // a calibration image then store the next image for camera calibration
    // Note: Technically don't need to store a calibration image as single image calibration
    // just uses markers detected from the current image, however, we need to store an image
    // so we can grab it later to write to log
    if(!_waitingToStoreImage &&
       !IsControlDelegated() &&
       !robot.GetMoveComponent().IsHeadMoving() &&
       robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == 0 &&
       _seeingTarget)
    {
      _waitingToStoreImage = true;

      // Wait half a second for marker detection to stabilize
      DelegateIfInControl(new WaitAction(0.5f), [&robot]{
        // Turn CLAHE off
        NativeAnkiUtilConsoleSetValueWithString("UseCLAHE_u8", "0");

        robot.GetVisionComponent().StoreNextImageForCameraCalibration();
      });
    }
    // Otherwise if we have a calibration image, start computing calibration
    else if(!IsControlDelegated() && robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == 1)
    {
      robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
      _computingCalibration = true;
      AddTimer(PlaypenConfig::kTimeoutForComputingCalibration_ms, [this, &robot](){ 
          // We have at least seen the calibration target so we should at least save the image we were trying to use for
          // calibration before failing with calibration timeout
          std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetCalibrationImageJpegData(nullptr);
          
          // Verify number of images
          // 2 images, the second is the undistorted version of the first
          const u32 NUM_CAMERA_CALIB_IMAGES = 2;
          bool invalidNumCalibImages = rawJpegData.size() != NUM_CAMERA_CALIB_IMAGES;
          if (invalidNumCalibImages)
          {
            PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.InvalidNumCalibImagesFound",
                                "%zu images found. Why?", rawJpegData.size());
            rawJpegData.resize(NUM_CAMERA_CALIB_IMAGES);
          }
          
          // Write calibration image to log
          PLAYPEN_TRY(GetLogger().AddFile("calibImage.jpg", *rawJpegData.begin()),
                      FactoryTestResultCode::WRITE_TO_LOG_FAILED);

          PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_TIMED_OUT); 
        }, 
        kWaitingForCalibTimer);
    }
  }
  
  return PlaypenStatus::Running;
}

void BehaviorPlaypenCameraCalibration::OnBehaviorDeactivated()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  robot.GetVisionComponent().ClearCalibrationImages();
  
  // Make sure to disable computing calibration mode in case it was left on for some reason
  robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, false);
  
  // Turn CLAHE back to "WhenDark"
  NativeAnkiUtilConsoleSetValueWithString("UseCLAHE_u8", "4");

  // Clear all objects from blockworld since calib target contains markers it will
  // cause objects to get added to the world
  BlockWorldFilter filter;
  filter.SetFilterFcn(nullptr);
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  robot.GetBlockWorld().DeleteLocatedObjects(filter);
  
  _computingCalibration = false;
  
  _seeingTarget = false;

  _waitingToStoreImage = false;
}

void BehaviorPlaypenCameraCalibration::HandleWhileActivatedInternal(const EngineToGameEvent& event)
{
  if(event.GetData().GetTag() == EngineToGameTag::CameraCalibration)
  {
    HandleCameraCalibration(event.GetData().Get_CameraCalibration());
  }
  else if(event.GetData().GetTag() == EngineToGameTag::RobotObservedObject)
  {
    HandleRobotObservedObject(event.GetData().Get_RobotObservedObject());
  }
}

void BehaviorPlaypenCameraCalibration::HandleCameraCalibration(const CameraCalibration& calibMsg)
{
  // We recieved calibration so remove the waiting for calibration timer
  RemoveTimer(kWaitingForCalibTimer);

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  std::shared_ptr<Vision::CameraCalibration> camCalib(new Vision::CameraCalibration(
    calibMsg.nrows, calibMsg.ncols,
    calibMsg.focalLength_x, calibMsg.focalLength_y,
    calibMsg.center_x, calibMsg.center_y,
    calibMsg.skew,
    calibMsg.distCoeffs));
  
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
  // 2 images, the second is the undistorted version of the first
  const u32 NUM_CAMERA_CALIB_IMAGES = 2;
  bool invalidNumCalibImages = rawJpegData.size() != NUM_CAMERA_CALIB_IMAGES;
  if (invalidNumCalibImages)
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.InvalidNumCalibImagesFound",
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

  // Write calibration image to log
  PLAYPEN_TRY(GetLogger().AddFile("calibImageUndistort.jpg", *(++rawJpegData.begin())),
              FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  // Check if calibration values are sane
  if (!Util::InRange(calibMsg.focalLength_x,
                     kApproxCalib->GetFocalLength_x() - PlaypenConfig::kFocalLengthTolerance,
                     kApproxCalib->GetFocalLength_x() + PlaypenConfig::kFocalLengthTolerance) ||
      !Util::InRange(calibMsg.focalLength_y,
                     kApproxCalib->GetFocalLength_y() - PlaypenConfig::kFocalLengthTolerance,
                     kApproxCalib->GetFocalLength_y() + PlaypenConfig::kFocalLengthTolerance) ||
      !Util::InRange(calibMsg.center_x,
                     kApproxCalib->GetCenter_x() - PlaypenConfig::kCenterTolerance,
                     kApproxCalib->GetCenter_x() + PlaypenConfig::kCenterTolerance) ||
      !Util::InRange(calibMsg.center_y,
                     kApproxCalib->GetCenter_y() - PlaypenConfig::kCenterTolerance,
                     kApproxCalib->GetCenter_y() + PlaypenConfig::kCenterTolerance) ||
      calibMsg.nrows != kApproxCalib->GetNrows() ||
      calibMsg.ncols != kApproxCalib->GetNcols())
  {
    PRINT_NAMED_WARNING("BehaviorPlaypenCameraCalibration.HandleCameraCalibration.Intrinsics.OOR",
                        "focalLength (%f, %f), center (%f, %f)",
                        calibMsg.focalLength_x, calibMsg.focalLength_y, calibMsg.center_x, calibMsg.center_y);
    
    PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_VALUES_OOR);
  }
  
  // Check if distortion values are sane
  if(!Util::InRange(calibMsg.distCoeffs[0],
                    kApproxCalib->GetDistortionCoeffs()[0] - PlaypenConfig::kRadialDistortionTolerance,
                    kApproxCalib->GetDistortionCoeffs()[0] + PlaypenConfig::kRadialDistortionTolerance) ||
     !Util::InRange(calibMsg.distCoeffs[1],
                    kApproxCalib->GetDistortionCoeffs()[1] - PlaypenConfig::kRadialDistortionTolerance,
                    kApproxCalib->GetDistortionCoeffs()[1] + PlaypenConfig::kRadialDistortionTolerance) ||
     !Util::InRange(calibMsg.distCoeffs[2],
                    kApproxCalib->GetDistortionCoeffs()[2] - PlaypenConfig::kTangentialDistortionTolerance,
                    kApproxCalib->GetDistortionCoeffs()[2] + PlaypenConfig::kTangentialDistortionTolerance) ||
     !Util::InRange(calibMsg.distCoeffs[3],
                    kApproxCalib->GetDistortionCoeffs()[3] - PlaypenConfig::kTangentialDistortionTolerance,
                    kApproxCalib->GetDistortionCoeffs()[3] + PlaypenConfig::kTangentialDistortionTolerance) ||
     !Util::InRange(calibMsg.distCoeffs[4],
                    kApproxCalib->GetDistortionCoeffs()[4] - PlaypenConfig::kRadialDistortionTolerance,
                    kApproxCalib->GetDistortionCoeffs()[4] + PlaypenConfig::kRadialDistortionTolerance) ||
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
  DelegateIfInControl(new MoveHeadToAngleAction(0), [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::SUCCESS); });
}

void BehaviorPlaypenCameraCalibration::HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg)
{
  if(msg.objectType == ObjectType::CustomType00)
  {
    _seeingTarget = true;

    // We are seeing the target remove the waiting for target timer
    RemoveTimer(kWaitingForTargetTimer);
  }
}

}
}

