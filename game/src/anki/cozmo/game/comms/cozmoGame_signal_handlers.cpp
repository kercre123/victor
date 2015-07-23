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
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {


#pragma mark - Base Class Signal Handlers
  
  void CozmoGameImpl::SetupSignalHandlers()
  {

    auto cbUiDeviceConnectedSignal = [this](UserDeviceID_t deviceID, bool successful) {
      this->HandleUiDeviceConnectedSignal(deviceID, successful);
    };
    _signalHandles.emplace_back( CozmoGameSignals::UiDeviceConnectedSignal().ScopedSubscribe(cbUiDeviceConnectedSignal));

    auto cbUiDeviceAvailableSignal = [this](UserDeviceID_t deviceID) {
      this->HandleUiDeviceAvailableSignal(deviceID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::UiDeviceAvailableSignal().ScopedSubscribe(cbUiDeviceAvailableSignal));

  } // SetupSignalHandlers()

  void CozmoGameImpl::HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID) {
    
    // Notify UI that a UI device is availale and let it issue message to connect
    ExternalInterface::UiDeviceAvailable msg;
    msg.deviceID = deviceID;
    ExternalInterface::MessageEngineToGame message;
    message.Set_UiDeviceAvailable(msg);
    _uiMsgHandler.SendMessage(message);
  }
  


  void CozmoGameImpl::HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful)
  {
    ExternalInterface::UiDeviceConnected msg;
    msg.deviceID = deviceID;
    msg.successful = successful;
    ExternalInterface::MessageEngineToGame message;
    message.Set_UiDeviceConnected(msg);
    _uiMsgHandler.SendMessage(message);
  }
  



} // namespace Cozmo
} // namespace Anki