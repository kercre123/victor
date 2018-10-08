/*
 * File:          cozmoAnim/animEngine.cpp
 * Date:          6/26/2017
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Animation Process.
 *
 * Author: Kevin Yoon
 *
 * Modifications:
 */

#include "cozmoAnim/alexaClient.h"
#include "cozmoAnim/alexaLogger.h"
#include "cozmoAnim/alexaTemplateRuntimeStub.h"
#include "cozmoAnim/alexaCapabilityWrapper.h"
//#include "cozmoAnim/alexaSpeechSynthesizer.h"
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
#include <iostream>

namespace Anki {
namespace Vector {
  
  using namespace alexaClientSDK;
  
namespace {
  #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)
}
  
  
  
std::shared_ptr<AlexaClient> AlexaClient::create(
                                                   std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                                                   std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                                                   std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                                                   std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                                                   std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
                                                   alexaDialogStateObservers,
                                                   std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
                                                   connectionObservers,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                                                   std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                                                   std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                                                   std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker)
{
  std::unique_ptr<AlexaClient> client(new AlexaClient());
  if (!client->Init(deviceInfo,
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
                    audioSpeaker))
  {
    return nullptr;
  }
  
  return client;
}
  
bool AlexaClient::Init(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                         std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                         std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                         std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                         std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
                         std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
                         std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
                         alexaDialogStateObservers,
                         std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
                         connectionObservers,
                         std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
                         std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
                         std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
                         std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
                         std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
                         std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
                         std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker)
{
  
  m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
  
  for (auto observer : alexaDialogStateObservers) {
    m_dialogUXStateAggregator->addObserver(observer);
  }
  
  /*
   * Creating the Attachment Manager - This component deals with managing attachments and allows for readers and
   * writers to be created to handle the attachment.
   */
  auto attachmentManager = std::make_shared<avsCommon::avs::attachment::AttachmentManager>(
                                                                                           avsCommon::avs::attachment::AttachmentManager::AttachmentType::IN_PROCESS);
  
  /*
   * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
   * It is required for each of the capability agents so that they may provide their state just before any event is
   * fired off.
   */
  auto contextManager = contextManager::ContextManager::create();
  if (!contextManager) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateContextManager"));
    return false;
  }
  
  /*
   * Create a factory for creating objects that handle tasks that need to be performed right after establishing
   * a connection to AVS.
   */
  m_postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create(contextManager);
  
  /*
   * Create a factory to create objects that establish a connection with AVS.
   */
  auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(m_postConnectSynchronizerFactory);
  
  /*
   * Creating the message router - This component actually maintains the connection to AVS over HTTP2. It is created
   * using the auth delegate, which provides authorization to connect to AVS, and the attachment manager, which helps
   * ACL write attachments received from AVS.
   */
  m_messageRouter = std::make_shared<acl::MessageRouter>(authDelegate, attachmentManager, transportFactory);
  
  /*
   * Creating the connection manager - This component is the overarching connection manager that glues together all
   * the other networking components into one easy-to-use component.
   */
  m_connectionManager = acl::AVSConnectionManager::create(m_messageRouter, false, connectionObservers, {m_dialogUXStateAggregator});
  if (!m_connectionManager) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionManager"));
    return false;
  }
  
//  auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
//  auto internetConnectionMonitor =
//  avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
//
//  if (!internetConnectionMonitor) {
//    ACSDK_CRITICAL(LX("initializeFailed").d("reason", "internetConnectionMonitor was nullptr"));
//    return;
//  }
  
