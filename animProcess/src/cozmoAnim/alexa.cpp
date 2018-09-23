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

#include "cozmoAnim/alexa.h"
#include "cozmoAnim/alexaLogger.h"
#include "cozmoAnim/alexaSpeechSynthesizer.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include <ACL/Transport/HTTP2TransportFactory.h>
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
  
  namespace {
      const std::string kConfig = R"4Nk1({
      "cblAuthDelegate":{
        // Path to CBLAuthDelegate's database file. e.g. /home/ubuntu/Build/cblAuthDelegate.db
        // Note: The directory specified must be valid.
        // The database file (cblAuthDelegate.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for CBLAuthDelegate (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/cblAuthDelegate.db"
      },
      "deviceInfo":{
        // Unique device serial number. e.g. 123456
        "deviceSerialNumber":"123457",
        // The Client ID of the Product from developer.amazon.com
        "clientId": "amzn1.application-oa2-client.35a58ee8f3444563aed328cb189da216",
        // Product ID from developer.amazon.com
        "productId": "test_product_1"
      },
      "capabilitiesDelegate":{
        // The endpoint to connect in order to send device capabilities.
        // This will only be used in DEBUG builds.
        // e.g. "endpoint": "https://api.amazonalexa.com"
        // Override the message to be sent out to the Capabilities API.
        // This will only be used in DEBUG builds.
        // e.g. "overridenCapabilitiesPublishMessageBody": {
        //          "envelopeVersion":"20160207",
        //          "capabilities":[
        //              {
        //                "type":"AlexaInterface",
        //                "interface":"Alerts",
        //                "version":"1.1"
        //              }
        //          ]
        //      }
      },
      "miscDatabase":{
        // Path to misc database file. e.g. /home/ubuntu/Build/miscDatabase.db
        // Note: The directory specified must be valid.
        // The database file (miscDatabase.db) will be created by SampleApp, do not create it yourself.
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/miscDatabase.db"
      },
      "alertsCapabilityAgent":{
        // Path to Alerts database file. e.g. /home/ubuntu/Build/alerts.db
        // Note: The directory specified must be valid.
        // The database file (alerts.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for alerts (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/alerts.db"
      },
      "settings":{
        // Path to Settings database file. e.g. /home/ubuntu/Build/settings.db
        // Note: The directory specified must be valid.
        // The database file (settings.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for settings (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/settings.db",
        "defaultAVSClientSettings":{
          // Default language for Alexa.
          // See https://developer.amazon.com/docs/alexa-voice-service/settings.html#settingsupdated for valid values.
          "locale":"en-US"
        }
      },
      "bluetooth" : {
        // Path to Bluetooth database file. e.g. /home/ubuntu/Build/bluetooth.db
        // Note: The directory specified must be valid.
        // The database file (bluetooth.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for bluetooth (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/bluetooth.db"
      },
      "certifiedSender":{
        // Path to Certified Sender database file. e.g. /home/ubuntu/Build/certifiedsender.db
        // Note: The directory specified must be valid.
        // The database file (certifiedsender.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for certifiedSender (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/certifiedsender.db"
      },
      "notifications":{
        // Path to Notifications database file. e.g. /home/ubuntu/Build/notifications.db
        // Note: The directory specified must be valid.
        // The database file (notifications.db) will be created by SampleApp, do not create it yourself.
        // The database file should only be used for notifications (don't use it for other components of SDK)
        "databaseFilePath":"/data/data/com.anki.victor/persistent/alexa/notifications.db"
      },
      "sampleApp":{
        // To specify if the SampleApp supports display cards.
        "displayCardsSupported":true
        // The firmware version of the device to send in SoftwareInfo event.
        // Note: The firmware version should be a positive 32-bit integer in the range [1-2147483647].
        // e.g. "firmwareVersion": 123
        // The default endpoint to connect to.
        // See https://developer.amazon.com/docs/alexa-voice-service/api-overview.html#endpoints for regions and values
        // e.g. "endpoint": "https://avs-alexa-na.amazon.com"
        
        // Example of specifying suggested latency in seconds when openning PortAudio stream. By default,
        // when this paramater isn't specified, SampleApp calls Pa_OpenDefaultStream to use the default value.
        // See http://portaudio.com/docs/v19-doxydocs/structPaStreamParameters.html for further explanation
        // on this parameter.
        //"portAudio":{
        //    "suggestedLatency": 0.150
        //}
      },
      
      // Example of specifying output format and the audioSink for the gstreamer-based MediaPlayer bundled with the SDK.
      // Many platforms will automatically set the output format correctly, but in some cases where the hardware requires
      // a specific format and the software stack is not automatically setting it correctly, these parameters can be used
      // to manually specify the output format.  Supported rate/format/channels values are documented in detail here:
      // https://gstreamer.freedesktop.org/documentation/design/mediatype-audio-raw.html
      //
      // By default the "autoaudiosink" element is used in the pipeline.  This element automatically selects the best sink
      // to use based on the configuration in the system.  But sometimes the wrong sink is selected and that prevented sound
      // from being played.  A new configuration is added where the audio sink can be specified for their system.
      // "gstreamerMediaPlayer":{
      //     "outputConversion":{
      //         "rate":16000,
      //         "format":"S16LE",
      //         "channels":1
      //     },
      //     "audioSink":"autoaudiosink"
      // },
      
      // Example of specifiying curl options that is different from the default values used by libcurl.
      // "libcurlUtils":{
      //
      //     By default libcurl is built with paths to a CA bundle and a directory containing CA certificates. You can
      //     direct the AVS Device SDK to configure libcurl to use an additional path to directories containing CA
      //     certificates via the CURLOPT_CAPATH setting.  Additional details of this curl option can be found in:
      //     https://curl.haxx.se/libcurl/c/CURLOPT_CAPATH.html
      //     "CURLOPT_CAPATH":"INSERT_YOUR_CA_CERTIFICATE_PATH_HERE",
      //
      //     You can specify the AVS Device SDK to use a specific outgoing network interface.  More information of
      //     this curl option can be found here:
      //     https://curl.haxx.se/libcurl/c/CURLOPT_INTERFACE.html
      //     "CURLOPT_INTERFACE":"INSERT_YOUR_INTERFACE_HERE"
      // },
      
      // Example of specifying a default log level for all ModuleLoggers.  If not specified, ModuleLoggers get
      // their log level from the sink logger.
       "logging":{
           "logLevel":"DEBUG9"
       }
    
      // Example of overriding a specific ModuleLogger's log level whether it was specified by the default value
      // provided by the logging.logLevel value (as in the above example) or the log level of the sink logger.
      // "acl":{
      //     "logLevel":"DEBUG9"
      // }
    }
    
    
    )4Nk1";

  }
  
