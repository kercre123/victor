/**
 * File: alexaClient.h
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Client for amazon voice services. This is an analog to the SDK example
*              DefaultClient that works with other Anki analogs like speakers/mics
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file an an analog to DefaultClient.cpp, some portions were copied. Here's the SDK license
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

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXACLIENT_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXACLIENT_H

#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"

#include <memory>

// todo: forward declare where possible. this is insane
#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/PostConnectSynchronizer.h>
#include <ACL/Transport/PostConnectSynchronizerFactory.h>
#include <ADSL/DirectiveSequencer.h>
#include <AFML/AudioActivityTracker.h>
#include <AFML/FocusManager.h>
#include <AFML/VisualActivityTracker.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <Alerts/AlertsCapabilityAgent.h>
#include <Alerts/Renderer/Renderer.h>
#include <Alerts/Storage/AlertStorageInterface.h>
#include <AudioPlayer/AudioPlayer.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Bluetooth/Bluetooth.h>
#include <Bluetooth/BluetoothStorageInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/SQLiteMessageStorage.h>
#include <DoNotDisturbCA/DoNotDisturbCapabilityAgent.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>
#include <InteractionModel/InteractionModelCapabilityAgent.h>
#include <Notifications/NotificationsCapabilityAgent.h>
#include <Notifications/NotificationRenderer.h>
#include <PlaybackController/PlaybackController.h>
#include <PlaybackController/PlaybackRouter.h>
#include <RegistrationManager/RegistrationManager.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/Settings.h>
#include <Settings/SettingsStorageInterface.h>
#include <Settings/SettingsUpdatedEventSender.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <SpeakerManager/SpeakerManager.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <System/SoftwareInfoSender.h>
#include <System/UserInactivityMonitor.h>
#include <TemplateRuntime/TemplateRuntime.h>

#if ANKI_DEV_CHEATS
  #include "cozmoAnim/alexa/devShutdownChecker.h"
#endif


namespace Anki {
namespace Vector {
  
class AlexaMessageRouter;
class AlexaRevokeAuthHandler;
class AlexaTemplateRuntimeStub; // (dev only)
  
class AlexaClient : private Util::noncopyable
                  , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface
                  , public std::enable_shared_from_this<AlexaClient>
{
public:
  
  ~AlexaClient();
  
  static std::unique_ptr<AlexaClient> Create( std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
                                              std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                                              std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
                                              std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::storage::AlertStorageInterface>   alertStorage,
                                              std::shared_ptr<alexaClientSDK::capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
                                              std::shared_ptr<alexaClientSDK::capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
                                              std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                                              std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
                                              std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
                                              std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface>> messageRequestObservers,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                                              std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                                              std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                                              std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                                              std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
                                              std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
                                              std::shared_ptr<alexaClientSDK::avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
                                              alexaClientSDK::avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion );
  
  void Connect( const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate );
  
  void SendDefaultSettings();
  
  void ChangeSetting( const std::string& key, const std::string& value );
  
  std::future<bool> NotifyOfTapToTalk( alexaClientSDK::capabilityAgents::aip::AudioProvider& tapToTalkAudioProvider,
                                       alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex
                                         = alexaClientSDK::capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX,
                                       std::chrono::steady_clock::time_point startOfSpeechTimestamp = std::chrono::steady_clock::now());
  std::future<bool> NotifyOfTapToTalkEnd();
  
  std::future<bool> NotifyOfWakeWord( alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
                                      alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex,
                                      alexaClientSDK::avsCommon::avs::AudioInputStream::Index endIndex,
                                      const std::string& keyword,
                                      std::chrono::steady_clock::time_point startOfSpeechTimestamp,
                                      const alexaClientSDK::capabilityAgents::aip::ESPData espData
                                        = alexaClientSDK::capabilityAgents::aip::ESPData::getEmptyESPData(),
                                      std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr );
  
  virtual void onCapabilitiesStateChange( alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
                                          alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError ) override;
  
  void StopForegroundActivity();
  
  void StopAlerts();
  
  std::shared_ptr<alexaClientSDK::registrationManager::RegistrationManager> GetRegistrationManager();
  
  std::shared_ptr<alexaClientSDK::settings::DeviceSettingsManager> GetSettingsManager();
  
  void AddSpeakerManagerObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer );
  
  void AddAlexaDialogStateObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer );
  
  void AddInternetConnectionObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer );
  
  void AddRevokeAuthorizationObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer );
  
  void AddNotificationsObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::NotificationsObserverInterface> observer );
  
  void AddAlertObserver( std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::AlertObserverInterface> observer );

  void AddAudioPlayerObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer );

  using OnDirectiveFunc = std::function<void(const std::string&,const std::string&,const std::string&)>;
  void SetDirectiveCallback(const OnDirectiveFunc& onDirective) { _onDirectiveFunc = onDirective; }
  
  bool IsAVSConnected() const;

  // hack to "reset" all of the timers (for use after time is synced)
  void ReinitializeAllTimers();
  
#if ANKI_DEV_CHEATS
  static void ConfirmShutdown() { _shutdownChecker.PrintRemaining(); }
#endif

private:
  
  AlexaClient(){}
  
  bool Init( std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
             std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
             std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
             std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
             std::shared_ptr<alexaClientSDK::capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
             std::shared_ptr<alexaClientSDK::capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
             std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
             std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
             std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
             std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface>> messageRequestObservers,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
             std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
             std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
             std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
             std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
             std::shared_ptr<alexaClientSDK::avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
             alexaClientSDK::avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion );
  
  void AddConnectionObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer );
  void RemoveConnectionObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer );
  
  OnDirectiveFunc _onDirectiveFunc;
  
#if ANKI_DEV_CHEATS
  static DevShutdownChecker _shutdownChecker;
#endif
  
  std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveSequencerInterface> _directiveSequencer;
  
  std::shared_ptr<alexaClientSDK::afml::FocusManager> _audioFocusManager;
  
  std::shared_ptr<alexaClientSDK::afml::AudioActivityTracker> _audioActivityTracker;
  
  std::shared_ptr<AlexaMessageRouter> _messageRouter;
  
  std::shared_ptr<alexaClientSDK::acl::AVSConnectionManager> _connectionManager;
  
  std::shared_ptr<alexaClientSDK::avsCommon::utils::network::InternetConnectionMonitor> _internetConnectionMonitor;
  
  std::shared_ptr<alexaClientSDK::avsCommon::avs::ExceptionEncounteredSender> _exceptionSender;
  
  std::shared_ptr<alexaClientSDK::certifiedSender::CertifiedSender> _certifiedSender;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioInputProcessor> _audioInputProcessor;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::speechSynthesizer::SpeechSynthesizer> _speechSynthesizer;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::audioPlayer::AudioPlayer> _audioPlayer;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::AlertsCapabilityAgent> _alertsCapabilityAgent;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::interactionModel::InteractionModelCapabilityAgent> _interactionCapabilityAgent;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::notifications::NotificationsCapabilityAgent> _notificationsCapabilityAgent;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::system::UserInactivityMonitor> _userInactivityMonitor;
  
  std::shared_ptr<alexaClientSDK::avsCommon::avs::DialogUXStateAggregator> _dialogUXStateAggregator;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::playbackController::PlaybackRouter> _playbackRouter;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::playbackController::PlaybackController> _playbackController;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::settings::Settings> _settings;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::speakerManager::SpeakerManager> _speakerManager;
  
#if ANKI_DEV_CHEATS
  // DEV-ONLY tool for testing
  std::shared_ptr<AlexaTemplateRuntimeStub> _templateRuntime;
#endif
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent> _dndCapabilityAgent;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::system::SoftwareInfoSender> _softwareInfoSender;
  
  std::shared_ptr<AlexaRevokeAuthHandler> _revokeAuthorizationHandler;
  
  std::shared_ptr<alexaClientSDK::registrationManager::RegistrationManager> _registrationManager;
  
  std::shared_ptr<alexaClientSDK::settings::DeviceSettingsManager> _deviceSettingsManager;
  
  std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> _deviceSettingStorage;
  
  // IMPORTANT: Adding another shared_ptr? Keep the order of shared_ptrs of SDK objects in the same
  // order as in the SDK's DefaultClient.h. There could otherwise be a risk of circular refs among
  // shared_ptrs because of liberal use of enable_shared_from_this.
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXACLIENT_H
