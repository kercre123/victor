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
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"

#if ANKI_DEV_CHEATS
  #include "cozmoAnim/alexa/devShutdownChecker.h"
#endif

#include <AVSCommon/SDKInterfaces/AudioPlayerInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SoftwareInfoSenderObserverInterface.h>
#include <Alerts/AlertObserverInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <AVSCommon/AVS/IndicatorState.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingCallbacks.h>

#include <functional>
#include <string>
#include <set>
#include <unordered_map>
#include <thread>

namespace alexaClientSDK {
  namespace capabilitiesDelegate { class CapabilitiesDelegate; }
  namespace capabilityAgents { namespace aip { class AudioProvider; } }
  namespace avsCommon {
    namespace utils{ namespace mediaPlayer { class MediaPlayerInterface; } }
  }
}


namespace Anki {
  
namespace Util {
  class Locale;
}
  
namespace Vector {

class AlexaAudioInput;
enum class AlexaAuthState : uint8_t;
enum class AlexaNetworkErrorType : uint8_t;
enum class AlexaUXState : uint8_t;
class AlexaClient;
class AlexaKeywordObserver;
class AlexaMediaPlayer;
class AlexaObserver;
namespace Anim {
  class AnimContext;
}

class AlexaImpl : private Util::noncopyable
{
public:
  
  AlexaImpl();
  
  ~AlexaImpl();
  
  // Starts an async init thread that when complete runs a callback on the same thread as callers to Update()
  using InitCompleteCallback = std::function<void(bool initSuccessful)>;
  void Init( const Anim::AnimContext* context, InitCompleteCallback&& completionCallback );
  
  // If true, the sdk is ready to go (but may not be connected yet)
  bool IsInitialized() const { return _initState == InitState::Completed; }
  
  void Update();
  
  void SetLocale( const Util::Locale& locale );
  
  void Logout();
  
  bool IsAlertActive() const { return _alertActive; }
  
  void StopAlert();
  
  // Adds samples to the mic stream buffer. Should be ok to call on another thread
  void AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples );
  
  // Get the number of samples already added to microphone stream buffer
  uint64_t GetMicrophoneTotalNumSamples() const;
  
  void NotifyOfTapToTalk();
  
  void NotifyOfWakeWord( uint64_t fromSampleIndex, uint64_t toSampleIndex );
  
  // Callback setters. Callbacks will run on the same thread as Update()
  
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
  
  // Removes all callbacks passed above, and also stops internally processing callbacks from the sdk in prep for being destroyed
  void RemoveCallbacksForShutdown();
  
#if ANKI_DEV_CHEATS
  static void ConfirmShutdown();
#endif

  using SourceId = uint64_t; // matches SDK's MediaPlayerInterface::SourceId, static asserted in cpp
  
private:
  using DialogUXState = alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState;
  
  void UpdateAsyncInit();
  void InitThread();
  
  std::vector<std::shared_ptr<std::istream>> GetConfigs() const;
  
  void OnDirective(const std::string& directive, const std::string& payload, const std::string& fullUnparsed);
  
  void SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag );
  
  // considers media player state and dialog state to determine _uxState
  void CheckForUXStateChange();

  void SetNetworkConnectionError();
  void SetNetworkError( AlexaNetworkErrorType errorType );
  bool InteractedRecently() const;
  
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
  void OnAlertState( const std::string& alertID, alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface::State state );
  void OnPlayerActivity( alexaClientSDK::avsCommon::avs::PlayerActivity state );

  // call every tick from update, occasionally this will perform some checks to see if it looks like we are
  // stuck in a UX state bug (e.g. "forever face")
  void CheckStateWatchdog();
  
  // if the watchdog fires, this function attempts to remedy the situation
  void AttemptToFixStuckInSpeakingBug();
  
  void FailInitialization( const std::string& reason );
  
  // readable version int
  alexaClientSDK::avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion GetFirmwareVersion() const;
  
  const Anim::AnimContext* _context = nullptr;
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
  
  // alert info
  bool _alertActive = false;
  bool _backgroundAlertActive = false;
  std::unordered_map<std::string, alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface::State> _alertStates;

  // audio player info (for the audio channel, e.g. flash briefing)
  bool _audioActive = false;
  float _audioActiveLastChangeTime_s = 0.0f;
  
  // todo: merge with _timeToSetIdle_s
  float _nextUXStateCheckTime_s = 0.0f;

  float _lastWatchdogCheckTime_s = 0.0f;
  float _possibleStuckStateStartTime_s = -1.0f;

  // hack to check if time is synced. As of this moment, OSState::IsWallTimeSynced() is not reliable and fast
  // on vicos.... so just track if the system clock jumps and if so, refresh the timers
  std::chrono::time_point<std::chrono::system_clock> _lastWallTime;
  float _lastWallTimeCheck_s;
  
  alexaClientSDK::avsCommon::avs::IndicatorState _notificationsIndicator;
  
  std::shared_ptr<AlexaClient> _client;
  
  std::shared_ptr<AlexaObserver> _observer;
  
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> _capabilitiesDelegate;
  
  std::shared_ptr<alexaClientSDK::settings::SettingCallbacks<alexaClientSDK::settings::DeviceSettingsManager>> _settingsCallbacks;
  
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
  
  InitCompleteCallback _initCompleteCallback;
  
  // handles state of the loading thread. some of this could be done with futures but meh
  enum class InitState : uint8_t {
    Uninitialized=0,
    PreInit,        // AlexaImpl::Init was called
    Initing,        // init thread running
    ThreadComplete, // init thread completed successfully
    ThreadFailed,   // init thread failed
    Completed,      // initialization complete
    Failed,         // initialization failed
  };
  std::atomic<InitState> _initState;
  std::thread _initThread;
  
  std::atomic<bool> _runSetNetworkConnectionError;
  std::atomic<uint64_t> _lastInteractionTime_ms;

#if ANKI_DEV_CHEATS
  static DevShutdownChecker _shutdownChecker;
#endif
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAIMPL_H
