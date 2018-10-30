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
#include "cozmoAnim/alexa/alexaObserver.h"
#include "cozmoAnim/alexa/alexaCapabilityWrapper.h"

// temp:
#include "cozmoAnim/alexa/stubSpeechSynth.h"

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
#include <Settings/SQLiteSettingStorage.h>
#include <DefaultClient/DefaultClient.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>

#include <memory>
#include <vector>

namespace Anki {
namespace Vector {
  
using namespace alexaClientSDK;
  
namespace {
  #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<AlexaClient>
  AlexaClient::Create(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                      std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                      std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                      std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                      std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                      std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                      std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
                      std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
                      std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                      std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                      std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker )
{
  std::unique_ptr<AlexaClient> client( new AlexaClient{} );
  if( !client->Init( deviceInfo,
                     customerDataManager,
                     authDelegate,
                     messageStorage,
                     alertStorage,
                     audioFactory,
                     alexaDialogStateObservers,
                     connectionObservers,
                     capabilitiesDelegate,
                     ttsMediaPlayer,
                     alertsMediaPlayer,
                     audioMediaPlayer,
                     ttsSpeaker,
                     alertsSpeaker,
                     audioSpeaker ) )
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
  if( _userInactivityMonitor ) {
    _userInactivityMonitor->shutdown();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaClient::Init( std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                        std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                        std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                        std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> alexaDialogStateObservers,
                        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> connectionObservers,
                        std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                        std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker )
{
  
  _dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
  
  for( auto observer : alexaDialogStateObservers ) {
    _dialogUXStateAggregator->addObserver( observer );
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
  
  // Create a factory for creating objects that handle tasks that need to be performed right after establishing
  // a connection to AVS.
  _postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create( contextManager );
  
  // Create a factory to create objects that establish a connection with AVS.
  auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>( _postConnectSynchronizerFactory );
  
  // Creating the message router - This component actually maintains the connection to AVS over HTTP2. It is created
  // using the auth delegate, which provides authorization to connect to AVS, and the attachment manager, which helps
  // ACL write attachments received from AVS.
  _messageRouter = std::make_shared<acl::MessageRouter>( authDelegate, attachmentManager, transportFactory );
  
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
  
  // Creating the Audio Input Processor - This component is the Capability Agent that implments the SpeechRecognizer
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

  // temp stub for speech synthesis so that alexa will authorize
  _speechSynthesizer = capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer::create(_exceptionSender);
  if( !_speechSynthesizer ) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
    return false;
  }
  
  AddConnectionObserver( _dialogUXStateAggregator );
  
  // Register directives
  
  // This creates a directive wrapper that lets us snoop on directives json sent to capabilities
  #define MAKE_WRAPPER(nameSpace, handler) std::make_shared<AlexaCapabilityWrapper>(nameSpace, handler, _exceptionSender, _onDirectiveFunc)
  
  if( !_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("SpeechSynthesizer", _speechSynthesizer) ) ) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeechSynthesizer"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(_speechSynthesizer))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechSynthesizer"));
    return false;
  }
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::Connect( const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate )
{
  // try connecting
  if( capabilitiesDelegate != nullptr ) {
    capabilitiesDelegate->publishCapabilitiesAsyncWithRetries();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaClient::AddConnectionObserver( std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer)
{
  if( ANKI_VERIFY(_connectionManager != nullptr, "AlexaClient.AddConnectionObserver.Null", "Connection manager is null") ) {
    _connectionManager->addConnectionStatusObserver( observer );
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
  
} // namespace Vector
} // namespace Anki
