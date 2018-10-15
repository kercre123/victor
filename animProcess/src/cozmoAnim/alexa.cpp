//Intentional build error so you read the below build instructions:
/*
 1) either build the coretech branch ross/add-AVS and copy the libraries to EXTERNALS/build, or obtain the necessary libraries from ross or jordan and put them there
 2) <removed>
 3) IMPORTANT: In the json in the top of this file, change the deviceSerialNumber to something else
 4) Build and deploy, then start the robot in DevDoNothing. I've been using the following filter:
       victor_log | grep "WHATNOW\|ALEXA\|vic-anim: 2018\|starved"
 5) On the first run, there should be a log line containing a URL and a code. If you need to log into a developer account, mine is
    username: ross+2@anki.com
    password: anki_tests_amazon
    Alternatively you can create your own account
    https://github.com/alexa/avs-device-sdk/wiki/macOS-Quick-Start-Guide#register-a-product
    and replace the relevant json below
 6) Once having activated the product, restart the robot. On the second run, it should authenticate.
 
 IN THIS DEMO:
   Both wake words work. If running in DevDoNothing, you can evaluate both wake words with getin animations.
   If you shake the robot,
   "Hey vector" works as usual
   "Alexa" will activate a listening behavior, which has a fixed timeout. It unfortunately sometimes plays the no-wifi animation at the end of it
 */

#include "cozmoAnim/alexa.h"
#include "cozmoAnim/alexaAlertManager.h"
#include "cozmoAnim/alexaClient.h"
#include "cozmoAnim/alexaLogger.h"
#include "cozmoAnim/alexaKeywordObserver.h"
#include "cozmoAnim/alexaMicrophone.h"
#include "cozmoAnim/alexaSpeaker.h"
#include "cozmoAnim/alexaWeatherParser.h"
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
#include <AVSCommon/AVS/AudioInputStream.h>
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
#include <ESP/DummyESPDataProvider.h>
//#include <DefaultClient/DefaultClient.h>

#include <memory>
#include <vector>
#include <iostream>
#include "util/console/consoleInterface.h"
#include "osState/osState.h"
#include "util/string/stringUtils.h"
#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Vector {
  
namespace {
    
  /// The sample rate of microphone audio data.
  static const unsigned int SAMPLE_RATE_HZ = 16000;
  
  /// The number of audio channels.
  static const unsigned int NUM_CHANNELS = 1;
  
  /// The size of each word within the stream.
  static const size_t WORD_SIZE = 2;
  
  /// The maximum number of readers of the stream.
  static const size_t MAX_READERS = 10;
  
  /// The amount of audio data to keep in the ring buffer.
  static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);
  
  /// The size of the ring buffer.
  static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();
  
  /// Key for the root node value containing configuration values for SampleApp.
  static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
  
  /// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
  static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
  
  /// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
  static const std::string ENDPOINT_KEY("endpoint");
  
  CONSOLE_VAR(bool, kUseWakeWord, "AAA", true);
  CONSOLE_VAR_RANGED(uint32_t, kAlexaSerialNumberModifier, "AAA", 1, 0, 100);
  
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
      "deviceSerialNumber":"<SERIAL_NUMBER>", // this gets replaced
      // The Client ID of the Product from developer.amazon.com
      "clientId": "amzn1.application-oa2-client.ada3c4fa4e2b4e5f93e48771c449c522",
      // Product ID from developer.amazon.com
      "productId": "test_product_2"
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
  
  using namespace alexaClientSDK;
  