void Alexa::Init()
{
  if( Util::FileUtils::DirectoryDoesNotExist( "/data/data/com.anki.victor/persistent/alexa" ) ) {
    Util::FileUtils::CreateDirectory( "/data/data/com.anki.victor/persistent/alexa", false, true );
  }
  
  using namespace alexaClientSDK;
  std::vector<std::shared_ptr<std::istream>> configJsonStreams;
  
  // todo: load from data loader
  configJsonStreams.push_back( std::shared_ptr<std::istringstream>(new std::istringstream(kConfig)) );
  
  // for (auto configFile : configFiles) {
  //   if (configFile.empty()) {
  //     alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Config filename is empty!");
  //     return;
  //   }
  
  //   auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
  //   if (!configInFile->good()) {
  //     ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
  //     alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file " + configFile);
  //     return;
  //   }
  
  //   configJsonStreams.push_back(configInFile);
  // }
  PRINT_NAMED_WARNING("WHATNOW", "calling");
  
  #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)
  
  // todo: this should be called befpre any other threads start (acccording to docs)
  if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize(configJsonStreams)) {
    //ACSDK_CRITICAL(LX("Failed to initialize SDK!"));
    PRINT_NAMED_WARNING("WHATNOW", "failed to init");
    return;
  }
  
  const auto& config = avsCommon::utils::configuration::ConfigurationNode::getRoot();
  
  /*
   * Creating customerDataManager which will be used by the registrationManager and all classes that extend
   * CustomerDataHandler
   */
  auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
  
  /*
   * Creating the deviceInfo object
   */
  std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create(config);
  if (!deviceInfo) {
    ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
    PRINT_NAMED_WARNING("WHATNOW", "deviceinfo failed");
    return;
  }
  
  /*
   * Creating the UI component that observes various components and prints to the console accordingly.
   */
  auto userInterfaceManager = std::make_shared<AlexaLogger>();
  
  //  /*
  //   * Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
  //   */
  auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(config);
  std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate =
  authorization::cblAuthDelegate::CBLAuthDelegate::create(
                                                          config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);
  
  // Creating the misc DB object to be used by various components.
  std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
    alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);
  
  // Create HTTP Put handler
  std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPut> httpPut =
    avsCommon::utils::libcurlUtils::HttpPut::create();

  if (!authDelegate) {
    ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
    PRINT_NAMED_WARNING("WHATNOW", "Creation of AuthDelegate failed!");
    return;
  }
  authDelegate->addAuthObserver(userInterfaceManager);
  
  /*
   * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
   * Capabilities API.
   */
  auto m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
    authDelegate, miscStorage, httpPut, customerDataManager, config, deviceInfo
  );
  if (!m_capabilitiesDelegate) {
    PRINT_NAMED_WARNING("WHATNOW", "Creation of CapabilitiesDelegate failed!");
    return;
  }
  m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);
  
  // add a capability (authentication won't work without it)
  // this one doesnt work
