/**
* File: robotImplMessageing
*
* Author: damjan stulic
* Created: 9/9/15
*
* Description:
* robot class methods specific to message handling
*
* Copyright: Anki, inc. 2015
*
*/

#include <opencv2/imgproc/imgproc.hpp>
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeFstream.h"
#include <functional>

namespace Anki {
namespace Cozmo {
  
using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;

void Robot::InitRobotMessageComponent(RobotInterface::MessageHandler* messageHandler, RobotID_t robotId)
{
  // bind to specific handlers in the robot class
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::cameraCalibration,
    std::bind(&Robot::HandleCameraCalibration, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::printText,
    std::bind(&Robot::HandlePrint, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::blockPickedUp,
    std::bind(&Robot::HandleBlockPickedUp, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::blockPlaced,
    std::bind(&Robot::HandleBlockPlaced, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectMoved,
    std::bind(&Robot::HandleActiveObjectMoved, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectStopped,
    std::bind(&Robot::HandleActiveObjectStopped, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectTapped,
    std::bind(&Robot::HandleActiveObjectTapped, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::goalPose,
    std::bind(&Robot::HandleGoalPose, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::cliffEvent,
    std::bind(&Robot::HandleCliffEvent, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::chargerEvent,
   std::bind(&Robot::HandleChargerEvent, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::image,
    std::bind(&Robot::HandleImageChunk, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::imuDataChunk,
    std::bind(&Robot::HandleImuData, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::syncTimeAck,
    std::bind(&Robot::HandleSyncTimeAck, this, std::placeholders::_1)));

  // lambda wrapper to call internal handler
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::state,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotState& payload = message.GetData().Get_state();
      UpdateFullRobotState(payload);
    }));


  // lambda for some simple message handling
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::animState,
     [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
       _numAnimationBytesPlayed = message.GetData().Get_animState().numAnimBytesPlayed;
       _animationTag = message.GetData().Get_animState().tag;
     }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseStarted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a ramp.", GetID());
      SetOnRamp(true);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseCompleted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a ramp.", GetID());
      SetOnRamp(false);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseStarted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a bridge.", GetID());
      //SetOnBridge(true);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseCompleted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a bridge.", GetID());
      //SetOnBridge(false);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::mainCycleTimeError,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotInterface::MainCycleTimeError& payload = message.GetData().Get_mainCycleTimeError();
      if (payload.numMainTooLongErrors > 0) {
        PRINT_NAMED_WARNING("Robot.MainCycleTooLong", " %d Num errors: %d, Avg time: %d us", GetID(), payload.numMainTooLongErrors, payload.avgMainTooLongTime);
      }
      if (payload.numMainTooLateErrors > 0) {
        PRINT_NAMED_WARNING("Robot.MainCycleTooLate", "%d Num errors: %d, Avg time: %d us", GetID(), payload.numMainTooLateErrors, payload.avgMainTooLateTime);
      }
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::dataDump,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotInterface::DataDump& payload = message.GetData().Get_dataDump();
      char buf[payload.data.size() * 2 + 1];
      FormatBytesAsHex((char *)payload.data.data(), (int)payload.data.size(), buf, (int)sizeof(buf));
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageDataDump", "ID: %d, size: %zd, data: %s", GetID(), payload.data.size(), buf);
    }));
}


void Robot::HandleCameraCalibration(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::CameraCalibration& payload = message.GetData().Get_cameraCalibration();
  PRINT_NAMED_INFO("RobotMessageHandler.CameraCalibration",
    "Received new %dx%d camera calibration from robot.", payload.ncols, payload.nrows);

  // Convert calibration message into a calibration object to pass to
  // the robot
  Vision::CameraCalibration calib(payload.nrows,
    payload.ncols,
    payload.focalLength_x,
    payload.focalLength_y,
    payload.center_x,
    payload.center_y,
    payload.skew);

  _visionComponent.SetCameraCalibration(calib);
  SetPhysicalRobot(payload.isPhysicalRobots);
}

void Robot::HandlePrint(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::PrintText& payload = message.GetData().Get_printText();
  printf("ROBOT-PRINT (%d): %s", GetID(), payload.text.c_str());
}

void Robot::HandleBlockPickedUp(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const BlockPickedUp& payload = message.GetData().Get_blockPickedUp();
  const char* successStr = (payload.didSucceed ? "succeeded" : "failed");
  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPickedUp",
    "Robot %d reported it %s picking up block. Stopping docking and turning on Look-for-Markers mode.", GetID(), successStr);

  if(payload.didSucceed) {
    SetDockObjectAsAttachedToLift();
    SetLastPickOrPlaceSucceeded(true);
  }
  else {
    SetLastPickOrPlaceSucceeded(false);
  }

  // Note: this returns the vision system to whatever mode it was in before
  // it was docking/tracking
  _visionComponent.EnableMode(VisionMode::Tracking, false);
}

void Robot::HandleBlockPlaced(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const BlockPlaced& payload = message.GetData().Get_blockPlaced();
  const char* successStr = (payload.didSucceed ? "succeeded" : "failed");
  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPlaced",
    "Robot %d reported it %s placing block. Stopping docking and turning on Look-for-Markers mode.", GetID(), successStr);

  if(payload.didSucceed) {
    SetCarriedObjectAsUnattached();
    SetLastPickOrPlaceSucceeded(true);
  }
  else {
    SetLastPickOrPlaceSucceeded(false);
  }

  _visionComponent.EnableMode(VisionMode::DetectingMarkers, true);
  _visionComponent.EnableMode(VisionMode::Tracking, false);

}

void Robot::HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectMoved payload = message.GetData().Get_activeObjectMoved();
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID
  ObservableObject* object = GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("Robot.HandleActiveObjectMoved.UnknownActiveID",
                        "Could not find match for active object ID %d", payload.objectID);
  }
  // Ignore move messages for objects we are docking to, since we expect to bump them
  else if(object->GetID() != GetDockObject())
  {
    ASSERT_NAMED(object->IsActive(), "Got movement message from non-active object?");
    
    if(object->GetPoseState() == ObservableObject::PoseState::Known)
    {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.ActiveObjectMoved",
                       "Received message that %s %d (Active ID %d) moved. Delocalizing it.",
                       EnumToString(object->GetType()),
                       object->GetID().GetValue(), object->GetActiveID());
      
      // Once an object moves, we can no longer use it for localization because
      // we don't know where it is anymore. Next time we see it, relocalize it
      // relative to robot's pose estimate. Then we can use it for localization
      // again.
      object->SetPoseState(ObservableObject::PoseState::Dirty);
      
      // If this is the object we were localized to, unset our localizedToID.
      // Note we are still "localized" by odometry, however.
      if(GetLocalizedTo() == object->GetID()) {
        ASSERT_NAMED(IsLocalized(), "Robot should think it is localized if GetLocalizedTo is set to something.");
        PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.UnsetLocalzedToID",
                         "Unsetting %s %d, which moved, as robot %d's localization object.",
                         ObjectTypeToString(object->GetType()), object->GetID().GetValue(), GetID());
        SetLocalizedTo(nullptr);
      } else if(!IsLocalized() && !IsPickedUp()) {
        // If we are not localized and there is nothing else left in the world that
        // we could localize to, then go ahead and mark us as localized (via
        // odometry alone)
        if(false == _blockWorld.AnyRemainingLocalizableObjects()) {
          PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.NoMoreRemainingLocalizableObjects",
                           "Marking previously-unlocalized robot %d as localized to odometry because "
                           "there are no more objects to localize to in the world.", GetID());
          SetLocalizedTo(nullptr);
        }
      }
      
    }
    