void Alexa::Init(const AnimContext* context,
                 const OnStateChangedCallback& onStateChanged,
                 const OnAlertChangedCallback& onAlertChanged,
                 const OnWeatherCallback& onWeather,
                 const OnAudioPlayedCallback& onAudioPlayed,
                 const OnAudioPlaybackStarted& onAudioPlaybackStarted,
                 const OnAudioPlaybackEnded& onAudioPlaybackEnded)
{
  _context = context;
  _onStateChanged = onStateChanged;
  if( Util::FileUtils::DirectoryDoesNotExist( "/data/data/com.anki.victor/persistent/alexa" ) ) {
    Util::FileUtils::CreateDirectory( "/data/data/com.anki.victor/persistent/alexa", false, true );
  }
  
  _alertsManager.reset( new AlexaAlertsManager(context, onAlertChanged) );
  _weatherParser.reset( new AlexaWeatherParser(context, onWeather) );
  
  std::vector<std::shared_ptr<std::istream>> configJsonStreams;
  
  // todo: load from data loader
  std::string configWithSerial = kConfig;
  const std::string toReplace = "<SERIAL_NUMBER>";
  auto* osState = OSState::getInstance();
  const uint32_t esn = osState->GetSerialNumber() + kAlexaSerialNumberModifier;
  PRINT_NAMED_WARNING("WHATNOW", "Registering serial %s", std::to_string(esn).c_str());
  Util::StringReplace( configWithSerial, toReplace, std::to_string(esn) );
  configJsonStreams.push_back( std::shared_ptr<std::istringstream>(new std::istringstream(configWithSerial)) );
  
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
  m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
    authDelegate, miscStorage, httpPut, customerDataManager, config, deviceInfo
  );
  if (!m_capabilitiesDelegate) {
    PRINT_NAMED_WARNING("WHATNOW", "Creation of CapabilitiesDelegate failed!");
    return;
  }
  m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);
  
  auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);
  
  // Creating the alert storage object to be used for rendering and storing alerts.
  auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
  auto alertStorage =
    alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts());
  
  
  // setup "speakers"
  auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
  m_TTSSpeaker = std::make_shared<AlexaSpeaker>(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME, "TTS", httpContentFetcherFactory);
  m_TTSSpeaker->Init(context);
  m_alertsSpeaker = std::make_shared<AlexaSpeaker>(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME, "Alerts", httpContentFetcherFactory);
  m_alertsSpeaker->Init(context);
  m_audioSpeaker = std::make_shared<AlexaSpeaker>(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
                                                  "Audio",
                                                  httpContentFetcherFactory);
  m_audioSpeaker->Init(context);
  
  // receive info on audio being played
  m_audioSpeaker->SetPlayedAudioCallback( onAudioPlayed );
  m_audioSpeaker->SetOnPlaybackStarted( onAudioPlaybackStarted );
  m_audioSpeaker->SetOnPlaybackEnded( onAudioPlaybackEnded );
  
  // this isnt the constructor, but i seem to still need this
  // https://forum.libcinder.org/topic/solution-calling-shared-from-this-in-the-constructor
  auto wptr = std::shared_ptr<Alexa>( this, [](Alexa*){} );
  
  // the alerts/audio speakers dont change UX so we observe them
  m_alertsSpeaker->setObserver( shared_from_this() );
  m_audioSpeaker->setObserver( shared_from_this() );
  
  m_alertsSpeaker->DisableSource(2); // dont play source 2 since this is timer stuff
  m_alertsSpeaker->DisableSource(3); // dont play source 3 since this is timer stuff
  
  
  //alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface
  
  
  
  m_client = AlexaClient::create(
    deviceInfo,
    customerDataManager,
    authDelegate,
    std::move(messageStorage),
    std::move(alertStorage),
    audioFactory,
    {userInterfaceManager, shared_from_this() },
    {userInterfaceManager},
    m_capabilitiesDelegate,
    m_TTSSpeaker,
    m_alertsSpeaker,
    m_audioSpeaker,
    std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(m_TTSSpeaker),
    std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(m_alertsSpeaker),
    std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(m_audioSpeaker)
  );
  
  if (!m_client) {
    ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
    return;
  }
  
  m_client->addSpeakerManagerObserver(userInterfaceManager);
  m_client->SetDirectiveCallback( std::bind(&Alexa::OnDirective, this, std::placeholders::_1, std::placeholders::_2) );
  
  /*
   * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
   */
  size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
                                                                                            BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
  PRINT_NAMED_WARNING("WHATNOW", "bufferSize=%d", bufferSize);
  auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
  std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
  alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);
  
  if (!sharedDataStream) {
    ACSDK_CRITICAL(LX("Failed to create shared data stream!"));
    return;
  }
  
  alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
  compatibleAudioFormat.sampleRateHz = SAMPLE_RATE_HZ;
  compatibleAudioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
  compatibleAudioFormat.numChannels = NUM_CHANNELS;
  compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
  compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;
  
  /*
   * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
   * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
   * stream is used since this sample application will only have one microphone.
   */
  
  // Creating tap to talk audio provider
  bool tapAlwaysReadable = true;
  bool tapCanOverride = true;
  bool tapCanBeOverridden = true;
  
  m_tapToTalkAudioProvider = std::make_shared<alexaClientSDK::capabilityAgents::aip::AudioProvider>(
                                                                                                    sharedDataStream,
                                                                              compatibleAudioFormat,
                                                                              alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
                                                                              tapAlwaysReadable,
                                                                              tapCanOverride,
                                                                              tapCanBeOverridden);
  
  if( kUseWakeWord ) {
    bool wakeAlwaysReadable = true;
    bool wakeCanOverride = false;
    bool wakeCanBeOverridden = true;
    
    m_wakeWordAudioProvider = std::make_shared<capabilityAgents::aip::AudioProvider>(
                                                                               sharedDataStream,
                                                                               compatibleAudioFormat,
                                                                               alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
                                                                               wakeAlwaysReadable,
                                                                               wakeCanOverride,
                                                                               wakeCanBeOverridden);
    
    auto dummyEspProvider = std::make_shared<esp::DummyESPDataProvider>();
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider = dummyEspProvider;
    std::shared_ptr<esp::ESPDataModifierInterface> espModifier = dummyEspProvider;
//
//    // This observer is notified any time a keyword is detected and notifies the DefaultClient to start recognizing.
    m_keywordObserver = std::make_shared<KeywordObserver>(m_client, *m_wakeWordAudioProvider, espProvider);
//
//    m_keywordDetector = alexaClientSDK::kwd::KeywordDetectorProvider::create(
//                                                                             sharedDataStream,
//                                                                             compatibleAudioFormat,
//                                                                             {keywordObserver},
//                                                                             std::unordered_set<
//                                                                             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
//                                                                             pathToInputFolder);
//    if (!m_keywordDetector) {
//      ACSDK_CRITICAL(LX("Failed to create keyword detector!"));
//    }
    
//    // If wake word is enabled, then creating the interaction manager with a wake word audio provider.
//    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
//                                                                                           client,
//                                                                                           micWrapper,
//                                                                                           userInterfaceManager,
//                                                                                           holdToTalkAudioProvider,
//                                                                                           tapToTalkAudioProvider,
//                                                                                           m_guiRenderer,
//                                                                                           wakeWordAudioProvider,
//                                                                                           espProvider,
//                                                                                           espModifier);
  }
  

  
