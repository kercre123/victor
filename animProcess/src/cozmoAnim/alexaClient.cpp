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
#include "cozmoAnim/alexaSpeechSynthesizer.h"
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
                                                   std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
                                                   alexaDialogStateObservers,
                                                   std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
                                                   connectionObservers,
                                                   std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate)
{
  std::unique_ptr<AlexaClient> client(new AlexaClient());
  if (!client->Init(deviceInfo,
                    customerDataManager,
                    authDelegate,
                    messageStorage,
                    alexaDialogStateObservers,
                    connectionObservers,
                    capabilitiesDelegate))
  {
    return nullptr;
  }
  
  return client;
}
  
bool AlexaClient::Init(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo,
                         std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
                         std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                         std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage,
                         std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
                         alexaDialogStateObservers,
                         std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
                         connectionObservers,
                         std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate)
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
  // m_audioActivityTracker
  // m_audioFocusManager
  // m_userInactivityMonitor
  // m_audioInputProcessor

  
    /*
     * Creating the Speech Synthesizer - This component is the Capability Agent that implements the SpeechSynthesizer
     * interface of AVS.
     */
  m_speechSynthesizer = capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer::create(m_exceptionSender);
//    auto m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create(
//                                                                                         speakMediaPlayer,
//                                                                                         m_connectionManager,
//                                                                                         m_audioFocusManager,
//                                                                                         contextManager,
//                                                                                         m_exceptionSender,
//                                                                                         m_dialogUXStateAggregator);
  if (!m_speechSynthesizer) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
    return false;
  }
  //m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);

  
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
  
  if (!m_directiveSequencer->addDirectiveHandler(m_speechSynthesizer)) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterDirectiveHandler")
                .d("directiveHandler", "SpeechSynthesizer"));
    return false;
  }
  
  if (!(capabilitiesDelegate->registerCapability(m_speechSynthesizer))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechSynthesizer"));
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
  
} // namespace Vector
} // namespace Anki
