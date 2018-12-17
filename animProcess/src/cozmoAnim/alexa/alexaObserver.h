/**
 * File: alexaObserver.h
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Observes many SDK components, runs callbacks, and optionally outputs some debug info
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to UIManager.h, some portions were copied. Here's the SDK license
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


#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAOBSERVER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAOBSERVER_H
#pragma once

// todo: fwd declare where possible
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/NotificationsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <Alerts/AlertObserverInterface.h>
#include <RegistrationManager/RegistrationObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>

#include <queue>

namespace Anki {
namespace Vector {
  
class AlexaObserver
  : public alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::SingleSettingObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::NotificationsObserverInterface
  , public alexaClientSDK::authorization::cblAuthDelegate::CBLAuthRequesterInterface
  , public alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface
  , public alexaClientSDK::registrationManager::RegistrationObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface
  , public alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface
  , public alexaClientSDK::avsCommon::sdkInterfaces::AudioPlayerObserverInterface
  
{
public:
  AlexaObserver();
  
  void Update();
  
  // prepares for deletion
  void Shutdown() { _running = false; }
  
  using OnDialogUXStateChangedFunc = std::function<void(DialogUXState)>;
  using OnRequestAuthorizationFunc = std::function<void(const std::string&,const std::string&)>;
  using OnAuthStateChangeFunc = std::function<void(alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State,
                                                   alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::Error)>;
  using OnSourcePlaybackChange = std::function<void(alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId,bool playing)>;
  using OnInternetConnectionChanged = std::function<void(bool connected)>;
  using OnAVSConnectionChanged = std::function<void(const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
                                                    const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason)>;
  using OnSendCompleted = std::function<void(alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status)>;
  using OnLogout = std::function<void(void)>;
  using OnNotificationIndicator = std::function<void(alexaClientSDK::avsCommon::avs::IndicatorState)>;
  using OnAlertState = std::function<void(const std::string&,alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface::State)>;
  using OnPlayerActivity = std::function<void(alexaClientSDK::avsCommon::avs::PlayerActivity)>;
  void Init( const OnDialogUXStateChangedFunc& onDialogUXStateChanged,
             const OnRequestAuthorizationFunc& onRequestAuthorization,
             const OnAuthStateChangeFunc& onAuthStateChange,
             const OnSourcePlaybackChange& onSourcePlaybackChange,
             const OnInternetConnectionChanged& onInternetConnectionChanged,
             const OnAVSConnectionChanged& onAVSConnectionChanged,
             const OnSendCompleted& onSendCompleted,
             const OnLogout& onLogout,
             const OnNotificationIndicator& onNotificationIndicator,
             const OnAlertState& onAlertState,
             const OnPlayerActivity& onPlayerActivity);
  
protected:
  virtual void onDialogUXStateChanged( DialogUXState state ) override;
  
  virtual void onConnectionStatusChanged( const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
                                          const alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason ) override;
  
  virtual void onSettingChanged( const std::string& key, const std::string& value ) override;
  
  virtual void onSpeakerSettingsChanged( const alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
                                         const alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
                                         const alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings ) override;
  
  virtual void onRequestAuthorization(const std::string& url, const std::string& code) override;
  virtual void onCheckingForAuthorization() override;
  
  
  virtual void onAuthStateChange( alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
                                  alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::Error newError ) override;
  
  virtual void onCapabilitiesStateChange( alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
                                          alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError ) override;
  
  // observing speaker state
  virtual void onPlaybackStarted( SourceId id ) override;
  virtual void onPlaybackFinished( SourceId id ) override;
  virtual void onPlaybackStopped( SourceId id ) override;
  virtual void onPlaybackError( SourceId id,
                                const alexaClientSDK::avsCommon::utils::mediaPlayer::ErrorType& type,
                                std::string error ) override;
  
  // internet connection monitoring
  virtual void onConnectionStatusChanged(bool connected) override;
  
  // registration monitoring
  virtual void onLogout() override;
  
  // message request monitoring
  virtual void onSendCompleted( alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status ) override;
  virtual void onExceptionReceived( const std::string &exceptionMessage ) override;
  
  // notifications
  virtual void onSetIndicator( alexaClientSDK::avsCommon::avs::IndicatorState state ) override;
  
  // alerts
  virtual void onAlertStateChange( const std::string& alertToken,
                                   alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface::State state,
                                   const std::string& reason ) override;

  // audio player
  virtual void onPlayerActivityChanged(alexaClientSDK::avsCommon::avs::PlayerActivity state,
                                       const alexaClientSDK::avsCommon::sdkInterfaces::AudioPlayerObserverInterface::Context& context) override;

  
private:
  using SourceId = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;
  
  void AddToQueue( std::function<void(void)>&& func );
  
  // Prints the current state of Alexa after checking what the appropriate message to display is based on the current
  // component states. This should only be used within the internal executor.
  void PrintState();
  
  // The current dialog UX state of the SDK
  DialogUXState _dialogState;
  
  // The current CapabilitiesDelegate state.
  alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State _capabilitiesState;
  
  // The error associated with the CapabilitiesDelegate state.
  alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error _capabilitiesError;
  
  // The current authorization state of the SDK.
  alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State _authState;
  
  // Counter used to make repeated messages about checking for authorization distinguishable from each other.
  int _authCheckCounter;
  
  // The current connection state of the SDK.
  alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status _connectionStatus;
  
  OnDialogUXStateChangedFunc _onDialogUXStateChanged;
  OnRequestAuthorizationFunc _onRequestAuthorization;
  OnAuthStateChangeFunc _onAuthStateChange;
  OnSourcePlaybackChange _onSourcePlaybackChange;
  OnInternetConnectionChanged _onInternetConnectionChanged;
  OnAVSConnectionChanged _onAVSConnectionChanged;
  OnSendCompleted _onSendCompleted;
  OnLogout _onLogout;
  OnNotificationIndicator _onNotificationIndicator;
  OnAlertState _onAlertState;
  OnPlayerActivity _onPlayerActivity;
  
  std::atomic<bool> _running;
  
  std::mutex _mutex;
  using QueueType = std::function<void(void)>;
  std::queue<QueueType> _workQueue;
};

} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXAOBSERVER_H