    // Don't notify game about moving objects that are being carried
    ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
    assert(actionObject != nullptr);
    if(!actionObject->IsBeingCarried()) {
      // Update the ID to be the blockworld ID before broadcasting
      payload.objectID = object->GetID();
      Broadcast(ExternalInterface::MessageEngineToGame(ObjectMoved(payload)));
    }
  }
}

void Robot::HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectStoppedMoving payload = message.GetData().Get_activeObjectStopped();
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID
  ObservableObject* object = GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.UnknownActiveID",
                     "Could not find match for active object ID %d", payload.objectID);
  }
  // Ignore stopped-moving messages for objects we are docking to, since we expect to bump them
  else if(object->GetID() != GetDockObject())
  {
    ASSERT_NAMED(object->IsActive(), "Got movement message from non-active object?");

    PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.MessageActiveObjectStoppedMoving",
                     "Received message that %s %d (Active ID %d) stopped moving.",
                     EnumToString(object->GetType()),
                     object->GetID().GetValue(), payload.objectID);
    
    if(object->GetPoseState() == ObservableObject::PoseState::Known) {
      // Not sure how an object could have a known pose before it stopped moving,
      // but just to be safe, re-delocalize and force a re-localization now
      // that we've gotten the stopped-moving message.
      object->SetPoseState(ObservableObject::PoseState::Dirty);
      
      // If this is the object we were localized to, unset our localizedToID.
      // Note we are still "localized" by odometry, however.
      if(GetLocalizedTo() == object->GetID()) {
        ASSERT_NAMED(IsLocalized(), "Robot should think it is localized if GetLocalizedTo is set to something.");
        PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.UnsetLocalzedToID",
                         "Unsetting %s %d, which stopped moving, as robot %d's localization object.",
                         ObjectTypeToString(object->GetType()), object->GetID().GetValue(), GetID());
        SetLocalizedTo(nullptr);
      } else if(!IsLocalized() && !IsPickedUp()) {
        // If we are not localized and there is nothing else left in the world that
        // we could localize to, then go ahead and mark us as localized (via
        // odometry alone)
        if(false == _blockWorld.AnyRemainingLocalizableObjects()) {
          PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.NoMoreRemainingLocalizableObjects",
                           "Marking previously-unlocalized robot %d as localized to odometry because "
                           "there are no more objects to localize to in the world.", GetID());
          SetLocalizedTo(nullptr);
        }
      }
    }
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = object->GetID();
    Broadcast(ExternalInterface::MessageEngineToGame(ObjectStoppedMoving(payload)));
  }
}