//  m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
//                                                                                         client, micWrapper, userInterfaceManager, holdToTalkAudioProvider, tapToTalkAudioProvider, m_guiRenderer);
//
//  m_dialogUXStateAggregator->addObserver(m_interactionManager);
  
  //m_client->addAlexaDialogStateObserver(m_interactionManager);
  
//  // Creating the input observer.
//  m_userInputManager = alexaClientSDK::sampleApp::UserInputManager::create(m_interactionManager, consoleReader);
//  if (!m_userInputManager) {
//    ACSDK_CRITICAL(LX("Failed to create UserInputManager!"));
//    return;
//  }
  
  // create a microphone
  m_microphone = AlexaMicrophone::create(sharedDataStream);
  m_microphone->startStreamingMicrophoneData();
  
  m_capabilitiesDelegate->addCapabilitiesObserver(m_client);
  
  // try connecting
  m_client->Connect( m_capabilitiesDelegate );
  
  PRINT_NAMED_WARNING("WHATNOW", "INIT FINISHED. (m_keywordObserver=%d, m_wakeWordAudioProvider=%d, stream=%d)",
                      m_keywordObserver!=nullptr, m_wakeWordAudioProvider!=nullptr, (m_wakeWordAudioProvider==nullptr) ? -1 : (m_wakeWordAudioProvider->stream != nullptr));
  return;

}
  
