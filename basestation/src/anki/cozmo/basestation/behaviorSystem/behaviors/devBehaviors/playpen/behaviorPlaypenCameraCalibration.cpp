/**
 * File: behaviorPlaypenCameraCalibration.cpp
 *
 * Author: Al Chaussee
 * Created: 07/25/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/customObject.h"
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

#ifdef COZMO_V2
static const Vision::CameraCalibration kApproxCalib(720, 1280,
                                                    507, 507,
                                                    639, 359);
#else
static const Vision::CameraCalibration kApproxCalib(240, 320,
                                                    290, 290,
                                                    160, 120);
#endif

static const u32 kFocalLengthTolerance       = 30;
static const u32 kCenterTolerance            = 30;
static const f32 kHeadAngleToSeeTarget       = MAX_HEAD_ANGLE;
static const u32 kTimeoutWaitingForTarget_ms = 10000;
static const u32 kTimeoutForComputingCalibration_ms = 2000;

static const CustomObjectMarker kMarkerToTriggerCalibration = CustomObjectMarker::Triangles2;
static const f32 kCalibMarkerSize_mm = 15;
static const f32 kCalibMarkerCubeSize_mm = 30;
}

BehaviorPlaypenCameraCalibration::BehaviorPlaypenCameraCalibration(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({{EngineToGameTag::CameraCalibration,
                    EngineToGameTag::RobotObservedObject}});
}

void BehaviorPlaypenCameraCalibration::GetResultsInternal()
{
  
}

Result BehaviorPlaypenCameraCalibration::InternalInitInternal(Robot& robot)
{
  // Define a custom object with marker Diamonds3 so we can know when we are seeing the
  // calibration target via a RobotObservedObject message
  CustomObject* customCube = CustomObject::CreateCube(ObjectType::CustomType00,
                                                      kMarkerToTriggerCalibration,
                                                      kCalibMarkerCubeSize_mm,
                                                      kCalibMarkerSize_mm, kCalibMarkerSize_mm,
                                                      true);
  robot.GetBlockWorld().DefineObject(std::unique_ptr<CustomObject>(customCube));

  // Set fake calibration if not already set so that we can actually run
  // calibration from images.
  if (!robot.GetVisionComponent().IsCameraCalibrationSet())
  {
    PRINT_NAMED_INFO("BehaviorPlaypenCameraCalibration.SettingFakeCalib", "");
    robot.GetVisionComponent().SetCameraCalibration(kApproxCalib);
  }
  
  robot.GetVisionComponent().ClearCalibrationImages();
  
  CompoundActionParallel* action = new CompoundActionParallel(robot, {new MoveHeadToAngleAction(robot, kHeadAngleToSeeTarget),
                                                                      new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK)});
  
  StartActing(action, [this]() {
    AddTimer(kTimeoutWaitingForTarget_ms, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::NOT_SEEING_CALIB_TARGET_TIMEOUT); });
  });
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenCameraCalibration::InternalUpdateInternal(Robot& robot)
{
  if(!_computingCalibration)
  {
    // If we are seeing the star block and our head is not moving then store the next image for
    // camera calibration
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
      AddTimer(kTimeoutForComputingCalibration_ms, [this](){ PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_TIMED_OUT); });
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
  
  _timeStartedWaitingForTarget = 0;
  
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
  PLAYPEN_TRY(robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CameraCalib, buf, numBytes),
              FactoryTestResultCode::NVSTORAGE_WRITE_FAILED);
  
  
  // Get calibration image data
  // TODO include how many/which markers were seen
  std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetCalibrationImageJpegData(nullptr);
  
  // Verify number of images
  const u32 NUM_CAMERA_CALIB_IMAGES = 1;
  bool tooManyCalibImages = rawJpegData.size() > NUM_CAMERA_CALIB_IMAGES;
  if (tooManyCalibImages)
  {
    PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleCameraCalibration.TooManyCalibImagesFound",
                        "%zu images found. Why?", rawJpegData.size());
    rawJpegData.resize(NUM_CAMERA_CALIB_IMAGES);
  }
  
  // Write calibration image to log
  PLAYPEN_TRY(GetLogger().AddFile("calibImage.jpg", *rawJpegData.begin()),
              FactoryTestResultCode::WRITE_TO_LOG_FAILED);
  
  // Write calibration image to robot
  PLAYPEN_TRY(robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CalibImage1,
                                                  rawJpegData.begin()->data(),
                                                  rawJpegData.begin()->size()),
              FactoryTestResultCode::NVSTORAGE_WRITE_FAILED);
  
  // Check if calibration values are sane
  #define CHECK_OOR(value, min, max) (value < min || value > max)
  if (CHECK_OOR(calibMsg.focalLength_x,
                kApproxCalib.GetFocalLength_x() - kFocalLengthTolerance,
                kApproxCalib.GetFocalLength_x() + kFocalLengthTolerance) ||
      CHECK_OOR(calibMsg.focalLength_y,
                kApproxCalib.GetFocalLength_y() - kFocalLengthTolerance,
                kApproxCalib.GetFocalLength_y() + kFocalLengthTolerance) ||
      CHECK_OOR(calibMsg.center_x,
                kApproxCalib.GetCenter_x() - kCenterTolerance,
                kApproxCalib.GetCenter_x() + kCenterTolerance) ||
      CHECK_OOR(calibMsg.center_y,
                kApproxCalib.GetCenter_y() - kCenterTolerance,
                kApproxCalib.GetCenter_y() + kCenterTolerance) ||
      calibMsg.nrows != kApproxCalib.GetNrows() ||
      calibMsg.ncols != kApproxCalib.GetNcols())
  {
    PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleCameraCalibration.OOR",
                        "focalLength (%f, %f), center (%f, %f)",
                        calibMsg.focalLength_x, calibMsg.focalLength_y, calibMsg.center_x, calibMsg.center_y);
    
    // TODO: Specific failures for each check?
    PLAYPEN_SET_RESULT(FactoryTestResultCode::CALIBRATION_VALUES_OOR);
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