void Robot::HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectTapped payload = message.GetData().Get_activeObjectTapped();
  ObservableObject* object = GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_INFO("Robot.HandleActiveObjectTapped.UnknownActiveID",
                     "Could not find match for active object ID %d", payload.objectID);
  } else {
    assert(object->IsActive());
    
    PRINT_NAMED_INFO("Robot.HandleActiveObjectTapped.MessageActiveObjectTapped",
                     "Received message that %s %d (Active ID %d) was tapped %d times.",
                     EnumToString(object->GetType()),
                     object->GetID().GetValue(), payload.objectID, payload.numTaps);
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = object->GetID();
    Broadcast(ExternalInterface::MessageEngineToGame(ObjectTapped(payload)));
  }
}

void Robot::HandleGoalPose(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const GoalPose& payload = message.GetData().Get_goalPose();
  Anki::Pose3d p(payload.pose.angle, Z_AXIS_3D(),
    Vec3f(payload.pose.x, payload.pose.y, payload.pose.z));
  //PRINT_INFO("Goal pose: x=%f y=%f %f deg (%d)", msg.pose_x, msg.pose_y, RAD_TO_DEG_F32(msg.pose_angle), msg.followingMarkerNormal);
  if (payload.followingMarkerNormal) {
    VizManager::getInstance()->DrawPreDockPose(100, p, ::Anki::NamedColors::RED);
  } else {
    VizManager::getInstance()->DrawPreDockPose(100, p, ::Anki::NamedColors::GREEN);
  }
}

  

void Robot::HandleCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  CliffEvent cliffEvent = message.GetData().Get_cliffEvent();
  if (cliffEvent.detected) {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Detected", "at %f,%f",
                     cliffEvent.x_mm, cliffEvent.y_mm);
    
    // Stop whatever we were doing
    GetActionList().Cancel();
    
    // Add cliff obstacle
    Pose3d cliffPose(cliffEvent.angle_rad, Z_AXIS_3D(), {cliffEvent.x_mm, cliffEvent.y_mm, 0}, GetWorldOrigin());
    _blockWorld.AddProxObstacle(cliffPose);
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Undetected", "");
  }
  
  // Forward on with EngineToGame event
  CliffEvent payload = message.GetData().Get_cliffEvent();
  Broadcast(ExternalInterface::MessageEngineToGame(CliffEvent(payload)));
  
}
  