//  {
//    auto systemCapabilityProvider = capabilityAgents::system::SystemCapabilityProvider::create();
//    if (!systemCapabilityProvider) {
//      ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSystemCapabilityProvider"));
//      return;
//    }
//    if (!(m_capabilitiesDelegate->registerCapability(systemCapabilityProvider))) {
//      ACSDK_ERROR(
//                  LX("initializeFailed").d("reason", "unableToRegisterCapability").d("capabilitiesDelegate", "System"));
//      return;
//    }
//  }
  
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
    return;
  }
  
  /*
   * Create a factory for creating objects that handle tasks that need to be performed right after establishing
   * a connection to AVS.
   */
  auto m_postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create(contextManager);
  
  /*
   * Create a factory to create objects that establish a connection with AVS.
   */
  auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(m_postConnectSynchronizerFactory);
  
  /*
   * Creating the message router - This component actually maintains the connection to AVS over HTTP2. It is created
   * using the auth delegate, which provides authorization to connect to AVS, and the attachment manager, which helps
   * ACL write attachments received from AVS.
   */
  auto m_messageRouter = std::make_shared<acl::MessageRouter>(authDelegate, attachmentManager, transportFactory);
  
  auto m_dialogUXStateAggregator = std::make_shared<avsCommon::avs::DialogUXStateAggregator>();
  m_dialogUXStateAggregator->addObserver(userInterfaceManager);
  
  std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
    connectionObservers = {userInterfaceManager};
  
  /*
   * Creating the connection manager - This component is the overarching connection manager that glues together all
   * the other networking components into one easy-to-use component.
   */
  auto m_connectionManager =
  acl::AVSConnectionManager::create(m_messageRouter, false, connectionObservers, {m_dialogUXStateAggregator});
  if (!m_connectionManager) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateConnectionManager"));
    return;
  }
  
  auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
  auto internetConnectionMonitor =
  avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
  
  if (!internetConnectionMonitor) {
    ACSDK_CRITICAL(LX("initializeFailed").d("reason", "internetConnectionMonitor was nullptr"));
    return;
  }
  
  auto messageStorage1 = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);
  std::shared_ptr<certifiedSender::MessageStorageInterface> messageStorage{std::move(messageStorage1)};
  /*
   * Creating our certified sender - this component guarantees that messages given to it (expected to be JSON
   * formatted AVS Events) will be sent to AVS.  This nicely decouples strict message sending from components which
   * require an Event be sent, even in conditions when there is no active AVS connection.
   */
  auto m_certifiedSender = certifiedSender::CertifiedSender::create(
                                                               m_connectionManager, m_connectionManager, messageStorage, customerDataManager);
  if (!m_certifiedSender) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateCertifiedSender"));
    return;
  }
  
  
  /*
   * Creating the Exception Sender - This component helps the SDK send exceptions when it is unable to handle a
   * directive sent by AVS. For that reason, the Directive Sequencer and each Capability Agent will need this
   * component.
   */
  auto m_exceptionSender1 = avsCommon::avs::ExceptionEncounteredSender::create(m_connectionManager);
  if (!m_exceptionSender1) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateExceptionSender"));
    return;
  }
  std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> m_exceptionSender{m_exceptionSender1.release()};
  
    /*
     * Creating the Speech Synthesizer - This component is the Capability Agent that implements the SpeechSynthesizer
     * interface of AVS.
     */
  auto m_speechSynthesizer = capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer::create(m_exceptionSender);
