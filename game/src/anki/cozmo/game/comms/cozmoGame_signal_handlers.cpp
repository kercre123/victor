/**
 * File: cozmoGame_U2G_callbacks.cpp
 *
 * Author: Andrew Stein
 * Date:   2/9/2015
 *
 * Description: Implements handlers for signals picked up by CozmoGame
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "cozmoGame_impl.h"

//#include "anki/cozmo/shared/cozmoConfig.h"

//#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/game/signals/cozmoGameSignals.h"

namespace Anki {
namespace Cozmo {


#pragma mark - Base Class Signal Handlers
  
  void CozmoGameImpl::SetupSignalHandlers()
  {
    auto cbDeviceDetectedVisionMarkerSignal = [this](uint8_t engineID, uint32_t markerType,
                                                     float x_upperLeft,  float y_upperLeft,
                                                     float x_lowerLeft,  float y_lowerLeft,
                                                     float x_upperRight, float y_upperRight,
                                                     float x_lowerRight, float y_lowerRight) {
      this->HandleDeviceDetectedVisionMarkerSignal(engineID, markerType,
                                                   x_upperLeft,  y_upperLeft,
                                                   x_lowerLeft,  y_lowerLeft,
                                                   x_upperRight, y_upperRight,
                                                   x_lowerRight, y_lowerRight);
    };
    
    _signalHandles.emplace_back( CozmoEngineSignals::DeviceDetectedVisionMarkerSignal().ScopedSubscribe(cbDeviceDetectedVisionMarkerSignal));
    
    auto cbRobotConnectedSignal = [this](RobotID_t robotID, bool successful) {
      this->HandleRobotConnectedSignal(robotID, successful);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotConnectedSignal().ScopedSubscribe(cbRobotConnectedSignal));
    
    auto cbRobotDisconnectedSignal = [this](RobotID_t robotID) {
      this->HandleRobotDisconnectedSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotDisconnectedSignal().ScopedSubscribe(cbRobotDisconnectedSignal));
    
    auto cbUiDeviceConnectedSignal = [this](UserDeviceID_t deviceID, bool successful) {
      this->HandleUiDeviceConnectedSignal(deviceID, successful);
    };
    _signalHandles.emplace_back( CozmoGameSignals::UiDeviceConnectedSignal().ScopedSubscribe(cbUiDeviceConnectedSignal));
    
    auto cbRobotAvailableSignal = [this](RobotID_t robotID) {
      this->HandleRobotAvailableSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotAvailableSignal().ScopedSubscribe(cbRobotAvailableSignal));
    
    auto cbUiDeviceAvailableSignal = [this](UserDeviceID_t deviceID) {
      this->HandleUiDeviceAvailableSignal(deviceID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::UiDeviceAvailableSignal().ScopedSubscribe(cbUiDeviceAvailableSignal));
    
    auto cbPlaySoundForRobotSignal = [this](RobotID_t robotID, const std::string& soundName, u8 numLoops, u8 volume) {
      this->HandlePlaySoundForRobotSignal(robotID, soundName, numLoops, volume);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::PlaySoundForRobotSignal().ScopedSubscribe(cbPlaySoundForRobotSignal));
    
    auto cbStopSoundForRobotSignal = [this](RobotID_t robotID) {
      this->HandleStopSoundForRobotSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::StopSoundForRobotSignal().ScopedSubscribe(cbStopSoundForRobotSignal));
    
    auto cbRobotObservedObjectSignal = [this](uint8_t robotID, uint32_t objectFamily,
                                              uint32_t objectType, uint32_t objectID,
                                              uint8_t markersVisible,
                                              float img_x_upperLeft,  float img_y_upperLeft,
                                              float img_width,  float img_height,
                                              float world_x, float world_y, float world_z,
                                              float q0, float q1, float q2, float q3,
                                              float topMarkerOrientation_rad,
                                              bool isActive) {
      this->HandleRobotObservedObjectSignal(robotID, objectFamily, objectType, objectID, markersVisible, img_x_upperLeft, img_y_upperLeft, img_width, img_height, world_x, world_y, world_z, q0, q1, q2, q3, topMarkerOrientation_rad, isActive);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotObservedObjectSignal().ScopedSubscribe(cbRobotObservedObjectSignal));
    
    auto cbRobotObservedNothingSignal = [this](uint8_t robotID) {
      this->HandleRobotObservedNothingSignal(robotID);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotObservedNothingSignal().ScopedSubscribe(cbRobotObservedNothingSignal));
    
    auto cbRobotDeletedObjectSignal = [this](uint8_t robotID, uint32_t objectID) {
      this->HandleRobotDeletedObjectSignal(robotID, objectID);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotDeletedObjectSignal().ScopedSubscribe(cbRobotDeletedObjectSignal));
    
    /* Now using U2G message handlers for these
    auto cbConnectToRobotSignal = [this](RobotID_t robotID) {
      this->HandleConnectToRobotSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::ConnectToRobotSignal().ScopedSubscribe(cbConnectToRobotSignal));
    
    auto cbConnectToUiDeviceSignal = [this](UserDeviceID_t deviceID) {
      this->HandleConnectToUiDeviceSignal(deviceID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::ConnectToUiDeviceSignal().ScopedSubscribe(cbConnectToUiDeviceSignal));
    */
    
    // TODO: This should probaly be in base class constructor so clients and hosts can respond
    auto cbRobotImageAvailable = [this](RobotID_t robotID) {
      this->HandleRobotImageAvailable(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotImageAvailableSignal().ScopedSubscribe(cbRobotImageAvailable));
    
    auto cbRobotImageChunkAvailable = [this](RobotID_t robotId, const void* chunk) {
      this->HandleRobotImageChunkAvailable(robotId, chunk);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotImageChunkAvailableSignal().ScopedSubscribe(cbRobotImageChunkAvailable));
    
    auto cbRobotCompletedAction = [this](RobotID_t robotID,
                                         RobotActionType actionType,
                                         ActionResult result,
                                         ActionCompletedStruct completionInfo) {
      this->HandleRobotCompletedAction(robotID, actionType, result, completionInfo);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotCompletedActionSignal().ScopedSubscribe(cbRobotCompletedAction));
    
    auto cbActiveObjectMoved = [this](uint8_t robotID, uint32_t objectID,
                                      float xAccel, float yAccel, float zAccel,
                                      uint8_t upAxis)
    {
      this->HandleActiveObjectMoved(robotID, objectID, xAccel, yAccel, zAccel, upAxis);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::ActiveObjectMovedSignal().ScopedSubscribe(cbActiveObjectMoved));
    
    auto cbActiveObjectStoppedMoving = [this](uint8_t robotID, uint32_t objectID, uint8_t upAxis, bool rolled)
    {
      this->HandleActiveObjectStoppedMoving(robotID, objectID, upAxis, rolled);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::ActiveObjectStoppedMovingSignal().ScopedSubscribe(cbActiveObjectStoppedMoving));
    
    auto cbActiveObjectTapped = [this](uint8_t robotID, uint32_t objectID, uint8_t numTaps)
    {
      this->HandleActiveObjectTapped(robotID, objectID, numTaps);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::ActiveObjectTappedSignal().ScopedSubscribe(cbActiveObjectTapped));
    
  } // SetupSignalHandlers()
  
  
  void CozmoGameImpl::HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                             float x_upperLeft,  float y_upperLeft,
                                                             float x_lowerLeft,  float y_lowerLeft,
                                                             float x_upperRight, float y_upperRight,
                                                             float x_lowerRight, float y_lowerRight)
  {
    // Notify the UI that the device camera saw a VisionMarker
    G2U::DeviceDetectedVisionMarker msg;
    msg.markerType = markerType;
    msg.x_upperLeft = x_upperLeft;
    msg.y_upperLeft = y_upperLeft;
    msg.x_lowerLeft = x_lowerLeft;
    msg.y_lowerLeft = y_lowerLeft;
    msg.x_upperRight = x_upperRight;
    msg.y_upperRight = y_upperRight;
    msg.x_lowerRight = x_lowerRight;
    msg.y_lowerRight = y_lowerRight;
    
    // TODO: Look up which UI device to notify based on the robotID that saw the object
    // TODO: go back to actually sending this as a message instead of storing it for polling
    //_uiMsgHandler.SendMessage(1, message);
    _visionMarkersDetectedByDevice.push_back(msg);
  }
  
  
  
  void CozmoGameImpl::HandleRobotAvailableSignal(RobotID_t robotID) {
    
    // Notify UI that robot is availale and let it issue message to connect
    G2U::RobotAvailable msg;
    msg.robotID = robotID;
    G2U::Message message;
    message.Set_RobotAvailable(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID) {
    
    // Notify UI that a UI device is availale and let it issue message to connect
    G2U::UiDeviceAvailable msg;
    msg.deviceID = deviceID;
    G2U::Message message;
    message.Set_UiDeviceAvailable(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotConnectedSignal(RobotID_t robotID, bool successful)
  {
    G2U::RobotConnected msg;
    msg.robotID = robotID;
    msg.successful = successful;
    G2U::Message message;
    message.Set_RobotConnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotDisconnectedSignal(RobotID_t robotID)
  {
    G2U::RobotDisconnected msg;
    msg.robotID = robotID;
    G2U::Message message;
    message.Set_RobotDisconnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful)
  {
    G2U::UiDeviceConnected msg;
    msg.deviceID = deviceID;
    msg.successful = successful;
    G2U::Message message;
    message.Set_UiDeviceConnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandlePlaySoundForRobotSignal(RobotID_t robotID, const std::string& soundName, u8 numLoops, u8 volume)
  {
#   if ANKI_IOS_BUILD
    // Tell the host UI device to play a sound:
    G2U::PlaySound msg;
    msg.numLoops = numLoops;
    msg.volume   = volume;
    const std::string& filename = SoundManager::getInstance()->GetSoundFile((SoundID_t)soundID);
    strncpy(&(msg.soundFilename[0]), filename.c_str(), msg.soundFilename.size());
    
    G2U::Message message;
    message.Set_PlaySound(msg);
    bool success = RESULT_OK == _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
    
#   else
    // Use SoundManager:
    bool success = SoundManager::getInstance()->Play(soundName,
                                                     numLoops,
                                                     volume);
#   endif
    
    if(!success) {
      PRINT_NAMED_ERROR("CozmoGameImpl.OnEventRaise.PlaySoundForRobot",
                        "SoundManager failed to play sound %s.\n", soundName.c_str());
    }
  }
  
  
  void CozmoGameImpl::HandleStopSoundForRobotSignal(RobotID_t robotID)
  {
#   if ANKI_IOS_BUILD
    // Tell the host UI device to stop the sound
    // TODO: somehow use the robot ID?
    G2U::StopSound msg;
    G2U::Message message;
    message.Set_StopSound(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
#   else
    // Use SoundManager:
    SoundManager::getInstance()->Stop();
#   endif
  }
  
  
  void CozmoGameImpl::HandleRobotObservedObjectSignal(uint8_t robotID,
                                                      uint32_t objectFamily,
                                                      uint32_t objectType,
                                                      uint32_t objectID,
                                                      uint8_t  markersVisible,
                                                      float img_x_upperLeft,  float img_y_upperLeft,
                                                      float img_width,  float img_height,
                                                      float world_x,
                                                      float world_y,
                                                      float world_z,
                                                      float q0, float q1, float q2, float q3,
                                                      float topMarkerOrientation_rad,
                                                      bool isActive)
  {
    // Send a message out to UI that the robot saw an object
    G2U::RobotObservedObject msg;
    msg.robotID       = robotID;
    msg.objectFamily  = objectFamily;
    msg.objectType    = objectType;
    msg.objectID      = objectID;
    msg.img_topLeft_x = img_x_upperLeft;
    msg.img_topLeft_y = img_y_upperLeft;
    msg.img_width     = img_width;
    msg.img_height    = img_height;
    msg.world_x       = world_x;
    msg.world_y       = world_y;
    msg.world_z       = world_z;
    msg.quaternion0   = q0;
    msg.quaternion1   = q1;
    msg.quaternion2   = q2;
    msg.quaternion3   = q3;
    msg.markersVisible= markersVisible;
    msg.topFaceOrientation_rad = topMarkerOrientation_rad;
    msg.isActive      = static_cast<uint8_t>(isActive);
    
    // TODO: Look up which UI device to notify based on the robotID that saw the object
    G2U::Message message;
    message.Set_RobotObservedObject(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotObservedNothingSignal(uint8_t robotID)
  {
    G2U::RobotObservedNothing msg;
    msg.robotID = robotID;
    
    G2U::Message message;
    message.Set_RobotObservedNothing(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotDeletedObjectSignal(uint8_t robotID, uint32_t objectID)
  {
    G2U::RobotDeletedObject msg;
    msg.robotID = robotID;
    msg.objectID = objectID;
    
    G2U::Message message;
    message.Set_RobotDeletedObject(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  /*
  void CozmoGameImpl::HandleConnectToRobotSignal(RobotID_t robotID)
  {
    const bool success = ConnectToRobot(robotID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameImpl.OnEventRaised", "Connected to robot %d!\n", robotID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.OnEventRaised", "Failed to connected to robot %d!\n", robotID);
    }
  }
  
  void CozmoGameImpl::HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID)
  {
    const bool success = ConnectToUiDevice(deviceID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameImpl.OnEventRaised", "Connected to UI device %d!\n", deviceID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.OnEventRaised", "Failed to connected to UI device %d!\n", deviceID);
    }
  }
   */
  
  void CozmoGameImpl::HandleRobotImageAvailable(RobotID_t robotID)
  {
    auto ismIter = _imageSendMode.find(robotID);
    
    if(ismIter == _imageSendMode.end()) {
      _imageSendMode[robotID] = ISM_OFF;
    } else if (ismIter->second != ISM_OFF) {
      
      const bool imageSent = SendRobotImage(robotID);
      
      if(imageSent && ismIter->second == ISM_SINGLE_SHOT) {
        // We were just in single-image send mode, and the image got sent, so
        // go back to "off". (If in stream mode, stay in stream mode.)
        ismIter->second = ISM_OFF;
      }
    }
  }
  
  void CozmoGameImpl::HandleRobotImageChunkAvailable(RobotID_t robotID, const void *chunkMsg)
  {
    auto ismIter = _imageSendMode.find(robotID);
    
    if(ismIter == _imageSendMode.end()) {
      _imageSendMode[robotID] = ISM_OFF;
    } else if (ismIter->second != ISM_OFF) {
      
      const MessageImageChunk *robotImgChunk = reinterpret_cast<const MessageImageChunk*>(chunkMsg);
      G2U::ImageChunk uiImgChunk;
      uiImgChunk.imageId = robotImgChunk->imageId;
      uiImgChunk.frameTimeStamp = robotImgChunk->frameTimeStamp;
      uiImgChunk.nrows = Vision::CameraResInfo[robotImgChunk->resolution].height;
      uiImgChunk.ncols = Vision::CameraResInfo[robotImgChunk->resolution].width;
      assert(uiImgChunk.data.size() == robotImgChunk->data.size());
      uiImgChunk.chunkSize = robotImgChunk->chunkSize;
      uiImgChunk.imageEncoding = robotImgChunk->imageEncoding;
      uiImgChunk.imageChunkCount = robotImgChunk->imageChunkCount;
      uiImgChunk.chunkId = robotImgChunk->chunkId;
      std::copy(robotImgChunk->data.begin(), robotImgChunk->data.end(),
                uiImgChunk.data.begin());
      
      G2U::Message msgWrapper;
      msgWrapper.Set_ImageChunk(uiImgChunk);
      _uiMsgHandler.SendMessage(_hostUiDeviceID, msgWrapper);
      
      const bool wasLastChunk = uiImgChunk.chunkId == uiImgChunk.imageChunkCount-1;
      
      if(wasLastChunk && ismIter->second == ISM_SINGLE_SHOT) {
        // We were just in single-image send mode, and the image got sent, so
        // go back to "off". (If in stream mode, stay in stream mode.)
        ismIter->second = ISM_OFF;
      }
    }
  }
  
  void CozmoGameImpl::HandleRobotCompletedAction(uint8_t robotID, RobotActionType actionType,
                                                 ActionResult result,
                                                 ActionCompletedStruct completionInfo)
  {
    G2U::RobotCompletedAction msg;
  
    msg.robotID = robotID;
    msg.actionType = actionType;
    msg.result = result;
    
    msg.completionInfo = completionInfo;
    
    G2U::Message message;
    message.Set_RobotCompletedAction(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  
  void CozmoGameImpl::HandleActiveObjectMoved(uint8_t robotID, uint32_t objectID,
                                              float xAccel, float yAccel, float zAccel,
                                              uint8_t upAxis)
  {
    
    G2U::ActiveObjectMoved msg;
    
    msg.robotID = robotID;
    msg.objectID = objectID;
    msg.xAccel = xAccel;
    msg.yAccel = yAccel;
    msg.zAccel = zAccel;
    msg.upAxis = upAxis;
    
    G2U::Message msgWrapper;
    msgWrapper.Set_ActiveObjectMoved(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msgWrapper);
    
  }
  
  void CozmoGameImpl::HandleActiveObjectStoppedMoving(uint8_t robotID, uint32_t objectID, uint8_t upAxis, bool rolled)
  {
    G2U::ActiveObjectStoppedMoving msg;
    
    msg.robotID  = robotID;
    msg.objectID = objectID;
    msg.upAxis   = upAxis;
    msg.rolled   = rolled;
    
    G2U::Message msgWrapper;
    msgWrapper.Set_ActiveObjectStoppedMoving(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msgWrapper);
  }
  
  void CozmoGameImpl::HandleActiveObjectTapped(uint8_t robotID, uint32_t objectID, uint8_t numTaps)
  {
    G2U::ActiveObjectTapped msg;
    
    msg.robotID  = robotID;
    msg.objectID = objectID;
    msg.numTaps   = numTaps;
    
    G2U::Message msgWrapper;
    msgWrapper.Set_ActiveObjectTapped(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msgWrapper);
  }


} // namespace Cozmo
} // namespace Anki