void Robot::HandleChargerEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ChargerEvent chargerEvent = message.GetData().Get_chargerEvent();
  if (chargerEvent.onCharger) {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleChargerEvent.OnCharger", "");
    
    // Stop whatever we were doing
    GetActionList().Cancel();
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleChargerEvent.OffCharger", "");
  }
  
  // Forward on with EngineToGame event
  ChargerEvent payload = message.GetData().Get_chargerEvent();
  Broadcast(ExternalInterface::MessageEngineToGame(ChargerEvent(payload)));
}
  

// For processing image chunks arriving from robot.
// Sends complete images to VizManager for visualization (and possible saving).
void Robot::HandleImageChunk(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // Ignore images if robot has not yet acknowledged time sync
  if (!_timeSynced)
    return;
  
  const ImageChunk& payload = message.GetData().Get_image();
  const u16 width  = Vision::CameraResInfo[(int)payload.resolution].width;
  const u16 height = Vision::CameraResInfo[(int)payload.resolution].height;

  const bool isImageReady = _imageDeChunker->
  AppendChunk(payload.imageId, payload.frameTimeStamp,
    height, width,
    payload.imageEncoding,
    payload.imageChunkCount,
    payload.chunkId, payload.data.data(), (uint32_t)payload.data.size() );

  if (_externalInterface != nullptr && GetImageSendMode() != ImageSendMode::Off) {
    ExternalInterface::MessageEngineToGame msgWrapper;
    msgWrapper.Set_ImageChunk(payload);
    _externalInterface->Broadcast(msgWrapper);

    const bool wasLastChunk = payload.chunkId == payload.imageChunkCount-1;

    if(wasLastChunk && GetImageSendMode() == ImageSendMode::SingleShot) {
      // We were just in single-image send mode, and the image got sent, so
      // go back to "off". (If in stream mode, stay in stream mode.)
      SetImageSendMode(ImageSendMode::Off);
    }
  }
  VizManager::getInstance()->SendImageChunk(GetID(), payload);

  if(isImageReady)
  {
    cv::Mat cvImg = _imageDeChunker->GetImage();
    if(cvImg.channels() == 1) {
      cv::cvtColor(cvImg, cvImg, CV_GRAY2RGB);
    }

    Vision::ImageRGB image(height,width,cvImg.data);
    image.SetTimestamp(payload.frameTimeStamp);

#   if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
    // Create a 50mb ramdisk on OSX at "/Volumes/RamDisk/" by typing: diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount ram://100000`
    static const char * const g_queueImages_filenamePattern = "/Volumes/RamDisk/robotImage%04d.bmp";
    static const s32 g_queueImages_queueLength = 70; // Must be at least the FPS of the camera. But higher numbers may cause more lag for the consuming process.
    static s32 g_queueImages_queueIndex = 0;
    
    char filename[256];
    snprintf(filename, 256, g_queueImages_filenamePattern, g_queueImages_queueIndex);
    
    cv::imwrite(filename, image.get_CvMat_());
    
    g_queueImages_queueIndex++;
    
    if(g_queueImages_queueIndex >= g_queueImages_queueLength)
      g_queueImages_queueIndex = 0;
#   endif // #if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1

    ProcessImage(image);

  } // if(isImageReady)

}

