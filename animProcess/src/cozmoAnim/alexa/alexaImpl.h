/**
 * File: alexaImpl.h
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Component that integrates with the Alexa Voice Service (AVS) SDK
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file uses the sdk, here's the SDK license
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

#ifndef ANIMPROCESS_COZMO_ALEXAIMPL_H
#define ANIMPROCESS_COZMO_ALEXAIMPL_H
#pragma once

#include "audioUtil/audioDataTypes.h"
#include "util/helpers/noncopyable.h"

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SoftwareInfoSenderObserverInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <AVSCommon/AVS/IndicatorState.h>

#include <functional>
#include <string>
#include <set>

namespace alexaClientSDK {
  namespace capabilitiesDelegate { class CapabilitiesDelegate; }
  namespace capabilityAgents { namespace aip { class AudioProvider; } }
  namespace avsCommon {
    namespace utils{ namespace mediaPlayer { class MediaPlayerInterface; } }
  }
}


namespace Anki {
namespace Vector {

class AlexaAudioInput;
enum class AlexaAuthState : uint8_t;
enum class AlexaNetworkErrorType : uint8_t;
enum class AlexaUXState : uint8_t;
class AlexaClient;
class AlexaKeywordObserver;
class AlexaMediaPlayer;
class AlexaObserver;
class AnimContext;

class AlexaImpl : private Util::noncopyable
{
public:
  
  AlexaImpl();
  
  ~AlexaImpl();
  
  bool Init( const AnimContext* context );
  
  void Update();
  
  void Logout();
  
  void StopForegroundActivity();
  
  // Adds samples to the mic stream buffer. Should be ok to call on another thread
  void AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples );
  
  void NotifyOfTapToTalk();
  
  void NotifyOfWakeWord( size_t fromSampleIndex, size_t toSampleIndex );
  
  // Callback setters
  
  // this callback should not call AuthDelegate methods
  using OnAlexaAuthStateChanged = std::function<void(AlexaAuthState, const std::string&, const std::string&, bool)>;
  void SetOnAlexaAuthStateChanged( const OnAlexaAuthStateChanged& callback ) { _onAlexaAuthStateChanged = callback; }
  
  // will never call back with ux state Error. see comment in method SetNetworkError.
  using OnAlexaUXStateChanged = std::function<void(AlexaUXState)>;
  void SetOnAlexaUXStateChanged( const OnAlexaUXStateChanged& callback ) { _onAlexaUXStateChanged = callback; }
  
  using OnLogout = std::function<void(void)>;
  void SetOnLogout( const OnLogout& callback ) { _onLogout = callback; }
  
  using OnNetworkError = std::function<void(AlexaNetworkErrorType)>;
  void SetOnNetworkError( const OnNetworkError& callback ) { _onNetworkError = callback; }
  
  using OnNotificationsChanged = std::function<void(bool hasNotification)>;
  void SetOnNotificationsChanged( const OnNotificationsChanged& callback ) { _onNotificationsChanged = callback; }
  
private:
  using DialogUXState = alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState;
  using SourceId = uint64_t; // matches SDK's MediaPlayerInterface::SourceId, static asserted in cpp
  
  std::vector<std::shared_ptr<std::istream>> GetConfigs() const;
  
  void OnDirective(const std::string& directive, const std::string& payload);
  
  void SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag );
  
  // considers media player state and dialog state to determine _uxState
  void CheckForUXStateChange();

  void SetNetworkConnectionError();
  void SetNetworkError( AlexaNetworkErrorType errorType );
  
  // things we care about called by AlexaObserver
  void OnDialogUXStateChanged( DialogUXState state );
  void OnRequestAuthorization( const std::string& url, const std::string& code );
  void OnAuthStateChange( alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
                          alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::Error newError );
  void OnSourcePlaybackChange( SourceId id, bool playing );
  void OnInternetConnectionChanged( bool connected );
  void OnAVSConnectionChanged( const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
                               const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason );
  void OnSendComplete( alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status );
  void OnSDKLogout();
  void OnNotificationsIndicator( alexaClientSDK::avsCommon::avs::IndicatorState state );
  
  
  // readable version int
  alexaClientSDK::avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion GetFirmwareVersion() const;
  
  const AnimContext* _context = nullptr;
  std::string _alexaPersistentFolder;
  std::string _alexaCacheFolder;
  
  // current auth state
  AlexaAuthState _authState;
  // current ux/media player state
  AlexaUXState _uxState;
  // ux state (may be IDLE when audio is playing)
  DialogUXState _dialogState;
  // if the sdk has been able to ping a specific amazon.com endpoint (not the AVS endpoint)
  bool _internetConnected = false;
  // if the sdk was ever connected to AVS
  bool _avsEverConnected = false;
  // sources that are playing (or paused?)
  std::set<SourceId> _playingSources;
  // the last BS time that the sdk received a "Play" or "Speak" directive
  float _lastPlayDirective_s = -1.0f;
  // if non-negative, the update loop with automatically set the ux state to Idle at this time
  float _timeToSetIdle_s = -1.0f;
  // tap to talk is active
  bool _isTapOccurring = false;

  // hack to check if time is synced. As of this moment, OSState::IsWallTimeSynced() is not reliable and fast
  // on vicos.... so just track if the system clock jumps and if so, refresh the timers
  std::chrono::time_point<std::chrono::system_clock> _lastWallTime;
  float _lastWallTimeCheck_s;
  
  alexaClientSDK::avsCommon::avs::IndicatorState _notificationsIndicator;
  
  std::shared_ptr<AlexaClient> _client;
  
  std::shared_ptr<AlexaObserver> _observer;
  
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> _capabilitiesDelegate;
  
  std::shared_ptr<AlexaMediaPlayer> _ttsMediaPlayer;
  std::shared_ptr<AlexaMediaPlayer> _alertsMediaPlayer;
  std::shared_ptr<AlexaMediaPlayer> _audioMediaPlayer;
  std::shared_ptr<AlexaMediaPlayer> _notificationsMediaPlayer;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> _tapToTalkAudioProvider;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> _wakeWordAudioProvider;
  std::shared_ptr<AlexaKeywordObserver> _keywordObserver;
  std::shared_ptr<AlexaAudioInput> _microphone;

  // copy that can be used to dump audio to file. Only used if a console var is set
  std::shared_ptr<AlexaAudioInput> _debugMicrophone;
  
  // callbacks to impl parent Alexa
  OnAlexaAuthStateChanged _onAlexaAuthStateChanged;
  OnAlexaUXStateChanged _onAlexaUXStateChanged;
  OnLogout _onLogout;
  OnNetworkError _onNetworkError;
  OnNotificationsChanged _onNotificationsChanged;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAIMPL_H