void Alexa::Update()
{
  if( m_TTSSpeaker != nullptr ) {
    m_TTSSpeaker->Update();
  }
  if( m_alertsSpeaker != nullptr ) {
    m_alertsSpeaker->Update();
  }
  if( m_audioSpeaker != nullptr ) {
    m_audioSpeaker->Update();
  }
  
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _timeToSetIdle >= 0.0f && currTime_s >= _timeToSetIdle && _playingSources.empty() ) {
    _dialogState = DialogUXState::IDLE;
    CheckForStateChange();
  }
    
}
  
void Alexa::ButtonPress()
{
  if( !m_client->notifyOfTapToTalk(*m_tapToTalkAudioProvider).get() ) {
    PRINT_NAMED_WARNING("WHATNOW", "Failed to notify tap to talk");
  }
}
  
void Alexa::TriggerWord(int from_ms, int to_ms)
{
  // int holds ~50 days
  if( !kUseWakeWord ) {
    return;
  }
  
  avsCommon::avs::AudioInputStream::Index fromIndex;
  avsCommon::avs::AudioInputStream::Index toIndex;
  if ( from_ms < 0 ) {
    fromIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;
  } else {
    fromIndex = from_ms; // first move into larger type
    fromIndex = 16*fromIndex;
    if( fromIndex > 100 ) {
      fromIndex -= 100;
    }
  }
  if ( to_ms < 0 ) {
    toIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;
  } else {
    toIndex = to_ms;
    toIndex = 16*toIndex + 100;
    if( toIndex > m_microphone->GetTotalSamples() ) {
      toIndex = m_microphone->GetTotalSamples();
    }
  }
  
  // fake it
  if ( from_ms >= 0 && to_ms >= 0 ) {
    PRINT_NAMED_WARNING("WHATNOW", "initial fromIndex=%llu, toIndex=%llu", fromIndex, toIndex);
    avsCommon::avs::AudioInputStream::Index dIndex = toIndex - fromIndex;
    toIndex = m_microphone->GetTotalSamples();
    fromIndex = toIndex - dIndex;
  }
  
  // this only works if our extra VAD is off, since otherwise the mic samples wont match the sample indices provided by sensory
  
  PRINT_NAMED_WARNING("WHATNOW", "alexa trigger %d-%d (%llu-%llu) micData=%d, (m_keywordObserver=%d, m_wakeWordAudioProvider=%d, stream=%d)", from_ms, to_ms, fromIndex, toIndex, m_microphone->GetTotalSamples(), m_keywordObserver!=nullptr, m_wakeWordAudioProvider!=nullptr, (m_wakeWordAudioProvider==nullptr) ? -1 : (m_wakeWordAudioProvider->stream != nullptr));
  // this can generate SdsReader:seekFailed:reason=seekOverwrittenData if the time indices contain overwritten data
  m_keywordObserver->onKeyWordDetected( m_wakeWordAudioProvider->stream, "ALEXA", fromIndex, toIndex, nullptr);
}

void Alexa::ProcessAudio( int16_t* data, size_t size)
{
  if( m_microphone ) {
    m_microphone->ProcessAudio(data, size);
  }
}
  
void Alexa::onDialogUXStateChanged(avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState)
{
  _dialogState = newState;
  CheckForStateChange();
}
  
