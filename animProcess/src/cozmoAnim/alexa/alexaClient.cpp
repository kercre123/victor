/**
 * File: alexaClient.cpp
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Client for amazon voice services. This is an analog to the SDK example
 *              DefaultClient that works with other Anki analogs like speakers/mics.
 *
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

#include "cozmoAnim/alexa/alexaClient.h"

#include "cozmoAnim/alexa/alexaCapabilityWrapper.h"
#include "cozmoAnim/alexa/alexaMessageRouter.h"
#include "cozmoAnim/alexa/alexaObserver.h"
#include "cozmoAnim/alexa/alexaRevokeAuthHandler.h"
#include "cozmoAnim/alexa/alexaTemplateRuntimeStub.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ADSL/MessageInterpreter.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Bluetooth/SQLiteBluetoothStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <ContextManager/ContextManager.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/SettingsUpdatedEventSender.h>
#include <Settings/SQLiteSettingStorage.h>
#include <System/EndpointHandler.h>
#include <System/SystemCapabilityProvider.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <ACL/Transport/PostConnectSynchronizer.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>

#include <memory>
#include <vector>

namespace Anki {
namespace Vector {
  
using namespace alexaClientSDK;
  
namespace {
  #define LOG_CHANNEL "Alexa"
  #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)

#if ANKI_DEV_CHEATS
  CONSOLE_VAR(bool, kDEV_ONLY_EnableAlexaTemplateRendererStub, "Alexa", false);
#endif
}
  
#if ANKI_DEV_CHEATS
  DevShutdownChecker AlexaClient::_shutdownChecker;
  std::list<Anki::Util::IConsoleFunction> sConsoleFuncs;
#endif
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<AlexaClient>
  AlexaClient::Create(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                      std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                      std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                      std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                      std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                      std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
                      std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
                      std::shared_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
                      std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                      std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
                      std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
                      std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>> messageRequestObservers,
                      std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
                      std::shared_ptr<avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
                      avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion )
{
  std::unique_ptr<AlexaClient> client( new AlexaClient{} );
  if( !client->Init( deviceInfo,
                     customerDataManager,
                     authDelegate,
                     messageStorage,
                     alertStorage,
                     notificationsStorage,
                     settingsStorage,
                     deviceSettingStorage,
                     audioFactory,
                     alexaDialogStateObservers,
                     connectionObservers,
                     messageRequestObservers,
                     capabilitiesDelegate,
                     ttsMediaPlayer,
                     alertsMediaPlayer,
                     audioMediaPlayer,
                     notificationsMediaPlayer,
                     ttsSpeaker,
                     alertsSpeaker,
                     audioSpeaker,
                     notificationsSpeaker,
                     internetConnectionMonitor,
                     firmwareVersion ) )
  {
    return nullptr;
  }
  
  return client;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaClient::~AlexaClient()
{
  if( _directiveSequencer ) {
    _directiveSequencer->shutdown();
  }
  if( _speakerManager ) {
    _speakerManager->shutdown();
  }
  if( _audioInputProcessor ) {
    _audioInputProcessor->shutdown();
  }
  if( _audioPlayer ) {
    _audioPlayer->shutdown();
  }
  if( _speechSynthesizer ) {
    _speechSynthesizer->shutdown();
  }
  if( _alertsCapabilityAgent ) {
    _alertsCapabilityAgent->shutdown();
  }
  if( _playbackController ) {
    _playbackController->shutdown();
  }
  if( _messageRouter ) {
    _messageRouter->shutdown();
  }
  if( _connectionManager ) {
    _connectionManager->shutdown();
  }
  if( _certifiedSender ) {
    _certifiedSender->shutdown();
  }
  if( _audioActivityTracker ) {
    _audioActivityTracker->shutdown();
  }
  if( _playbackRouter ) {
    ACSDK_DEBUG5(LX("PlaybackRouterShutdown."));
    _playbackRouter->shutdown();
  }
  if( _notificationsCapabilityAgent ) {
    ACSDK_DEBUG5(LX("NotificationsShutdown."));
    _notificationsCapabilityAgent->shutdown();
  }
  if( _userInactivityMonitor ) {
    _userInactivityMonitor->shutdown();
  }
  if( _dndCapabilityAgent ) {
    RemoveConnectionObserver( _dndCapabilityAgent );
    _dndCapabilityAgent->shutdown();
  }
  if( _softwareInfoSender ) {
    _softwareInfoSender->shutdown();
  }
  if( _deviceSettingStorage ) {
    _deviceSettingStorage->close();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaClient::Init( std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                        std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                        std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                        std::shared_ptr<capabilityAgents::notifications::NotificationsStorageInterface> notificationsStorage,
                        std::shared_ptr<capabilityAgents::settings::SettingsStorageInterface> settingsStorage,
                        std::shared_ptr<settings::storage::DeviceSettingStorageInterface> deviceSettingStorage,
                        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
                        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
                        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>> messageRequestObservers,
                        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> notificationsMediaPlayer,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker,
                        std::shared_ptr<avsCommon::utils::network::InternetConnectionMonitor> internetConnectionMonitor,
                        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion)
{
  
  _internetConnectionMonitor = internetConnectionMonitor;
  
  _dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
  
  for( auto observer : alexaDialogStateObservers ) {
    _dialogUXStateAggregator->addObserver( observer );
  }
  
  _deviceSettingStorage = deviceSettingStorage;
  if( !_deviceSettingStorage->open() ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "deviceSettingStorageOpenFailed"));
    return false;
  }
  
  _deviceSettingsManager = std::make_shared<settings::DeviceSettingsManager>();
  if( !_deviceSettingsManager ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "createDeviceSettingsManagerFailed"));
    return false;
  }
  
  // Creating the Attachment Manager - This component deals with managing attachments and allows for readers and
  // writers to be created to handle the attachment.
  auto attachmentManager = std::make_shared<avsCommon::avs::attachment::AttachmentManager>( avsCommon::avs::attachment::AttachmentManager::AttachmentType::IN_PROCESS );
  
  // Creating the Context Manager - This component manages the context of each of the components to update to AVS.
  // It is required for each of the capability agents so that they may provide their state just before any event is
  // fired off.
  auto contextManager = contextManager::ContextManager::create();
  if( !contextManager ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateContextManager"));
    return false;
  }
  
  // Create a factory to create objects that establish a connection with AVS.
  auto postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create( contextManager );
  
  // Create a factory to create objects that establish a connection with AVS.
  auto transportFactory
    = std::make_shared<acl::HTTP2TransportFactory>( std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
                                                    postConnectSynchronizerFactory );
  
  // Creating the message router - This component actually maintains the connection to AVS over HTTP2. It is created
  // using the auth delegate, which provides authorization to connect to AVS, and the attachment manager, which helps
  // ACL write attachments received from AVS.
  _messageRouter = std::make_shared<AlexaMessageRouter>( messageRequestObservers, authDelegate, attachmentManager, transportFactory );
  
  // Creating the connection manager - This component is the overarching connection manager that glues together all
  // the other networking components into one easy-to-use component.
  _connectionManager = acl::AVSConnectionManager::create( _messageRouter, false, connectionObservers, {_dialogUXStateAggregator} );
  if( !_connectionManager ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionManager"));
    return false;
  }
  
  // Creating our certified sender - this component guarantees that messages given to it (expected to be JSON
  // formatted AVS Events) will be sent to AVS.  This nicely decouples strict message sending from components which
  // require an Event be sent, even in conditions when there is no active AVS connection.
  _certifiedSender = certifiedSender::CertifiedSender::create( _connectionManager, _connectionManager, messageStorage, customerDataManager );
  if( !_certifiedSender ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateCertifiedSender"));
    return false;
  }
  
  // Creating the Exception Sender - This component helps the SDK send exceptions when it is unable to handle a
  // directive sent by AVS. For that reason, the Directive Sequencer and each Capability Agent will need this
  // component.
  _exceptionSender = avsCommon::avs::ExceptionEncounteredSender::create(_connectionManager);
  if( !_exceptionSender ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExceptionSender"));
    return false;
  }
  
  // Creating the Directive Sequencer - This is the component that deals with the sequencing and ordering of
  // directives sent from AVS and forwarding them along to the appropriate Capability Agent that deals with
  // directives in that Namespace/Name.
  _directiveSequencer = adsl::DirectiveSequencer::create( _exceptionSender );
  if( !_directiveSequencer ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDirectiveSequencer"));
    return false;
  }
  
  auto messageInterpreter = std::make_shared<adsl::MessageInterpreter>( _exceptionSender, _directiveSequencer, attachmentManager );
  _connectionManager->addMessageObserver( messageInterpreter );
  
  // Creating the Registration Manager - This component is responsible for implementing any customer registration
  // operation such as login and logout
  _registrationManager = std::make_shared<registrationManager::RegistrationManager>( _directiveSequencer,
                                                                                     _connectionManager,
                                                                                     customerDataManager);
  
  // Creating the Audio Activity Tracker - This component is responsibly for reporting the audio channel focus
  //  information to AVS.
  _audioActivityTracker = afml::AudioActivityTracker::create( contextManager );
  
  // Creating the Focus Manager - This component deals with the management of layered audio focus across various
  // components. It handles granting access to Channels as well as pushing different "Channels" to foreground,
  // background, or no focus based on which other Channels are active and the priorities of those Channels. Each
  // Capability Agent will require the Focus Manager in order to request access to the Channel it wishes to play on.
  _audioFocusManager =
    std::make_shared<afml::FocusManager>( afml::FocusManager::getDefaultAudioChannels(), _audioActivityTracker );
  
  // Creating the User Inactivity Monitor - This component is responsibly for updating AVS of user inactivity as
  // described in the System Interface of AVS.
  _userInactivityMonitor =
    capabilityAgents::system::UserInactivityMonitor::create( _connectionManager, _exceptionSender );
  if( !_userInactivityMonitor ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateUserInactivityMonitor"));
    return false;
  }
  
  // Creating the Audio Input Processor - This component is the Capability Agent that implements the SpeechRecognizer
  // interface of AVS.
  _audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create( _directiveSequencer,
                                                                             _connectionManager,
                                                                             contextManager,
                                                                             _audioFocusManager,
                                                                             _dialogUXStateAggregator,
                                                                             _exceptionSender,
                                                                             _userInactivityMonitor );
  if( !_audioInputProcessor ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioInputProcessor"));
    return false;
  }
  _audioInputProcessor->addObserver( _dialogUXStateAggregator );

  // Creating the Speech Synthesizer - This component is the Capability Agent that implements the SpeechSynthesizer
  // interface of AVS.
  _speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create( ttsMediaPlayer,
                                                                                       _connectionManager,
                                                                                       _audioFocusManager,
                                                                                       contextManager,
                                                                                       _exceptionSender,
                                                                                       _dialogUXStateAggregator );
  if( !_speechSynthesizer ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
    return false;
  }
  _speechSynthesizer->addObserver( _dialogUXStateAggregator );
  
  // Creating the PlaybackController Capability Agent - This component is the Capability Agent that implements the
  // PlaybackController interface of AVS.
  _playbackController = capabilityAgents::playbackController::PlaybackController::create(contextManager, _connectionManager);
  if( !_playbackController ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackController"));
    return false;
  }
  
  // Creating the PlaybackRouter - This component routes a playback button press to the active handler.
  // The default handler is PlaybackController.
  _playbackRouter = capabilityAgents::playbackController::PlaybackRouter::create( _playbackController );
  if( !_playbackRouter ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackRouter"));
    return false;
  }

  // Creating the Audio Player - This component is the Capability Agent that implements the AudioPlayer
  // interface of AVS.
  _audioPlayer = capabilityAgents::audioPlayer::AudioPlayer::create( audioMediaPlayer,
                                                                     _connectionManager,
                                                                     _audioFocusManager,
                                                                     contextManager,
                                                                     _exceptionSender,
                                                                     _playbackRouter );
  if( !_audioPlayer ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioPlayer"));
    return false;
  }
  
  std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allSpeakers = {
    ttsSpeaker,
    alertsSpeaker,
    audioSpeaker,
    notificationsSpeaker,
    // unused speaker types supported by SDK: bluetoothSpeaker, ringtoneSpeaker
  };
  
  // Creating the SpeakerManager Capability Agent - This component is the Capability Agent that implements the
  // Speaker interface of AVS.
  _speakerManager = capabilityAgents::speakerManager::SpeakerManager::create( allSpeakers, contextManager, _connectionManager, _exceptionSender );
  if( !_speakerManager ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeakerManager"));
    return false;
  }
  
  
  _alertsCapabilityAgent = capabilityAgents::alerts::AlertsCapabilityAgent::create( _connectionManager,
                                                                                    _connectionManager,
                                                                                    _certifiedSender,
                                                                                    _audioFocusManager,
                                                                                    _speakerManager,
                                                                                    contextManager,
                                                                                    _exceptionSender,
                                                                                    alertStorage,
                                                                                    audioFactory->alerts(),
                                                                                    capabilityAgents::alerts::renderer::Renderer::create( alertsMediaPlayer ),
                                                                                    customerDataManager );
  if (!_alertsCapabilityAgent) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlertsCapabilityAgent"));
    return false;
  }
  
  AddConnectionObserver( _dialogUXStateAggregator );
  
  // Creating the Notifications Capability Agent - This component is the Capability Agent that implements the
  // Notifications interface of AVS.
  _notificationsCapabilityAgent = capabilityAgents::notifications::NotificationsCapabilityAgent::create(
    notificationsStorage,
    capabilityAgents::notifications::NotificationRenderer::create(notificationsMediaPlayer),
    contextManager,
    _exceptionSender,
    audioFactory->notifications(),
    customerDataManager );
  
  if( !_notificationsCapabilityAgent ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateNotificationsCapabilityAgent"));
    return false;
  }

  // VIC-11806: notifications cleared when victor was turned off never recieve the SetIndicator(OFF) callback so
  //            the notification light remains on when you boot up.  Clearing all saved notification data at startup,
  //            since amazon re-sends them upon connection
  // note: this seems really wrong and is a hack to pass cert :(
  _notificationsCapabilityAgent->clearData();
  
  // The Interaction Model Capability Agent provides a way for AVS cloud initiated actions to be executed by the client.
  _interactionCapabilityAgent
    = capabilityAgents::interactionModel::InteractionModelCapabilityAgent::create( _directiveSequencer, _exceptionSender );
  if( !_interactionCapabilityAgent ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateInteractionModelCapabilityAgent"));
    return false;
  }
  
  std::shared_ptr<capabilityAgents::settings::SettingsUpdatedEventSender> settingsUpdatedEventSender
    = capabilityAgents::settings::SettingsUpdatedEventSender::create( _certifiedSender );
  if( !settingsUpdatedEventSender ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSettingsObserver"));
    return false;
  }
  
  // Creating the Setting object - This component implements the Setting interface of AVS.
  _settings = capabilityAgents::settings::Settings::create( settingsStorage, {settingsUpdatedEventSender}, customerDataManager );
  if( !_settings ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSettingsObject"));
    return false;
  }
  
  // Creating the Endpoint Handler - This component is responsible for handling directives from AVS instructing the
  // client to change the endpoint to connect to.
  auto endpointHandler = capabilityAgents::system::EndpointHandler::create( _connectionManager, _exceptionSender );
  if( !endpointHandler ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateEndpointHandler"));
    return false;
  }
  
  // Creating the SystemCapabilityProvider - This component is responsible for publishing information about the System
  // capability agent.
  auto systemCapabilityProvider = capabilityAgents::system::SystemCapabilityProvider::create();
  if( !systemCapabilityProvider ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSystemCapabilityProvider"));
    return false;
  }
  
  // Creating the RevokeAuthorizationHandler - This component is responsible for handling RevokeAuthorization
  // directives from AVS to notify the client to clear out authorization and re-enter the registration flow.
  _revokeAuthorizationHandler = AlexaRevokeAuthHandler::create( _exceptionSender );
  if( !_revokeAuthorizationHandler ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateRevokeAuthorizationHandler"));
    return false;
  }
  
  if( avsCommon::sdkInterfaces::softwareInfo::isValidFirmwareVersion(firmwareVersion) ) {
    const bool sendSoftwareInfoOnConnected = true;
    _softwareInfoSender = capabilityAgents::system::SoftwareInfoSender::create( firmwareVersion,
                                                                                sendSoftwareInfoOnConnected,
                                                                                nullptr, // softwareInfoSenderObserver
                                                                                _connectionManager,
                                                                                _connectionManager,
                                                                                _exceptionSender );
    if( !_softwareInfoSender ) {
      ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSoftwareInfoSender"));
      return false;
    }
  } else {
    LOG_WARNING("AlexaClient.Init.InvalidFirmwareVersion", "SDK rejected firmware version %d", firmwareVersion);
    return false;
  }


#if ANKI_DEV_CHEATS
  if( kDEV_ONLY_EnableAlexaTemplateRendererStub ) {
    _templateRuntime = std::make_shared<AlexaTemplateRuntimeStub>( _exceptionSender );
  }
#endif
  
  // Creating the DoNotDisturb Capability Agent
  //_dndCapabilityAgent
  //  = capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent::create( customerDataManager,
  //                                                                         _exceptionSender,
  //                                                                         _connectionManager,
  //                                                                         _deviceSettingsManager,
  //                                                                         _deviceSettingStorage );
  //if( !_dndCapabilityAgent ) {
  //  ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDNDCapabilityAgent"));
  //  return false;
  //}
  //AddConnectionObserver( _dndCapabilityAgent );
  
  
  // Register directives
  
  // This creates a directive wrapper that lets us snoop on directives json sent to capabilities
  // The nameSpace is the namespace constant defined at the top of each capabilities' cpp file.
  // TODO: expose this through a change to the SDK.
  #define MAKE_WRAPPER(nameSpace, handler) std::make_shared<AlexaCapabilityWrapper>(nameSpace, handler, _exceptionSender, _onDirectiveFunc)
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("SpeechSynthesizer", _speechSynthesizer) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeechSynthesizer"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("SpeechRecognizer", _audioInputProcessor) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AudioInputProcessor"));
    return false;
  }
  
  // Note: I've never actually seen this one receive the "RevokeAuthorization" directive we need
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("System", _revokeAuthorizationHandler) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "RevokeAuthorizationHandler"));
      return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("System", _userInactivityMonitor) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "UserInactivityMonitor"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Alerts", _alertsCapabilityAgent) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AlertsCapabilityAgent"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("System", endpointHandler) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "EndpointHandler"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("AudioPlayer", _audioPlayer) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AudioPlayer"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Speaker", _speakerManager) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeakerManager"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Notifications", _notificationsCapabilityAgent) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "NotificationsCapabilityAgent"));
    return false;
  }
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("InteractionModel", _interactionCapabilityAgent) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "InteractionModelCapabilityAgent"));
    return false;
  }

#if ANKI_DEV_CHEATS
  if( kDEV_ONLY_EnableAlexaTemplateRendererStub ) {
    if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("TemplateRuntime", _templateRuntime) ) ) {
      ACSDK_ERROR(LX("initializeFailed")
                  .d("reason", "unableToRegisterDirectiveHandler")
                  .d("directiveHandler", "TemplateRuntime"));
      return false;
    }
  }
#endif
  
  //if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Alexa.DoNotDisturb", _dndCapabilityAgent) ) ) {
  //  ACSDK_ERROR(LX("initializeFailed")
  //              .d("reason", "unableToRegisterDirectiveHandler")
  //              .d("directiveHandler", "DND"));
  //  return false;
  //}
  
  // Register capabilities
  
  if( !capabilitiesDelegate->registerCapability(_alertsCapabilityAgent) ) {
    ACSDK_ERROR(
                LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "Alerts"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_audioActivityTracker) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "AudioActivityTracker"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_audioInputProcessor) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechRecognizer"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_speechSynthesizer) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechSynthesizer"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_audioPlayer) ) {
    ACSDK_ERROR(
                LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "AudioPlayer"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_notificationsCapabilityAgent) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "Notifications"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_playbackController) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "PlaybackController"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_settings) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "Settings"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_speakerManager) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "Speaker"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(_interactionCapabilityAgent) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "Interaction"));
    return false;
  }
  
  if( !capabilitiesDelegate->registerCapability(systemCapabilityProvider) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "System"));
    return false;
  }

#if ANKI_DEV_CHEATS
  if( kDEV_ONLY_EnableAlexaTemplateRendererStub ) {
    if (!(capabilitiesDelegate->registerCapability(_templateRuntime))) {
      ACSDK_ERROR(LX("initializeFailed")
                  .d("reason", "unableToRegisterCapability")
                  .d("capabilitiesDelegate", "TemplateRuntime"));
      return false;
    }
  }
  
  //if( !capabilitiesDelegate->registerCapability(_dndCapabilityAgent) ) {
  //  ACSDK_ERROR(LX("initializeFailed")
  //              .d("reason", "unableToRegisterCapability")
  //              .d("capabilitiesDelegate", "DoNotDisturb"));
  //  return false;
  //}
  
  // create console func(s)
  auto sendDirective = [&](ConsoleFunctionContextRef context ) {
    const char* unparsedDirective = ConsoleArg_Get_String( context, "directive" );
    if( _directiveSequencer ) {
      // if whatever responds to the directive needs to parse an attachment, that will be undefined behavior.
      // for other things like alerts, this should work. Set LogAlexaDirectives to true, note the
      // full directive printed to logs, and then modify it as needed and re-send it here
      const std::string attachmentContextId;
      auto directive = avsCommon::avs::AVSDirective::create( unparsedDirective, attachmentManager, attachmentContextId );
      _directiveSequencer->onDirective( std::move(directive.first) );
    }
  };
  sConsoleFuncs.emplace_front( "SendAlexaDirective", std::move(sendDirective), "Alexa", "const char* directive" );
  
#endif
  
#if ANKI_DEV_CHEATS
  #define ADD_TO_SHUTDOWN_CHECKER(x) _shutdownChecker.AddObject(#x, x)
  ADD_TO_SHUTDOWN_CHECKER( _directiveSequencer );
  ADD_TO_SHUTDOWN_CHECKER( _audioFocusManager );
  ADD_TO_SHUTDOWN_CHECKER( _audioActivityTracker );
  ADD_TO_SHUTDOWN_CHECKER( _messageRouter );
  ADD_TO_SHUTDOWN_CHECKER( _connectionManager );
  ADD_TO_SHUTDOWN_CHECKER( _internetConnectionMonitor );
  ADD_TO_SHUTDOWN_CHECKER( _exceptionSender );
  ADD_TO_SHUTDOWN_CHECKER( _certifiedSender );
  ADD_TO_SHUTDOWN_CHECKER( _audioInputProcessor );
  ADD_TO_SHUTDOWN_CHECKER( _speechSynthesizer );
  ADD_TO_SHUTDOWN_CHECKER( _audioPlayer );
  ADD_TO_SHUTDOWN_CHECKER( _alertsCapabilityAgent );
  ADD_TO_SHUTDOWN_CHECKER( _interactionCapabilityAgent );
  ADD_TO_SHUTDOWN_CHECKER( _notificationsCapabilityAgent );
  ADD_TO_SHUTDOWN_CHECKER( _userInactivityMonitor );
  ADD_TO_SHUTDOWN_CHECKER( _dialogUXStateAggregator );
  ADD_TO_SHUTDOWN_CHECKER( _playbackRouter );
  ADD_TO_SHUTDOWN_CHECKER( _playbackController );
  ADD_TO_SHUTDOWN_CHECKER( _speakerManager );
  ADD_TO_SHUTDOWN_CHECKER( _softwareInfoSender );
  ADD_TO_SHUTDOWN_CHECKER( _revokeAuthorizationHandler );
  ADD_TO_SHUTDOWN_CHECKER( _registrationManager );
#endif
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::Connect( const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate )
{
  // try connecting
  if( capabilitiesDelegate != nullptr ) {
    capabilitiesDelegate->publishCapabilitiesAsyncWithRetries();
  }
  else {
    LOG_ERROR("AlexaClient.Connect.NoCapabilities", "no capabilities delegate, can't register capabilities");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::ChangeSetting( const std::string& key, const std::string& value )
{
  if( _settings != nullptr ) {
     _settings->changeSetting( key, value );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::SendDefaultSettings()
{
  if( _settings != nullptr ) {
    _settings->sendDefaultSettings();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::future<bool> AlexaClient::NotifyOfTapToTalk( capabilityAgents::aip::AudioProvider& tapToTalkAudioProvider,
                                                  avsCommon::avs::AudioInputStream::Index beginIndex,
                                                  std::chrono::steady_clock::time_point startOfSpeechTimestamp )
{
  return _audioInputProcessor->recognize(tapToTalkAudioProvider, capabilityAgents::aip::Initiator::TAP, startOfSpeechTimestamp, beginIndex);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::future<bool> AlexaClient::NotifyOfTapToTalkEnd()
{
  return _audioInputProcessor->stopCapture();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::future<bool> AlexaClient::NotifyOfWakeWord( capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
                                                 avsCommon::avs::AudioInputStream::Index beginIndex,
                                                 avsCommon::avs::AudioInputStream::Index endIndex,
                                                 const std::string& keyword,
                                                 std::chrono::steady_clock::time_point startOfSpeechTimestamp,
                                                 const capabilityAgents::aip::ESPData espData,
                                                 std::shared_ptr<const std::vector<char>> KWDMetadata )
{

  LOG_INFO("AlexaClient.NotifyOfWakeWord", "keyword='%s', beginIDx=%llu, endIdx=%llu, connected=%d",
           keyword.c_str(),
           beginIndex,
           endIndex,
           _connectionManager->isConnected());


  if( !_connectionManager->isConnected() ) {
    std::promise<bool> ret;
    static const std::string ALEXA_STOP_KEYWORD = "STOP";
    if( ALEXA_STOP_KEYWORD == keyword ) {
      // Alexa Stop uttered while offline
      StopForegroundActivity();
      // Returning as interaction handled
      ret.set_value(true);
      return ret.get_future();
    } else {
      // Ignore Alexa wake word while disconnected
      // Returning as interaction not handled
      ret.set_value(false);
      return ret.get_future();
    }
  }

  return _audioInputProcessor->recognize( wakeWordAudioProvider,
                                          capabilityAgents::aip::Initiator::WAKEWORD,
                                          startOfSpeechTimestamp,
                                          beginIndex,
                                          endIndex,
                                          keyword,
                                          espData,
                                          KWDMetadata );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddConnectionObserver( std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer)
{
  if( ANKI_VERIFY(_connectionManager != nullptr, "AlexaClient.AddConnectionObserver.Null", "Connection manager is null") ) {
    _connectionManager->addConnectionStatusObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::RemoveConnectionObserver( std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer )
{
  if( ANKI_VERIFY(_connectionManager != nullptr, "AlexaClient.RemoveConnectionObserver.Null", "Connection manager is null") ) {
    _connectionManager->removeConnectionStatusObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::onCapabilitiesStateChange( CapabilitiesObserverInterface::State newState,
                                             CapabilitiesObserverInterface::Error newError )
{
  if( CapabilitiesObserverInterface::State::SUCCESS == newState ) {
    if( ANKI_VERIFY(_connectionManager != nullptr,
                    "AlexaClient.onCapabilitiesStateChange.Null",
                    "Connection manager is null") )
    {
      _connectionManager->enable();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::StopForegroundActivity()
{
  if( ANKI_VERIFY(_audioFocusManager != nullptr,
                  "AlexaClient.StopForegroundActivity.Null",
                  "Audio focus manager is null") )
  {
    _audioFocusManager->stopForegroundActivity();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::StopAlerts()
{
  if( ANKI_VERIFY(_alertsCapabilityAgent != nullptr,
                  "AlexaClient.StopAlerts.Null",
                  "Alerts capability is null") )
  {
    _alertsCapabilityAgent->onLocalStop();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddSpeakerManagerObserver( std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer )
{
  if( ANKI_VERIFY(_speakerManager != nullptr,
                  "AlexaClient.AddSpeakerManagerObserver.Null",
                  "Speaker manager is null") )
  {
    _speakerManager->addSpeakerManagerObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddAlexaDialogStateObserver( std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer )
{
  if( ANKI_VERIFY(_dialogUXStateAggregator != nullptr,
                  "AlexaClient.AddAlexaDialogStateObserver.Null",
                  "Dialog state aggregator is null") )
  {
    _dialogUXStateAggregator->addObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddInternetConnectionObserver( std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer )
{
  if( ANKI_VERIFY(_internetConnectionMonitor != nullptr,
                  "AlexaClient.AddInternetConnectionObserver.Null",
                  "Internet monitor is null") )
  {
    _internetConnectionMonitor->addInternetConnectionObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddRevokeAuthorizationObserver( std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer )
{
  if( !_revokeAuthorizationHandler ) {
    ACSDK_ERROR(LX("addRevokeAuthorizationObserver").d("reason", "revokeAuthorizationNotSupported"));
    return;
  }
  _revokeAuthorizationHandler->addObserver( observer );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddNotificationsObserver( std::shared_ptr<avsCommon::sdkInterfaces::NotificationsObserverInterface> observer )
{
  if( ANKI_VERIFY(_notificationsCapabilityAgent != nullptr,
                  "AlexaClient.AddNotificationsObserver.Null",
                  "Notifications agent is null") )
  {
    _notificationsCapabilityAgent->addObserver( observer );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddAlertObserver( std::shared_ptr<capabilityAgents::alerts::AlertObserverInterface> observer )
{
  if( ANKI_VERIFY(_alertsCapabilityAgent != nullptr,
                  "AlexaClient.AddAlertObserver.Null",
                  "Alerts agent is null") )
  {
    _alertsCapabilityAgent->addObserver( observer );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddAudioPlayerObserver( std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AudioPlayerObserverInterface> observer )
{
  if( ANKI_VERIFY( _audioPlayer != nullptr,
                   "AlexaClient.AddAudioPlayerObserver.Null",
                   "audio player is null") )
  {
    _audioPlayer->addObserver( observer );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::shared_ptr<registrationManager::RegistrationManager> AlexaClient::GetRegistrationManager()
{
  return _registrationManager;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::shared_ptr<settings::DeviceSettingsManager> AlexaClient::GetSettingsManager()
{
  return _deviceSettingsManager;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaClient::IsAVSConnected() const
{
  return _connectionManager && _connectionManager->isConnected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::ReinitializeAllTimers()
{
  if( _alertsCapabilityAgent ) {
    _alertsCapabilityAgent->refreshTimers();
  }
}

  
} // namespace Vector
} // namespace Anki