  /*
   * Creating our certified sender - this component guarantees that messages given to it (expected to be JSON
   * formatted AVS Events) will be sent to AVS.  This nicely decouples strict message sending from components which
   * require an Event be sent, even in conditions when there is no active AVS connection.
   */
  m_certifiedSender = certifiedSender::CertifiedSender::create(
                                                               m_connectionManager, m_connectionManager, messageStorage, customerDataManager);
  if (!m_certifiedSender) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateCertifiedSender"));
    return false;
  }
  
  
  /*
   * Creating the Exception Sender - This component helps the SDK send exceptions when it is unable to handle a
   * directive sent by AVS. For that reason, the Directive Sequencer and each Capability Agent will need this
   * component.
   */
  m_exceptionSender = avsCommon::avs::ExceptionEncounteredSender::create(m_connectionManager);
  if (!m_exceptionSender) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExceptionSender"));
    return false;
  }
  
  /*
   * Creating the Directive Sequencer - This is the component that deals with the sequencing and ordering of
   * directives sent from AVS and forwarding them along to the appropriate Capability Agent that deals with
   * directives in that Namespace/Name.
   */
  m_directiveSequencer = adsl::DirectiveSequencer::create(m_exceptionSender);
  if (!m_directiveSequencer) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateDirectiveSequencer"));
    return false;
  }
  
  auto messageInterpreter = std::make_shared<adsl::MessageInterpreter>(m_exceptionSender, m_directiveSequencer, attachmentManager);
  m_connectionManager->addMessageObserver(messageInterpreter);
  
  // m_registrationManager
  
  /*
   * Creating the Audio Activity Tracker - This component is responsibly for reporting the audio channel focus
   * information to AVS.
   */
  m_audioActivityTracker = afml::AudioActivityTracker::create(contextManager);
  
  /*
   * Creating the Focus Manager - This component deals with the management of layered audio focus across various
   * components. It handles granting access to Channels as well as pushing different "Channels" to foreground,
   * background, or no focus based on which other Channels are active and the priorities of those Channels. Each
   * Capability Agent will require the Focus Manager in order to request access to the Channel it wishes to play on.
   */
  m_audioFocusManager =
    std::make_shared<afml::FocusManager>(afml::FocusManager::getDefaultAudioChannels(), m_audioActivityTracker);
  
  /*
   * Creating the User Inactivity Monitor - This component is responsibly for updating AVS of user inactivity as
   * described in the System Interface of AVS.
   */
  m_userInactivityMonitor =
    capabilityAgents::system::UserInactivityMonitor::create(m_connectionManager, m_exceptionSender);
  if (!m_userInactivityMonitor) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateUserInactivityMonitor"));
    return false;
  }
  
  /*
   * Creating the Audio Input Processor - This component is the Capability Agent that implments the SpeechRecognizer
   * interface of AVS.
   */
  m_audioInputProcessor = capabilityAgents::aip::AudioInputProcessor::create(
                                                                             m_directiveSequencer,
                                                                             m_connectionManager,
                                                                             contextManager,
                                                                             m_audioFocusManager,
                                                                             m_dialogUXStateAggregator,
                                                                             m_exceptionSender,
                                                                             m_userInactivityMonitor);
  if (!m_audioInputProcessor) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioInputProcessor"));
    return false;
  }
  
  m_audioInputProcessor->addObserver(m_dialogUXStateAggregator);

  
    /*
     * Creating the Speech Synthesizer - This component is the Capability Agent that implements the SpeechSynthesizer
     * interface of AVS.
     */