void Alexa::CheckForStateChange()
{
  const auto oldState = _uxState;
  const bool anyPlaying = !_playingSources.empty();
  
  _timeToSetIdle = -1.0f;
  
  switch( _dialogState ) {
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::FINISHED:
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE:
    {
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      // if nothing is playing, it could be because somehting is about to play. We only know because
      // a directive for "Play" or "Speak" has arrived. If one has arrived recently, don't switch to idle yet,
      // and instead set a timer so that it will switch to idle only if no other state change occurs.
      // Sometimes this still doesnt happen, though, and it will momentarily break to normal behavior before
      // returning to alexa. Maybe we need a base delay
      if( anyPlaying ) {
        _uxState = AlexaUXState::Speaking;
      } else if( _lastPlayDirective < 0.0f || (currTime_s > _lastPlayDirective + 2.0f) ) {
        _uxState = AlexaUXState::Idle;
      } else {
        _timeToSetIdle = _lastPlayDirective + 1.0f;
      }
    }
      break;
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::LISTENING:
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::THINKING:
    {
      _uxState = AlexaUXState::Listening;
    }
      break;
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::SPEAKING:
    {
      _uxState = AlexaUXState::Speaking;
    }
      break;
  }
  PRINT_NAMED_WARNING("WHATNOW", "new AlexaUXState=%d (dialogState=%d, sources=%d, timeToSetIdle=%1.1f", (int) _uxState, (int)_dialogState, (int)_playingSources.size(), _timeToSetIdle );
  if( oldState != _uxState && _onStateChanged != nullptr) {
    _onStateChanged( _uxState );
  }
}
  
void Alexa::onPlaybackStarted (SourceId id)
{
  PRINT_NAMED_WARNING("WHATNOW", "CALLBACK PLAY %d", (int)id);
  _playingSources.insert(id);
  CheckForStateChange();
  //if( name == "Audio" ) {
//    OnAudioPlaying();
  //}
}
void Alexa::onPlaybackFinished (SourceId id)
{
  PRINT_NAMED_WARNING("WHATNOW", "CALLBACK FINISH %d", (int)id);
  _playingSources.erase(id);
  CheckForStateChange();
  //if( name == "Audio" ) {
  //  OnAudioStopped();
  //}
}
void Alexa::onPlaybackStopped(SourceId id)
{
  PRINT_NAMED_WARNING("WHATNOW", "CALLBACK STOPPED %d", (int)id);
  _playingSources.erase(id);
  CheckForStateChange();
  //if( name == "Audio" ) {
  //  OnAudioStopped();
  //}
}
  
void Alexa::onPlaybackError( SourceId id,
                            const avsCommon::utils::mediaPlayer::ErrorType& type,
                            std::string error)
{
  PRINT_NAMED_WARNING("WHATNOW", "CALLBACK ERROR %d", (int)id);
  // does an error mean a stop? probably
  _playingSources.erase(id);
  CheckForStateChange();
}
  
void Alexa::OnDirective(const std::string& directive, const std::string& payload)
{
  if( directive == "Play" || directive == "Speak" ) {
    _lastPlayDirective = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  else if( directive == "DeleteAlert" ) {
    PRINT_NAMED_WARNING("WHATNOW", "Deleting Alert");
    if( _alertsManager ) {
      _alertsManager->ProcessAlert(directive, payload);
    }
  }
  else if( directive == "SetAlert" ) {
    PRINT_NAMED_WARNING("WHATNOW", "Setting Alert");
    if( _alertsManager ) {
      _alertsManager->ProcessAlert(directive, payload);
    }
  } else if( directive == "RenderTemplate" ) {
    // this contains weather, and maybe more
    if( _weatherParser && payload.find("WeatherTemplate") != std::string::npos ) {
      _weatherParser->Parse(payload);
    }
  }
}

void Alexa::StopForegroundActivity()
{
  if( m_client ) {
    PRINT_NAMED_WARNING("WHATNOW", "alexa.cpp Stopping foreground activity");
    m_client->stopForegroundActivity();
  }
}
  
void Alexa::CancelAlerts(const std::vector<int>& alertIDs)
{
  if( _alertsManager ) {
    _alertsManager->CancelAlerts(alertIDs);
  }
}
  
  
} // namespace Vector
} // namespace Anki
