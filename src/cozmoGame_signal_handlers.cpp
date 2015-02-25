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
    
    auto cbRobotDisconnectedSignal = [this](RobotID_t robotID, float timeSinceLastMsg_sec) {
      this->HandleRobotDisconnectedSignal(robotID, timeSinceLastMsg_sec);
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
                                              float x_upperLeft,  float y_upperLeft,
                                              float width,  float height) {
      this->HandleRobotObservedObjectSignal(robotID, objectFamily, objectType, objectID, x_upperLeft, y_upperLeft, width, height);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotObservedObjectSignal().ScopedSubscribe(cbRobotObservedObjectSignal));
    
    auto cbRobotObservedNothingSignal = [this](uint8_t robotID) {
      this->HandleRobotObservedNothingSignal(robotID);
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotObservedNothingSignal().ScopedSubscribe(cbRobotObservedNothingSignal));
    
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
    
  } // SetupSignalHandlers()
  
  
  void CozmoGameImpl::HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                             float x_upperLeft,  float y_upperLeft,
                                                             float x_lowerLeft,  float y_lowerLeft,
                                                             float x_upperRight, float y_upperRight,
                                                             float x_lowerRight, float y_lowerRight)
  {
    // Notify the UI that the device camera saw a VisionMarker
    MessageG2U_DeviceDetectedVisionMarker msg;
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
    //_uiMsgHandler.SendMessage(1, msg);
    _visionMarkersDetectedByDevice.push_back(msg);
  }
  
  
  
  void CozmoGameImpl::HandleRobotAvailableSignal(RobotID_t robotID) {
    
    // Notify UI that robot is availale and let it issue message to connect
    MessageG2U_RobotAvailable msg;
    msg.robotID = robotID;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID) {
    
    // Notify UI that a UI device is availale and let it issue message to connect
    MessageG2U_UiDeviceAvailable msg;
    msg.deviceID = deviceID;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandleRobotConnectedSignal(RobotID_t robotID, bool successful)
  {
    MessageG2U_RobotConnected msg;
    msg.robotID = robotID;
    msg.successful = successful;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandleRobotDisconnectedSignal(RobotID_t robotID, float timeSinceLastMsg_sec)
  {
    MessageG2U_RobotDisconnected msg;
    msg.robotID = robotID;
    msg.timeSinceLastMsg_sec = timeSinceLastMsg_sec;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful)
  {
    MessageG2U_UiDeviceConnected msg;
    msg.deviceID = deviceID;
    msg.successful = successful;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandlePlaySoundForRobotSignal(RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume)
  {
#   if ANKI_IOS_BUILD
    // Tell the host UI device to play a sound:
    MessageG2U_PlaySound msg;
    msg.numLoops = numLoops;
    msg.volume   = volume;
    const std::string& filename = SoundManager::getInstance()->GetSoundFile((SoundID_t)soundID);
    strncpy(&(msg.soundFilename[0]), filename.c_str(), msg.soundFilename.size());
    
    bool success = RESULT_OK == _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
    
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
    MessageG2U_StopSound msg;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
#   else
    // Use SoundManager:
    SoundManager::getInstance()->Stop();
#   endif
  }
  
  
  void CozmoGameImpl::HandleRobotObservedObjectSignal(uint8_t robotID,
                                                      uint32_t objectFamily,
                                                      uint32_t objectType,
                                                      uint32_t objectID,
                                                      float x_upperLeft,  float y_upperLeft,
                                                      float width,  float height) {
    // Send a message out to UI that the device found a vision marker
    MessageG2U_RobotObservedObject msg;
    msg.robotID      = robotID;
    msg.objectFamily = objectFamily;
    msg.objectType   = objectType;
    msg.objectID     = objectID;
    msg.topLeft_x    = x_upperLeft;
    msg.topLeft_y    = y_upperLeft;
    msg.width        = width;
    msg.height       = height;
    
    // TODO: Look up which UI device to notify based on the robotID that saw the object
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }
  
  void CozmoGameImpl::HandleRobotObservedNothingSignal(uint8_t robotID)
  {
    MessageG2U_RobotObservedNothing msg;
    msg.robotID = robotID;
    
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
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
  
} // namespace Cozmo
} // namespace Anki