//  m_speechSynthesizer = capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer::create(m_exceptionSender);
    m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create(
                                                                                         ttsMediaPlayer,
                                                                                         m_connectionManager,
                                                                                         m_audioFocusManager,
                                                                                         contextManager,
                                                                                         m_exceptionSender,
                                                                                         m_dialogUXStateAggregator);
  if (!m_speechSynthesizer) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
    return false;
  }
  m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);
  
  
  
  /*
   * Creating the PlaybackController Capability Agent - This component is the Capability Agent that implements the
   * PlaybackController interface of AVS.
   */
  m_playbackController = capabilityAgents::playbackController::PlaybackController::create(contextManager, m_connectionManager);
  if (!m_playbackController) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackController"));
    return false;
  }
  
  /*
   * Creating the PlaybackRouter - This component routes a playback button press to the active handler.
   * The default handler is @c PlaybackController.
   */
  m_playbackRouter = capabilityAgents::playbackController::PlaybackRouter::create(m_playbackController);
  if (!m_playbackRouter) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreatePlaybackRouter"));
    return false;
  }
  
  /*
   * Creating the Audio Player - This component is the Capability Agent that implements the AudioPlayer
   * interface of AVS.
   */
  m_audioPlayer = capabilityAgents::audioPlayer::AudioPlayer::create(
                                                                     audioMediaPlayer,
                                                                     m_connectionManager,
                                                                     m_audioFocusManager,
                                                                     contextManager,
                                                                     m_exceptionSender,
                                                                     m_playbackRouter);
  if (!m_audioPlayer) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAudioPlayer"));
    return false;
  }
  
  
  
  std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> allSpeakers = {
    ttsSpeaker,
    alertsSpeaker,
    audioSpeaker,
    // notificationsSpeaker, bluetoothSpeaker, ringtoneSpeaker
  };
  //allSpeakers.insert(allSpeakers.end(), additionalSpeakers.begin(), additionalSpeakers.end());
  
  /*
   * Creating the SpeakerManager Capability Agent - This component is the Capability Agent that implements the
   * Speaker interface of AVS.
   */
  m_speakerManager = capabilityAgents::speakerManager::SpeakerManager::create(
                                                                              allSpeakers, contextManager, m_connectionManager, m_exceptionSender);
  if (!m_speakerManager) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeakerManager"));
    return false;
  }
  
  
  /*
   * Creating the Alerts Capability Agent - This component is the Capability Agent that implements the Alerts
   * interface of AVS.
   */
  m_alertsCapabilityAgent = capabilityAgents::alerts::AlertsCapabilityAgent::create(
                                                                                    m_connectionManager,
                                                                                    m_connectionManager,
                                                                                    m_certifiedSender,
                                                                                    m_audioFocusManager,
                                                                                    m_speakerManager,
                                                                                    contextManager,
                                                                                    m_exceptionSender,
                                                                                    alertStorage,
                                                                                    audioFactory->alerts(),
                                                                                    capabilityAgents::alerts::renderer::Renderer::create(alertsMediaPlayer),
                                                                                    customerDataManager);
  if (!m_alertsCapabilityAgent) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateAlertsCapabilityAgent"));
    return false;
  }
  
  addConnectionObserver(m_dialogUXStateAggregator);
  
//  auto xx = std::make_shared<AlexaDevDirectiveHandler>("AlexaDevDirectiveHandler", m_exceptionSender);
//  m_devDirectiveHandler = xx;
  
  m_templateRuntime = std::make_shared<AlexaTemplateRuntimeStub>( m_exceptionSender );
  
  // m_playbackRouter
  // m_audioPlayer
  // audio focus manager

  
  // m_playbackController
  // m_playbackRouter
  // m_audioPlayer
  // allSpeakers
  // m_speakerManager
  // m_alertsCapabilityAgent
  // m_notificationsCapabilityAgent
  // m_interactionCapabilityAgent
  // m_settings
  // m_externalMediaPlayer
  // endpointHandler
  // systemCapabilityProvider
  
  
  #define MAKE_WRAPPER(nameSpace, handler) std::make_shared<AlexaCapabilityWrapper>(nameSpace, handler, m_exceptionSender, _onDirectiveFunc)
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("SpeechSynthesizer", m_speechSynthesizer) )) {
    ACSDK_ERROR(LX("initializeFailed") 
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeechSynthesizer"));
    return false;
  }
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("SpeechRecognizer", m_audioInputProcessor) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AudioInputProcessor"));
    return false;
  }
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("System", m_userInactivityMonitor) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "UserInactivityMonitor"));
    return false;
  }
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Alerts", m_alertsCapabilityAgent) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AlertsCapabilityAgent"));
    return false;
  }
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("AudioPlayer", m_audioPlayer) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "AudioPlayer"));
    return false;
  }
  
      // todo: use constant in SpeakerManagerConstants.h
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("Speaker", m_speakerManager) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeakerManager"));
    return false;
  }
  
  if (!m_directiveSequencer->addDirectiveHandler( MAKE_WRAPPER("TemplateRuntime", m_templateRuntime) )) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "TemplateRuntime"));
    return false;
  }
  