// For processing imu data chunks arriving from robot.
// Writes the entire log of 3-axis accelerometer and 3-axis
// gyro readings to a .m file in kP_IMU_LOGS_DIR so they
// can be read in from Matlab. (See robot/util/imuLogsTool.m)
void Robot::HandleImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::IMUDataChunk& payload = message.GetData().Get_imuDataChunk();

  // If seqID has changed, then start over.
  if (payload.seqId != _imuSeqID) {
    _imuSeqID = payload.seqId;
    _imuDataSize = 0;
  }

  // Msgs are guaranteed to be received in order so just append data to array
  memcpy(_imuData[0] + _imuDataSize, payload.aX.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);
  memcpy(_imuData[1] + _imuDataSize, payload.aY.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);
  memcpy(_imuData[2] + _imuDataSize, payload.aZ.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);

  memcpy(_imuData[3] + _imuDataSize, payload.gX.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);
  memcpy(_imuData[4] + _imuDataSize, payload.gY.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);
  memcpy(_imuData[5] + _imuDataSize, payload.gZ.data(), (size_t)IMUConstants::IMU_CHUNK_SIZE);

  _imuDataSize += (size_t)IMUConstants::IMU_CHUNK_SIZE;

  // When dataSize matches the expected size, print to file
  if (payload.chunkId == payload.totalNumChunks - 1) {

    // Make sure image capture folder exists
    std::string imuLogsDir = _dataPlatform->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.ProcessIMUDataChunk.CreateDirFailed","%s", imuLogsDir.c_str());
    }

    // Create image file
    char logFilename[564];
    snprintf(logFilename, sizeof(logFilename), "%s/robot%d_imu%d.m", imuLogsDir.c_str(), GetID(), _imuSeqID);
    PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageIMUDataChunk",
      "Printing imu log to %s (dataSize = %d)", logFilename, _imuDataSize);

    std::ofstream oFile(logFilename);
    for (u32 axis = 0; axis < 6; ++axis) {
      oFile << "imuData" << axis << " = [";
      for (u32 i=0; i<_imuDataSize; ++i) {
        oFile << (s32)(_imuData[axis][i]) << " ";
      }
      oFile << "];\n\n";
    }
    oFile.close();
  }
}
  
void Robot::HandleSyncTimeAck(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _timeSynced = true;
}
  

void Robot::SetupMiscHandlers(IExternalInterface& externalInterface)
{
  auto helper = AnkiEventUtil<Robot, decltype(_signalHandles)>(externalInterface, *this, _signalHandles);
  
  using namespace ExternalInterface;
  helper.SubscribeInternal<MessageGameToEngineTag::SetBehaviorSystemEnabled>();
  helper.SubscribeInternal<MessageGameToEngineTag::CancelAction>();
  helper.SubscribeInternal<MessageGameToEngineTag::DrawPoseMarker>();
  helper.SubscribeInternal<MessageGameToEngineTag::IMURequest>();
  helper.SubscribeInternal<MessageGameToEngineTag::EnableRobotPickupParalysis>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetBackpackLEDs>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetIdleAnimation>();
  helper.SubscribeInternal<MessageGameToEngineTag::ReplayLastAnimation>();
  helper.SubscribeInternal<MessageGameToEngineTag::ExecuteTestPlan>();
  helper.SubscribeInternal<MessageGameToEngineTag::SaveImages>();
  helper.SubscribeInternal<MessageGameToEngineTag::SaveRobotState>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetRobotCarryingObject>();
  helper.SubscribeInternal<MessageGameToEngineTag::AbortPath>();
  helper.SubscribeInternal<MessageGameToEngineTag::AbortAll>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetActiveObjectLEDs>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetAllActiveObjectLEDs>();
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetBehaviorSystemEnabled& msg)
{
  _isBehaviorMgrEnabled = msg.enabled;
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::CancelAction& msg)
{
  GetActionList().Cancel(-1, (RobotActionType)msg.actionType);
}

template<>
void Robot::HandleMessage(const ExternalInterface::DrawPoseMarker& msg)
{
  if(IsCarryingObject()) {
    Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0));
    Quad2f objectFootprint = GetBlockWorld().GetObjectByID(GetCarryingObject())->GetBoundingQuadXY(targetPose);
    VizManager::getInstance()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
  }
}

