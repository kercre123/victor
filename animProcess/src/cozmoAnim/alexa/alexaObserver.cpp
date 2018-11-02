/**
 * File: alexaObserver.cpp
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Observes many SDK components, runs callbacks, and optionally outputs some debug info
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to UIManager.cpp, some portions were copied. Here's the SDK license
/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "cozmoAnim/alexa/alexaObserver.h"
#include "util/logging/logging.h"

#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>

#include <sstream>

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "Alexa"
#define CONSOLE_LOG(x) LOG_INFO("AlexaObserver.Log", "%s", std::string{x}.c_str())
using namespace alexaClientSDK;
using namespace avsCommon::sdkInterfaces;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaObserver::AlexaObserver()
: _dialogState{ DialogUXState::IDLE }
, _capabilitiesState{ CapabilitiesObserverInterface::State::UNINITIALIZED }
, _capabilitiesError{ CapabilitiesObserverInterface::Error::UNINITIALIZED }
, _authState{ AuthObserverInterface::State::UNINITIALIZED }
, _authCheckCounter{ 0 }
, _connectionStatus{ avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED }
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::Init( const OnDialogUXStateChangedFunc& onDialogUXStateChanged,
                          const OnRequestAuthorizationFunc& onRequestAuthorization,
                          const OnAuthStateChangeFunc& onAuthStateChange,
                          const OnSourcePlaybackChange& onSourcePlaybackChange )
{
  _onDialogUXStateChanged = onDialogUXStateChanged;
  _onRequestAuthorization = onRequestAuthorization;
  _onAuthStateChange = onAuthStateChange;
  _onSourcePlaybackChange = onSourcePlaybackChange;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::Update()
{
  // puts callables on the main thread. the sdk uses its Executor to run things sequentially, but it spins up a bunch
  // of threads
  while( !_workQueue.empty() ) {
    std::function<void(void)> func;
    {
      std::lock_guard<std::mutex> lg(_mutex);
      func = std::move(_workQueue.front());
      _workQueue.pop();
    }
    if( func != nullptr ) {
      func();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::AddToQueue( std::function<void(void)>&& func )
{
  std::lock_guard<std::mutex> lg(_mutex);
  _workQueue.push( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onDialogUXStateChanged( DialogUXState state )
{
  auto func = [this, state]() {
    if( state == _dialogState ) {
      return;
    }
    _dialogState = state;
    PrintState();
    if( _onDialogUXStateChanged != nullptr ) {
      _onDialogUXStateChanged( state );
    }
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onConnectionStatusChanged( const Status status, const ChangedReason reason )
{
  auto func = [this, status]() {
    if (_connectionStatus == status) {
      return;
    }
    _connectionStatus = status;
    PrintState();
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onSettingChanged( const std::string& key, const std::string& value )
{
  auto func = [key, value]() {
    std::string msg = key + " set to " + value;
    CONSOLE_LOG(msg);
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onSpeakerSettingsChanged( const SpeakerManagerObserverInterface::Source& source,
                                              const SpeakerInterface::Type& type,
                                              const SpeakerInterface::SpeakerSettings& settings )
{
  auto func = [source, type, settings]() {
    std::ostringstream oss;
    oss << "SOURCE:" << source << " TYPE:" << type << " VOLUME:" << static_cast<int>(settings.volume)
    << " MUTE:" << settings.mute;
    CONSOLE_LOG(oss.str());
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onSetIndicator( avsCommon::avs::IndicatorState state )
{
  auto func = [state]() {
    std::ostringstream oss;
    oss << "NOTIFICATION INDICATOR STATE: " << state;
    CONSOLE_LOG(oss.str());
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onRequestAuthorization(const std::string& url, const std::string& code)
{
  auto func = [this, url, code]() {
    _authCheckCounter = 0;
    CONSOLE_LOG("NOT YET AUTHORIZED");
    std::ostringstream oss;

    oss << "To authorize, browse to: '" << url << "' and enter the code: " << code;
    CONSOLE_LOG(oss.str());
    if( _onRequestAuthorization != nullptr ) {
      _onRequestAuthorization( url, code );
    }
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onCheckingForAuthorization()
{
  auto func = [this]() {
    std::ostringstream oss;
    oss << "Checking for authorization (" << ++_authCheckCounter << ")...";
    CONSOLE_LOG(oss.str());
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError)
{
  auto func = [this, newState, newError]() {
    if (_authState != newState) {
      _authState = newState;
      switch (_authState) {
        case AuthObserverInterface::State::UNINITIALIZED:
          break;
        case AuthObserverInterface::State::REFRESHED:
          CONSOLE_LOG("Authorized!");
          break;
        case AuthObserverInterface::State::EXPIRED:
          CONSOLE_LOG("AUTHORIZATION EXPIRED");
          break;
        case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
        {
          std::ostringstream oss;
          oss << "UNRECOVERABLE AUTHORIZATION ERROR: " << newError;
          CONSOLE_LOG({oss.str()});
        }
          break;
      }
    }
    if( _onAuthStateChange != nullptr ) {
      _onAuthStateChange( newState, newError );
    }
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onCapabilitiesStateChange( CapabilitiesObserverInterface::State newState,
                                             CapabilitiesObserverInterface::Error newError)
{
  auto func = [this, newState, newError]() {
    if ((_capabilitiesState != newState) && (_capabilitiesError != newError)) {
      _capabilitiesState = newState;
      _capabilitiesError = newError;
      if (CapabilitiesObserverInterface::State::FATAL_ERROR == _capabilitiesState) {
        std::ostringstream oss;
        oss << "UNRECOVERABLE CAPABILITIES API ERROR: " << _capabilitiesError;
        CONSOLE_LOG({oss.str()});
      }
    }
  };
  AddToQueue( std::move(func) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onPlaybackStarted( SourceId id )
{
  auto func = [this,id]() {
    if( _onSourcePlaybackChange != nullptr ) {
      _onSourcePlaybackChange( id, true );
    }
  };
  AddToQueue( std::move(func) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onPlaybackFinished( SourceId id )
{
  auto func = [this,id]() {
    if( _onSourcePlaybackChange != nullptr ) {
      _onSourcePlaybackChange( id, false );
    }
  };
  AddToQueue( std::move(func) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onPlaybackStopped( SourceId id )
{
  auto func = [this,id]() {
    if( _onSourcePlaybackChange != nullptr ) {
      _onSourcePlaybackChange( id, false );
    }
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::onPlaybackError( SourceId id,
                                     const avsCommon::utils::mediaPlayer::ErrorType& type,
                                     std::string error )
{
  PRINT_NAMED_ERROR( "AlexaObserver.onPlaybackError", "Error '%s': %s", errorTypeToString(type).c_str(), error.c_str() );
  auto func = [this,id]() {
    if( _onSourcePlaybackChange != nullptr ) {
      _onSourcePlaybackChange( id, false );
    }
  };
  AddToQueue( std::move(func) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaObserver::PrintState()
{
  if( _connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::DISCONNECTED ) {
    CONSOLE_LOG("Client not connected!");
  } else if( _connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::PENDING ) {
    CONSOLE_LOG("Connecting...");
  } else if( _connectionStatus == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED ) {
    switch( _dialogState ) {
      case DialogUXState::IDLE:
        CONSOLE_LOG("Alexa is currently idle!");
        return;
      case DialogUXState::LISTENING:
        CONSOLE_LOG("Listening...");
        return;
      case DialogUXState::THINKING:
        CONSOLE_LOG("Thinking...");
        return;
      case DialogUXState::SPEAKING:
        CONSOLE_LOG("Speaking...");
        return;
      // This is an intermediate state after a SPEAK directive is completed. In the case of a speech burst the
      // next SPEAK could kick in or if its the last SPEAK directive ALEXA moves to the IDLE state. So we do
      // nothing for this state.
      case DialogUXState::FINISHED:
        return;
    }
  }
}

} // namespace Vector
} // namespace Anki