//  if (!m_directiveSequencer->addDirectiveHandler(m_devDirectiveHandler)) {
//    ACSDK_ERROR(LX("initializeFailed")
//                .d("reason", "unableToRegisterDirectiveHandler")
//                .d("directiveHandler", "AlexaDevDirectiveHandler"));
//    return false;
//  }
  
  
  
//  X synth
//  X m_audioInputProcessor
//  m_userInactivityMonitor
//  X aleerts
//  X  audioPLayer
//  X sperakerMaabger
//  X m_playbackController
//
//extra:
//  m_audioActivityTracker
  
  if (!(capabilitiesDelegate->registerCapability(m_alertsCapabilityAgent))) {
    ACSDK_ERROR(
                LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "Alerts"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_audioActivityTracker))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "AudioActivityTracker"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_audioInputProcessor))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechRecognizer"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_speechSynthesizer))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechSynthesizer"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_audioPlayer))) {
    ACSDK_ERROR(
                LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "AudioPlayer"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_playbackController))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "PlaybackController"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_speakerManager))) {
    ACSDK_ERROR(
                LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "Speaker"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_templateRuntime))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "TemplateRuntime"));
    return false;
  }
  
  
  return true;
}
  
void AlexaClient::Connect(const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate)
{
  // try connecting
  capabilitiesDelegate->publishCapabilitiesAsyncWithRetries();
  
  PRINT_NAMED_WARNING("WHATNOW", "worked");
}
  
std::future<bool> AlexaClient::notifyOfTapToTalk(
                                                   capabilityAgents::aip::AudioProvider& tapToTalkAudioProvider,
                                                   avsCommon::avs::AudioInputStream::Index beginIndex) {
  return m_audioInputProcessor->recognize(tapToTalkAudioProvider, capabilityAgents::aip::Initiator::TAP, beginIndex);
}
  
  
std::future<bool> AlexaClient::notifyOfWakeWord(
    capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::string keyword,
    const capabilityAgents::aip::ESPData espData,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (!m_connectionManager->isConnected()) {
        std::promise<bool> ret;
        static const std::string ALEXA_STOP_KEYWORD = "STOP";
        if (ALEXA_STOP_KEYWORD == keyword) {
            // Alexa Stop uttered while offline
            ACSDK_INFO(LX("notifyOfWakeWord").d("action", "localStop").d("reason", "stopUtteredWhileNotConnected"));
            stopForegroundActivity();

            // Returning as interaction handled
            ret.set_value(true);
            return ret.get_future();
        } else {
            // Ignore Alexa wake word while disconnected
            ACSDK_INFO(LX("notifyOfWakeWord").d("action", "ignoreAlexaWakeWord").d("reason", "networkDisconnected"));

            // Returning as interaction not handled
            ret.set_value(false);
            return ret.get_future();
        }
    }

    PRINT_NAMED_WARNING("WHATNOW", "calling wake word recognizer!");
    return m_audioInputProcessor->recognize(
        wakeWordAudioProvider,
        capabilityAgents::aip::Initiator::WAKEWORD,
        beginIndex,
        endIndex,
        keyword,
        espData,
        KWDMetadata);
}
  
void AlexaClient::addConnectionObserver(
                                          std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
  m_connectionManager->addConnectionStatusObserver(observer);
}
  
void AlexaClient::onCapabilitiesStateChange( CapabilitiesObserverInterface::State newState,
                                             CapabilitiesObserverInterface::Error newError)
{
  if (CapabilitiesObserverInterface::State::SUCCESS == newState) {
      m_connectionManager->enable();
  }
}

void AlexaClient::stopForegroundActivity() {
  m_audioFocusManager->stopForegroundActivity();
}
  
void AlexaClient::addSpeakerManagerObserver( std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer)
{
  m_speakerManager->addSpeakerManagerObserver(observer);
}
  
  void AlexaClient::addAlexaDialogStateObserver( std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer)
  {
    m_dialogUXStateAggregator->addObserver(observer);
  }
  
} // namespace Vector
} // namespace Anki
