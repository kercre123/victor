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
    
    auto cbPlaySoundForRobotSignal = [this](RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume) {
      this->HandlePlaySoundForRobotSignal(robotID, soundID, numLoops, volume);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::PlaySoundForRobotSignal().ScopedSubscribe(cbPlaySoundForRobotSignal));
    
    auto cbStopSoundForRobotSignal = [this](RobotID_t robotID) {
      this->HandleStopSoundForRobotSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::StopSoundForRobotSignal().ScopedSubscribe(cbStopSoundForRobotSignal));
    
    auto cbRobotObservedObjectSignal = [this](uint8_t robotID, uint32_t objectFamily,
                                              uint32_t objectType, uint32_t objectID,
                                              float img_x_upperLeft,  float img_y_upperLeft,
                                              float img_width,  float img_height,
                                              float world_x, float world_y, float world_z) {
      this->HandleRobotObservedObjectSignal(robotID, objectFamily, objectType, objectID, img_x_upperLeft, img_y_upperLeft, img_width, img_height, world_x, world_y, world_z);
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
    
    auto cbRobotCompletedAction = [this](RobotID_t robotID, uint8_t success) {
      this->HandleRobotCompletedAction(robotID, success);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotCompletedActionSignal().ScopedSubscribe(cbRobotCompletedAction));
    
  } // SetupSignalHandlers()
  
  
  void CozmoGameImpl::HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                             float x_upperLeft,  float y_upperLeft,
                                                             float x_lowerLeft,  float y_lowerLeft,
                                                             float x_upperRight, float y_upperRight,
                                                             float x_lowerRight, float y_lowerRight)
  {
    // Notify the UI that the device camera saw a VisionMarker
    G2U_DeviceDetectedVisionMarker msg;
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
    G2U_RobotAvailable msg;
    msg.robotID = robotID;
    G2U_Message message;
    message.Set_RobotAvailable(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID) {
    
    // Notify UI that a UI device is availale and let it issue message to connect
    G2U_UiDeviceAvailable msg;
    msg.deviceID = deviceID;
    G2U_Message message;
    message.Set_UiDeviceAvailable(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotConnectedSignal(RobotID_t robotID, bool successful)
  {
    G2U_RobotConnected msg;
    msg.robotID = robotID;
    msg.successful = successful;
    G2U_Message message;
    message.Set_RobotConnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotDisconnectedSignal(RobotID_t robotID)
  {
    G2U_RobotDisconnected msg;
    msg.robotID = robotID;
    G2U_Message message;
    message.Set_RobotDisconnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful)
  {
    G2U_UiDeviceConnected msg;
    msg.deviceID = deviceID;
    msg.successful = successful;
    G2U_Message message;
    message.Set_UiDeviceConnected(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandlePlaySoundForRobotSignal(RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume)
  {
#   if ANKI_IOS_BUILD
    // Tell the host UI device to play a sound:
    G2U_PlaySound msg;
    msg.numLoops = numLoops;
    msg.volume   = volume;
    const std::string& filename = SoundManager::getInstance()->GetSoundFile((SoundID_t)soundID);
    strncpy(&(msg.soundFilename[0]), filename.c_str(), msg.soundFilename.size());
    
    G2U_Message message;
    message.Set_PlaySound(msg);
    bool success = RESULT_OK == _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
    
#   else
    // Use SoundManager:
    bool success = SoundManager::getInstance()->Play((SoundID_t)soundID,
                                                     numLoops,
                                                     volume);
#   endif
    
    if(!success) {
      PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaise.PlaySoundForRobot",
                        "SoundManager failed to play sound ID %d.\n", soundID);
    }
  }
  
  
  void CozmoGameImpl::HandleStopSoundForRobotSignal(RobotID_t robotID)
  {
#   if ANKI_IOS_BUILD
    // Tell the host UI device to stop the sound
    // TODO: somehow use the robot ID?
    G2U_StopSound msg;
    G2U_Message message;
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
                                                      float img_x_upperLeft,  float img_y_upperLeft,
                                                      float img_width,  float img_height,
                                                      float world_x,
                                                      float world_y,
                                                      float world_z)
  {
    // Send a message out to UI that the robot saw an object
    G2U_RobotObservedObject msg;
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
    
    // TODO: Look up which UI device to notify based on the robotID that saw the object
    G2U_Message message;
    message.Set_RobotObservedObject(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotObservedNothingSignal(uint8_t robotID)
  {
    G2U_RobotObservedNothing msg;
    msg.robotID = robotID;
    
    G2U_Message message;
    message.Set_RobotObservedNothing(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  void CozmoGameImpl::HandleRobotDeletedObjectSignal(uint8_t robotID, uint32_t objectID)
  {
    G2U_RobotDeletedObject msg;
    msg.robotID = robotID;
    msg.objectID = objectID;
    
    G2U_Message message;
    message.Set_RobotDeletedObject(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }
  
  /*
  void CozmoGameImpl::HandleConnectToRobotSignal(RobotID_t robotID)
  {
    const bool success = ConnectToRobot(robotID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to robot %d!\n", robotID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to robot %d!\n", robotID);
    }
  }
  
  void CozmoGameImpl::HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID)
  {
    const bool success = ConnectToUiDevice(deviceID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to UI device %d!\n", deviceID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to UI device %d!\n", deviceID);
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
  
  
  void CozmoGameImpl::HandleRobotCompletedAction(uint8_t robotID, uint8_t success)
  {
    G2U_RobotCompletedAction msg;
  
    msg.robotID = robotID;
    msg.success = success;
    
    G2U_Message message;
    message.Set_RobotCompletedAction(msg);
    _uiMsgHandler.SendMessage(_hostUiDeviceID, message);
  }

} // namespace Cozmo
} // namespace Anki