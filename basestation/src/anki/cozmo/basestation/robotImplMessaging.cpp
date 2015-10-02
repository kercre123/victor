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
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeFstream.h"

namespace Anki {
namespace Cozmo {

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
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::image,
    std::bind(&Robot::HandleImageChunk, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::imuDataChunk,
    std::bind(&Robot::HandleImuData, this, std::placeholders::_1)));

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

  SetCameraCalibration(calib);
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
  StopDocking();
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

  StopDocking();
  StartLookingForMarkers();

}

void Robot::HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const ObjectMoved& payload = message.GetData().Get_activeObjectMoved();
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID

  const BlockWorld::ObjectsMapByType_t& activeBlocksByType =
    GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);

  for(auto objectsByID : activeBlocksByType) {
    for(auto objectWithID : objectsByID.second) {
      ObservableObject* object = objectWithID.second;
      assert(object->IsActive());
      if(object->GetActiveID() == payload.objectID) {
        // TODO: Mark object as de-localized
        PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ActiveObjectMoved",
          "Received message that Object %d (Active ID %d) moved.",
          objectWithID.first.GetValue(), payload.objectID);

        // Don't notify game about moving objects that are being carried
        ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
        assert(actionObject != nullptr);
        if(!actionObject->IsBeingCarried()) {
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ObjectMoved(payload)));
        }

        return;
      }
    }
  }

  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.ActiveObjectMoved",
    "Could not find match for active object ID %d", payload.objectID);
}

void Robot::HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const ObjectStoppedMoving& payload = message.GetData().Get_activeObjectStopped();
  const BlockWorld::ObjectsMapByType_t& activeBlocksByType =
    GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);

  for(auto objectsByID : activeBlocksByType) {
    for(auto objectWithID : objectsByID.second) {
      ObservableObject* object = objectWithID.second;
      assert(object->IsActive());
      if(object->GetActiveID() == payload.objectID) {
        // TODO: Mark object as de-localized
        PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageActiveObjectStoppedMoving",
          "Received message that Object %d (Active ID %d) stopped moving.", objectWithID.first.GetValue(), payload.objectID);

        _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ObjectStoppedMoving(payload)));
        return;
      }
    }
  }

  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageActiveObjectStoppedMoving",
    "Could not find match for active object ID %d", payload.objectID);
}

void Robot::HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const ObjectTapped& payload = message.GetData().Get_activeObjectTapped();
  const BlockWorld::ObjectsMapByType_t& activeBlocksByType =
    GetBlockWorld().GetExistingObjectsByFamily(ObjectFamily::LightCube);

  for(auto objectsByID : activeBlocksByType) {
    for(auto objectWithID : objectsByID.second) {
      ObservableObject* object = objectWithID.second;
      assert(object->IsActive());
      if(object->GetActiveID() == payload.objectID) {

        PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage."
          "MessageActiveObjectTapped",
          "Received message that object %d (Active ID %d) was tapped %d times.",
          objectWithID.first.GetValue(), payload.objectID, payload.numTaps);

        GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ObjectTapped(payload)));

        return;
      }
    }
  }

  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageActiveObjectTapped",
    "Could not find match for active object ID %d", payload.objectID);
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


// For processing image chunks arriving from robot.
// Sends complete images to VizManager for visualization (and possible saving).
void Robot::HandleImageChunk(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const ImageChunk& payload = message.GetData().Get_image();
  const u16 width  = Vision::CameraResInfo[(int)payload.resolution].width;
  const u16 height = Vision::CameraResInfo[(int)payload.resolution].height;

  const bool isImageReady = _imageDeChunker.AppendChunk(payload.imageId, payload.frameTimeStamp,
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
    Vision::Image image;
    cv::Mat cvImg = _imageDeChunker.GetImage();
    if(cvImg.channels() == 1) {
      image = Vision::Image(height, width, cvImg.data);
    } else {
      // TODO: Actually support processing color data (and have ImageRGB object)
      cv::cvtColor(cvImg, cvImg, CV_RGB2GRAY);
      image = Vision::Image(height, width, cvImg.data);
    }

    image.SetTimestamp(payload.frameTimeStamp);

#       if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
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
#       endif // #if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1

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


} // end namespace Cozmo
} // end namespace Anki