//    auto m_speechSynthesizer = capabilityAgents::speechSynthesizer::SpeechSynthesizer::create(
//                                                                                         speakMediaPlayer,
//                                                                                         m_connectionManager,
//                                                                                         m_audioFocusManager,
//                                                                                         contextManager,
//                                                                                         m_exceptionSender,
//                                                                                         m_dialogUXStateAggregator);
  if (!m_speechSynthesizer) {
    ACSDK_ERROR(LX("initializeFailed").d("reason", "unableToCreateSpeechSynthesizer"));
    return;
  }

  //m_speechSynthesizer->addObserver(m_dialogUXStateAggregator);
  if (!(m_capabilitiesDelegate->registerCapability(m_speechSynthesizer))) {
    ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "unableToRegisterCapability")
                .d("capabilitiesDelegate", "SpeechSynthesizer"));
    return;
  }
  
  
  
  // try connecting
  m_capabilitiesDelegate->publishCapabilitiesAsyncWithRetries();

//  const bool displayCardsSupported = false;
//  const int firmwareVersion = 1;
//  std::unordered_map<std::string, std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
//    m_externalMusicProviderMediaPlayersMap;
//
//  /// The map of the adapters and their mediaPlayers.
//  std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
//    m_externalMusicProviderSpeakersMap;
//
//  /// The vector of mediaPlayers for the adapters.
//  class ApplicationMediaPlayer;
//  std::vector<std::shared_ptr<ApplicationMediaPlayer>> m_adapterMediaPlayers;
//
//  /// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
//  static capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap m_adapterToCreateFuncMap;
//
//  std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers;
//
//  auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
//
//  // build "DefaultClient"
//  std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> client =
//  alexaClientSDK::defaultClient::DefaultClient::create(
//       deviceInfo,
//       customerDataManager,
//       m_externalMusicProviderMediaPlayersMap,
//       m_externalMusicProviderSpeakersMap,
//       m_adapterToCreateFuncMap,
//       nullptr,//m_speakMediaPlayer,
//       nullptr,//m_audioMediaPlayer,
//       nullptr,//m_alertsMediaPlayer,
//       nullptr,//m_notificationsMediaPlayer,
//       nullptr,//m_bluetoothMediaPlayer,
//       nullptr,//m_ringtoneMediaPlayer,
//       nullptr,//speakSpeaker,
//       nullptr,//audioSpeaker,
//       nullptr,//alertsSpeaker,
//       nullptr,//notificationsSpeaker,
//       nullptr,//bluetoothSpeaker,
//       nullptr,//ringtoneSpeaker,
//       additionalSpeakers,
//       audioFactory,
//       authDelegate,
//       nullptr,//std::move(alertStorage),
//       nullptr,//std::move(messageStorage),
//       nullptr,//std::move(notificationsStorage),
//       nullptr,//std::move(settingsStorage),
//       nullptr,//std::move(bluetoothStorage),
//       {userInterfaceManager},
//       {userInterfaceManager},
//       nullptr,//std::move(internetConnectionMonitor),
//       displayCardsSupported,
//       nullptr,//m_capabilitiesDelegate,
//       firmwareVersion,
//       true,
//       nullptr);
//
//  if (!client) {
//    ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
//    return;
//  }
  
  PRINT_NAMED_WARNING("WHATNOW", "worked");
  return;

}
  
} // namespace Vector
} // namespace Anki