template<>
void Robot::HandleMessage(const ExternalInterface::IMURequest& msg)
{
  RequestIMU(msg.length_ms);
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::EnableRobotPickupParalysis& msg)
{
  SendEnablePickupParalysis(msg.enable);
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  SetBackpackLights(msg.onColor, msg.offColor,
                    msg.onPeriod_ms, msg.offPeriod_ms,
                    msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetIdleAnimation& msg)
{
  _animationStreamer.SetIdleAnimation(msg.animationName);
}

template<>
void Robot::HandleMessage(const ExternalInterface::ReplayLastAnimation& msg)
{
  _animationStreamer.SetStreamingAnimation(_lastPlayedAnimationId, msg.numLoops);
}

template<>
void Robot::HandleMessage(const ExternalInterface::ExecuteTestPlan& msg)
{
  Planning::Path p;
  _longPathPlanner->GetTestPath(GetPose(), p);
  ExecutePath(p);
}

template<>
void Robot::HandleMessage(const ExternalInterface::SaveImages& msg)
{
  _imageSaveMode = (SaveMode_t)msg.mode;
}

template<>
void Robot::HandleMessage(const ExternalInterface::SaveRobotState& msg)
{
  _stateSaveMode = (SaveMode_t)msg.mode;
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::SetRobotCarryingObject& msg)
{
  if(msg.objectID < 0) {
    UnSetCarryingObjects();
  } else {
    SetCarryingObject(msg.objectID);
  }
}

template<>
void Robot::HandleMessage(const ExternalInterface::AbortPath& msg)
{
  AbortDrivingToPose();
}

template<>
void Robot::HandleMessage(const ExternalInterface::AbortAll& msg)
{
  AbortAll();
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetActiveObjectLEDs& msg)
{
  assert(msg.objectID <= s32_MAX);
  SetObjectLights(msg.objectID,
                  msg.whichLEDs,
                  msg.onColor, msg.offColor,
                  msg.onPeriod_ms, msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                  msg.turnOffUnspecifiedLEDs,
                  msg.makeRelative,
                  Point2f(msg.relativeToX, msg.relativeToY));
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetAllActiveObjectLEDs& msg)
{
  assert(msg.objectID <= s32_MAX);
  SetObjectLights(msg.objectID,
                  msg.onColor, msg.offColor,
                  msg.onPeriod_ms, msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                  msg.makeRelative, Point2f(msg.relativeToX, msg.relativeToY));
}
  
void Robot::SetupVisionHandlers(IExternalInterface& externalInterface)
{
  // StartFaceTracking
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::EnableVisionMode,
    [this] (const GameToEngineEvent& event)
    {
      auto const& payload = event.GetData().Get_EnableVisionMode();
      _visionComponent.EnableMode(payload.mode, payload.enable);
    }));
  
  // VisionWhileMoving
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::VisionWhileMoving,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::VisionWhileMoving& msg = event.GetData().Get_VisionWhileMoving();
      EnableVisionWhileMoving(msg.enable);
    }));
}

void Robot::SetupGainsHandlers(IExternalInterface& externalInterface)
{
  // SetWheelControllerGains
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetWheelControllerGains,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::SetWheelControllerGains& msg = event.GetData().Get_SetWheelControllerGains();
      
      SendRobotMessage<RobotInterface::ControllerGains>(msg.kpLeft, msg.kiLeft, 0.0f, msg.maxIntegralErrorLeft,
                                                        Anki::Cozmo::RobotInterface::ControllerChannel::controller_wheel);
    }));
  
  // SetHeadControllerGains
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetHeadControllerGains,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::SetHeadControllerGains& msg = event.GetData().Get_SetHeadControllerGains();
      
      SendRobotMessage<RobotInterface::ControllerGains>(msg.kp, msg.ki, msg.kd, msg.maxIntegralError,
                                                        Anki::Cozmo::RobotInterface::ControllerChannel::controller_head);
    }));
  
  // SetLiftControllerGains
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetLiftControllerGains,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::SetLiftControllerGains& msg = event.GetData().Get_SetLiftControllerGains();
      
      SendRobotMessage<RobotInterface::ControllerGains>(msg.kp, msg.ki, msg.kd, msg.maxIntegralError,
                                                        Anki::Cozmo::RobotInterface::ControllerChannel::controller_lift);
    }));
  
  // SetSteeringControllerGains
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetSteeringControllerGains,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::SetSteeringControllerGains& msg = event.GetData().Get_SetSteeringControllerGains();
      
      SendRobotMessage<RobotInterface::ControllerGains>(msg.k1, msg.k2, 0.0f, 0.0f,
                                                        Anki::Cozmo::RobotInterface::ControllerChannel::controller_stearing);
    }));
}
  
} // end namespace Cozmo
} // end namespace Anki
