/**
 * File: alexaImpl.cpp
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

#include "cozmoAnim/alexa/alexaImpl.h"

#include "clad/types/alexaTypes.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/alexa/alexaAudioInput.h"
#include "cozmoAnim/alexa/alexaClient.h"
#include "cozmoAnim/alexa/alexaKeywordObserver.h"
#include "cozmoAnim/alexa/alexaObserver.h"
#include "cozmoAnim/alexa/alexaRevokeAuthObserver.h"
#include "cozmoAnim/alexa/media/alexaAudioFactory.h"
#include "cozmoAnim/alexa/media/alexaMediaPlayer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "osState/osState.h"
#include "osState/wallTime.h"
#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <Settings/SQLiteSettingStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/SQLiteSettingStorage.h>
#include <ESP/DummyESPDataProvider.h>

#include <chrono>
#include <fstream>

namespace Anki {
namespace Vector {
  
using namespace alexaClientSDK;
  
namespace {
  #define CRITICAL_SDK(event) ACSDK_CRITICAL(avsCommon::utils::logger::LogEntry(__FILE__, event))
  #define LOG_CHANNEL "Alexa"
  
  // The sample rate of microphone audio data.
  static const unsigned int kSampleRate_Hz = 16000;
  // The number of audio channels.
  static const unsigned int kNumChannels = 1;
  // The size of each word within the stream.
  static const size_t kWordSize = 2;
  // The maximum number of readers of the stream.
  static const size_t kMaxReaders = 10;
  // The amount of audio data to keep in the ring buffer.
  static const std::chrono::seconds kBufferDuration_s = std::chrono::seconds(15);
  // The size of the ring buffer.
  static const size_t kBufferSize = (kSampleRate_Hz)*kBufferDuration_s.count();

  // how often to double check playing states
  static const float kAlexaImplWatchdogCheckTime_s = 1.0f;
  
  const std::string kDirectivesFile = "directives.txt";
  const std::string kAlexaFolder = "alexa";

  static const float kTimeToHoldSpeakingBetweenAudioPlays_s = 1.0f;
  static const float kAlexaImplWatchdogFailTime_s = 5.0f;
  // TODO:(bn) FIXME: kAlexaImplWatchdogFailTime_s must be greater than kAlexaIdleDelay_s and
  // kAlexaMaxIdleDelay_s. Should add an assert
  
  // If the DialogUXState is set to Idle, but a directive was received to Play, then wait this long before
  // setting to idle
  CONSOLE_VAR_RANGED(float, kAlexaIdleDelay_s, "Alexa", 2.0f, 0.0f, 10.0f);
  // If the DialogUXState was set to Idle, but the last Play directive was older than this amount, then
  // switch to Idle
  CONSOLE_VAR_RANGED(float, kAlexaMaxIdleDelay_s, "Alexa", 3.0f, 0.0f, 10.0f);

  // enable to log directives received into a directives.txt file in the cache
  CONSOLE_VAR(bool, kLogAlexaDirectives, "Alexa", false );

  // enable to save a copy of the mic data out to the cache folder when we detect an "alexa" wake word
  // NOTE: only valid and read during Init, so make sure to save the console var to enable it (or edit the value here)
  CONSOLE_VAR(bool, kDumpAlexaTriggerAudio, "Alexa.Init", false);

  // every this many seconds (in basestation time), grab the wall time (system clock) and see if it looks like
  // it may have jumped. If so, refresh the alexa timers
  CONSOLE_VAR(float, kAlexaHackCheckForSystemClockSyncPeriod_s, "Alexa", 5.0f);

#if ANKI_DEV_CHEATS

  static AlexaImpl::SourceId sAddFakeSourcePlaying = 0;
  static AlexaImpl::SourceId sRemoveFakeSourcePlaying = 0;

  void AddFakePlayingSource(ConsoleFunctionContextRef context) {
    const int source = ConsoleArg_Get_Int(context, "source");

    if( sAddFakeSourcePlaying != 0 ) {
      LOG_ERROR("AlexaImpl.ConsoleVar.AddFakePlayingSource.AlreadyPending",
                "Trying to set fake source for adding to %d, but %llu is already pending",
                source,
                sAddFakeSourcePlaying);
    }
    else {
      sAddFakeSourcePlaying = source;
    }
  }
  CONSOLE_FUNC(AddFakePlayingSource, "Alexa.ImplWatchdog", int source);

  void RemoveFakePlayingSource(ConsoleFunctionContextRef context) {
    const int source = ConsoleArg_Get_Int(context, "source");

    if( sRemoveFakeSourcePlaying != 0 ) {
      LOG_ERROR("AlexaImpl.ConsoleVar.RemoveFakePlayingSource.AlreadyPending",
                "Trying to set fake source for removal to %d, but %llu is already pending",
                source,
                sRemoveFakeSourcePlaying);
    }
    else {
      sRemoveFakeSourcePlaying = source;
    }
  }
  CONSOLE_FUNC(RemoveFakePlayingSource, "Alexa.ImplWatchdog", int source);

#endif


  // TODO:(bn) cleanup: the AVS SDK already provides all of these with slightly different names, so let's use those instead...
  const char* DialogUXStateToString( const avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState& duxState ) {
    switch( duxState ) {
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::FINISHED: return "FINISHED";
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE: return "IDLE";
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::THINKING: return "THINKING";
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::LISTENING: return "LISTENING";
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::EXPECTING: return "EXPECTING";
      case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::SPEAKING: return "SPEAKING";
      default: return "UNKNOWN";
    }
  }

  const char* MessageStatusToString(const avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status& status ) {
    switch( status ) {
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PENDING: return "PENDING";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS: return "SUCCESS";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SUCCESS_NO_CONTENT: return "SUCCESS_NO_CONTENT";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_CONNECTED: return "NOT_CONNECTED";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::NOT_SYNCHRONIZED: return "NOT_SYNCHRONIZED";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::TIMEDOUT: return "TIMEDOUT";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::PROTOCOL_ERROR: return "PROTOCOL_ERROR";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR: return "INTERNAL_ERROR";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_INTERNAL_ERROR_V2: return "SERVER_INTERNAL_ERROR_V2";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::REFUSED: return "REFUSED";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::CANCELED: return "CANCELED";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::THROTTLED: return "THROTTLED";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::BAD_REQUEST: return "BAD_REQUEST";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::SERVER_OTHER_ERROR: return "SERVER_OTHER_ERROR";
      case avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INVALID_AUTH: return "INVALID_AUTH";
      default: return "UNKNOWN";
    }
  }

  const char* AuthErrorToString(const avsCommon::sdkInterfaces::AuthObserverInterface::Error& error) {
    switch(error) {
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::SUCCESS: return "SUCCESS";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::AUTHORIZATION_FAILED: return "AUTHORIZATION_FAILED";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::UNAUTHORIZED_CLIENT: return "UNAUTHORIZED_CLIENT";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::SERVER_ERROR: return "SERVER_ERROR";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::INVALID_REQUEST: return "INVALID_REQUEST";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::INVALID_VALUE: return "INVALID_VALUE";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::AUTHORIZATION_EXPIRED: return "AUTHORIZATION_EXPIRED";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::UNSUPPORTED_GRANT_TYPE: return "UNSUPPORTED_GRANT_TYPE";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::INVALID_CODE_PAIR: return "INVALID_CODE_PAIR";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::AUTHORIZATION_PENDING: return "AUTHORIZATION_PENDING";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::SLOW_DOWN: return "SLOW_DOWN";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::INTERNAL_ERROR: return "INTERNAL_ERROR";
      case avsCommon::sdkInterfaces::AuthObserverInterface::Error::INVALID_CBL_CLIENT_ID: return "INVALID_CBL_CLIENT_ID";
      default: return "UNKNOWN";
    }
  }

  const char* AlertStateToString(const capabilityAgents::alerts::AlertObserverInterface::State& state ) {
    switch(state) {
      case capabilityAgents::alerts::AlertObserverInterface::State::READY: return "READY";
      case capabilityAgents::alerts::AlertObserverInterface::State::STARTED: return "STARTED";
      case capabilityAgents::alerts::AlertObserverInterface::State::STOPPED: return "STOPPED";
      case capabilityAgents::alerts::AlertObserverInterface::State::SNOOZED: return "SNOOZED";
      case capabilityAgents::alerts::AlertObserverInterface::State::COMPLETED: return "COMPLETED";
      case capabilityAgents::alerts::AlertObserverInterface::State::PAST_DUE: return "PAST_DUE";
      case capabilityAgents::alerts::AlertObserverInterface::State::FOCUS_ENTERED_FOREGROUND: return "FOCUS_ENTERED_FOREGROUND";
      case capabilityAgents::alerts::AlertObserverInterface::State::FOCUS_ENTERED_BACKGROUND: return "FOCUS_ENTERED_BACKGROUND";
      case capabilityAgents::alerts::AlertObserverInterface::State::ERROR: return "ERROR";
      default: return "UNKNOWN";
    }
  }

}
  
#if ANKI_DEV_CHEATS
  DevShutdownChecker AlexaImpl::_shutdownChecker;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::AlexaImpl()
  : _authState{ AlexaAuthState::Uninitialized }
  , _uxState{ AlexaUXState::Idle }
  , _dialogState{ DialogUXState::IDLE }
  , _notificationsIndicator{ avsCommon::avs::IndicatorState::UNDEFINED }
  , _initState{ InitState::Uninitialized }
  , _runSetNetworkConnectionError{ false }
{
  static_assert( std::is_same<SourceId, avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId>::value,
                 "Our shorthand SourceId type differs" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::~AlexaImpl()
{
  RemoveCallbacksForShutdown();
  
  if( _capabilitiesDelegate ) {
    _capabilitiesDelegate->shutdown();
  }
  
  // First clean up anything that depend on the the MediaPlayers.
  _client.reset();
  
  // Now it's safe to shut down the MediaPlayers.
  if( _ttsMediaPlayer ) {
    _ttsMediaPlayer->shutdown();
  }
  if( _alertsMediaPlayer ) {
    _alertsMediaPlayer->shutdown();
  }
  if( _audioMediaPlayer ) {
    _audioMediaPlayer->shutdown();
  }
  if( _notificationsMediaPlayer ) {
    _notificationsMediaPlayer->shutdown();
  }
  
  avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Init( const AnimContext* context, InitCompleteCallback&& completionCallback )
{
  _context = context;
  _initCompleteCallback = std::move(completionCallback);

  _lastWallTime = WallTime::getInstance()->GetApproximateTime();
  _lastWallTimeCheck_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const auto* dataPlatform = _context->GetDataPlatform();
  
  _alexaPersistentFolder = dataPlatform->pathToResource( Util::Data::Scope::Persistent, kAlexaFolder );
  _alexaPersistentFolder = Util::FileUtils::AddTrailingFileSeparator( _alexaPersistentFolder );
  if( !_alexaPersistentFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _alexaPersistentFolder ) ) {
    Util::FileUtils::CreateDirectory( _alexaPersistentFolder );
  }
  
  _alexaCacheFolder = dataPlatform->pathToResource( Util::Data::Scope::Cache, kAlexaFolder );
  _alexaCacheFolder = Util::FileUtils::AddTrailingFileSeparator( _alexaCacheFolder );
  if( !_alexaCacheFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _alexaCacheFolder ) ) {
    Util::FileUtils::CreateDirectory( _alexaCacheFolder );
  }
  
#if ANKI_DEV_CHEATS
  if( Util::FileUtils::FileExists( _alexaCacheFolder + kDirectivesFile ) ) {
    Util::FileUtils::DeleteFile( _alexaCacheFolder + kDirectivesFile );
  }
#endif
  
  // start the sdk async init process via Update()
  _initState = InitState::PreInit;
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::InitThread()
{
  auto configs = GetConfigs();
  if( configs.empty() ) {
    _initState = InitState::ThreadFailed;
    return;
  }
  
  // TODO: docs say this should be called before any other threads start. I'm guessing theyre talking
  // about alexa threads.
  if( !avsCommon::avs::initialization::AlexaClientSDKInit::initialize( configs ) ) {
    CRITICAL_SDK("Failed to initialize SDK!");
    _initState = InitState::ThreadFailed;
    return;
  }
  
  // The observer logs important changes in a variety of sdk components, and calls back to AlexaImpl
  // for things we register to. It's not possible to do away with this object and instead pass
  // shared_from_this{AlexaImpl} to methods, even if AlexaImpl derived from the same base classes as AlexaObserver,
  // because Alexa needs to be able to destroy AlexaImpl. Use of shared_from_this would mean that
  // AlexaImpl and its children would form a group of self-referencing shared_ptrs that could never be
  // deleted. Instead, AlexaImpl can be held as a unique_ptr in Alexa, but it means that we need this additional
  // object AlexaObserver.
  _observer = std::make_shared<AlexaObserver>();
  _observer->Init( std::bind(&AlexaImpl::OnDialogUXStateChanged, this, std::placeholders::_1),
                   std::bind(&AlexaImpl::OnRequestAuthorization, this, std::placeholders::_1, std::placeholders::_2),
                   std::bind(&AlexaImpl::OnAuthStateChange, this, std::placeholders::_1, std::placeholders::_2),
                   std::bind(&AlexaImpl::OnSourcePlaybackChange, this, std::placeholders::_1, std::placeholders::_2),
                   std::bind(&AlexaImpl::OnInternetConnectionChanged, this, std::placeholders::_1),
                   std::bind(&AlexaImpl::OnAVSConnectionChanged, this, std::placeholders::_1, std::placeholders::_2),
                   std::bind(&AlexaImpl::OnSendComplete, this, std::placeholders::_1),
                   std::bind(&AlexaImpl::OnSDKLogout, this),
                   std::bind(&AlexaImpl::OnNotificationsIndicator, this, std::placeholders::_1),
                   std::bind(&AlexaImpl::OnAlertState, this, std::placeholders::_1, std::placeholders::_2),
                   std::bind(&AlexaImpl::OnPlayerActivity, this, std::placeholders::_1));
  
  const auto& rootConfig = avsCommon::utils::configuration::ConfigurationNode::getRoot();
  
  // Creating customerDataManager which will be used by the registrationManager and all classes that extend
  // CustomerDataHandler
  // TODO (VIC-9837) see what else this baby can do
  auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
  
  // Creating the deviceInfo object
  std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create( rootConfig );
  if( !deviceInfo ) {
    CRITICAL_SDK("Creation of DeviceInfo failed!");
    _initState = InitState::ThreadFailed;
    return;
  }
  else {
    DASMSG(avs_device_info, "alexa.device_info", "we created an alexa object with the given device info");
    DASMSG_SET(s1, deviceInfo->getProductId(), "product id (from json)");
    DASMSG_SEND();
  }
  
  // Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
  auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create( rootConfig );
  std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate
    = authorization::cblAuthDelegate::CBLAuthDelegate::create( rootConfig,
                                                               customerDataManager,
                                                               std::move(authDelegateStorage),
                                                               _observer,
                                                               nullptr,
                                                               deviceInfo );
  
  // Creating the misc DB object to be used by various components.
  std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> miscStorage = storage::sqliteStorage::SQLiteMiscStorage::create(rootConfig);
  if( !miscStorage ) {
    CRITICAL_SDK("Creation of miscStorage failed!");
    _initState = InitState::ThreadFailed;
    return;
  }
  
  // Create HTTP Put handler
  std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPut> httpPut = avsCommon::utils::libcurlUtils::HttpPut::create();
  
  if( !authDelegate ) {
    CRITICAL_SDK("Creation of AuthDelegate failed!");
    _initState = InitState::ThreadFailed;
    return;
  }
  authDelegate->addAuthObserver( _observer );
  
  // Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
  // Capabilities API.
  _capabilitiesDelegate
    = capabilitiesDelegate::CapabilitiesDelegate::create( authDelegate,
                                                          miscStorage,
                                                          httpPut,
                                                          customerDataManager,
                                                          rootConfig,
                                                          deviceInfo );
  if( !_capabilitiesDelegate ) {
    CRITICAL_SDK("Creation of CapabilitiesDelegate failed!");
    _initState = InitState::ThreadFailed;
    return;
  }
  _capabilitiesDelegate->addCapabilitiesObserver( _observer );
  
  auto messageStorage = certifiedSender::SQLiteMessageStorage::create( rootConfig );
  
  auto notificationsStorage
    = capabilityAgents::notifications::SQLiteNotificationsStorage::create( rootConfig );
  
  auto settingsStorage = capabilityAgents::settings::SQLiteSettingStorage::create( rootConfig );
  
  // Creating the alert storage object to be used for rendering and storing alerts.
  auto audioFactory = std::make_shared<AlexaAudioFactory>();
  auto alertStorage
    = capabilityAgents::alerts::storage::SQLiteAlertStorage::create( rootConfig,
                                                                     audioFactory->alerts() );
  // setup media player interfaces
  auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
  _ttsMediaPlayer = std::make_shared<AlexaMediaPlayer>( AlexaMediaPlayer::Type::TTS,
                                                        httpContentFetcherFactory );
  _ttsMediaPlayer->Init( _context );
  _alertsMediaPlayer = std::make_shared<AlexaMediaPlayer>( AlexaMediaPlayer::Type::Alerts,
                                                           httpContentFetcherFactory );
  _alertsMediaPlayer->Init( _context );
  _audioMediaPlayer = std::make_shared<AlexaMediaPlayer>( AlexaMediaPlayer::Type::Audio,
                                                          httpContentFetcherFactory );
  _audioMediaPlayer->Init( _context );
  _notificationsMediaPlayer = std::make_shared<AlexaMediaPlayer>( AlexaMediaPlayer::Type::Notifications,
                                                                  httpContentFetcherFactory );
  _notificationsMediaPlayer->Init( _context );
  
  // the alerts/audio/notifications speakers dont change UX so we observe them
  _alertsMediaPlayer->setObserver( _observer );
  _audioMediaPlayer->setObserver( _observer );
  _notificationsMediaPlayer->setObserver( _observer );
  
  
  // Create an internet connection monitor that will notify the _observer when internet connectivity changes.
  // Note that this seems to poll every DEFAULT_TEST_PERIOD=5 mins, which isn't enough to depend on for error states. However, it
  // does provide an accurate internet status on init
  auto internetConnectionMonitor = avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
  if( !internetConnectionMonitor ) {
    CRITICAL_SDK("Failed to create InternetConnectionMonitor");
    _initState = InitState::ThreadFailed;
    return;
  }
  
  const avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion = GetFirmwareVersion();
  
  
  _client = AlexaClient::Create( deviceInfo,
                                 customerDataManager,
                                 authDelegate,
                                 std::move(messageStorage),
                                 std::move(alertStorage),
                                 std::move(notificationsStorage),
                                 std::move(settingsStorage),
                                 audioFactory,
                                 {_observer},
                                 {_observer},
                                 {_observer},
                                 _capabilitiesDelegate,
                                 _ttsMediaPlayer,
                                 _alertsMediaPlayer,
                                 _audioMediaPlayer,
                                 _notificationsMediaPlayer,
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_ttsMediaPlayer),
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_alertsMediaPlayer),
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_audioMediaPlayer),
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_notificationsMediaPlayer),
                                 std::move(internetConnectionMonitor),
                                 firmwareVersion );
  
  if( !_client ) {
    CRITICAL_SDK("Failed to create SDK client!");
    _initState = InitState::ThreadFailed;
    return;
  }
  
  _client->SetDirectiveCallback( std::bind(&AlexaImpl::OnDirective, this, std::placeholders::_1, std::placeholders::_2) );
  
  _client->AddSpeakerManagerObserver( _observer );
  _client->AddInternetConnectionObserver( _observer );
  _client->AddNotificationsObserver( _observer );
  _client->AddAlertObserver( _observer );
  _client->AddAudioPlayerObserver( _observer );
  
   // Creating the revoke authorization observer.
  auto revokeObserver = std::make_shared<AlexaRevokeAuthObserver>( _client->GetRegistrationManager() );
  _client->AddRevokeAuthorizationObserver( revokeObserver );

  // Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
  size_t bufferSize = avsCommon::avs::AudioInputStream::calculateBufferSize( kBufferSize,
                                                                             kWordSize,
                                                                             kMaxReaders );
  auto buffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>( bufferSize );
  std::shared_ptr<avsCommon::avs::AudioInputStream> sharedDataStream
    = avsCommon::avs::AudioInputStream::create( buffer, kWordSize, kMaxReaders );

  if( !sharedDataStream ) {
    CRITICAL_SDK("Failed to create shared data stream!");
    _initState = InitState::ThreadFailed;
    return;
  }

  avsCommon::utils::AudioFormat compatibleAudioFormat;
  compatibleAudioFormat.sampleRateHz = kSampleRate_Hz;
  compatibleAudioFormat.sampleSizeInBits = kWordSize * CHAR_BIT;
  compatibleAudioFormat.numChannels = kNumChannels;
  compatibleAudioFormat.endianness = avsCommon::utils::AudioFormat::Endianness::LITTLE;
  compatibleAudioFormat.encoding = avsCommon::utils::AudioFormat::Encoding::LPCM;

  // Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
  // of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
  // stream is used since this sample application will only have one microphone.

  const auto kNearField = capabilityAgents::aip::ASRProfile::NEAR_FIELD;
  
  {
    // Creating tap to talk audio provider
    bool tapAlwaysReadable = true;
    bool tapCanOverride = true;
    bool tapCanBeOverridden = true;
    _tapToTalkAudioProvider
      = std::make_shared<capabilityAgents::aip::AudioProvider>( sharedDataStream,
                                                                compatibleAudioFormat,
                                                                kNearField,
                                                                tapAlwaysReadable,
                                                                tapCanOverride,
                                                                tapCanBeOverridden );
  }

  {
    bool wakeAlwaysReadable = true;
    bool wakeCanOverride = false;
    bool wakeCanBeOverridden = true;
  
    _wakeWordAudioProvider = std::make_shared<capabilityAgents::aip::AudioProvider>( sharedDataStream,
                                                                                     compatibleAudioFormat,
                                                                                     kNearField,
                                                                                     wakeAlwaysReadable,
                                                                                     wakeCanOverride,
                                                                                     wakeCanBeOverridden );
  
    auto dummyEspProvider = std::make_shared<esp::DummyESPDataProvider>();
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider = dummyEspProvider;
    std::shared_ptr<esp::ESPDataModifierInterface> espModifier = dummyEspProvider;
    // This observer is notified any time a keyword is detected and notifies the AlexaClient to start recognizing.
    _keywordObserver = std::make_shared<AlexaKeywordObserver>( _client, *_wakeWordAudioProvider, espProvider );
  }
  
  _microphone = AlexaAudioInput::Create( sharedDataStream );
  _microphone->startStreamingMicrophoneData();


  if( ANKI_DEV_CHEATS && kDumpAlexaTriggerAudio ) {
    auto debugBuffer = std::make_shared<avsCommon::avs::AudioInputStream::Buffer>( bufferSize );

    std::shared_ptr<avsCommon::avs::AudioInputStream> debugDataStream
      = avsCommon::avs::AudioInputStream::create( debugBuffer, kWordSize, kMaxReaders );
    _debugMicrophone = AlexaAudioInput::Create( debugDataStream );
    _debugMicrophone->startStreamingMicrophoneData();
  }
  
  _capabilitiesDelegate->addCapabilitiesObserver( _client );
  
  // try connecting... this is already async but let's trigger it from this thread to keep everything in one place
  _client->Connect( _capabilitiesDelegate );
  
  // Send default settings set by the user to AVS.
  _client->SendDefaultSettings();
  
  _initState = InitState::ThreadComplete;

#if ANKI_DEV_CHEATS
  #define ADD_TO_SHUTDOWN_CHECKER(x) _shutdownChecker.AddObject(#x, x)
  ADD_TO_SHUTDOWN_CHECKER( _observer );
  ADD_TO_SHUTDOWN_CHECKER( _capabilitiesDelegate );
  ADD_TO_SHUTDOWN_CHECKER( _ttsMediaPlayer );
  ADD_TO_SHUTDOWN_CHECKER( _alertsMediaPlayer );
  ADD_TO_SHUTDOWN_CHECKER( _audioMediaPlayer );
  ADD_TO_SHUTDOWN_CHECKER( _notificationsMediaPlayer );
  ADD_TO_SHUTDOWN_CHECKER( _tapToTalkAudioProvider );
  ADD_TO_SHUTDOWN_CHECKER( _wakeWordAudioProvider );
  ADD_TO_SHUTDOWN_CHECKER( _microphone );
  ADD_TO_SHUTDOWN_CHECKER( _debugMicrophone );
  ADD_TO_SHUTDOWN_CHECKER( _client );
  ADD_TO_SHUTDOWN_CHECKER( _keywordObserver );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Update()
{
  if( (_initState == InitState::PreInit) ||
      (_initState == InitState::Initing) ||
      (_initState == InitState::ThreadComplete) ||
      (_initState == InitState::ThreadFailed) )
  {
    UpdateAsyncInit();
  } else if( _initState != InitState::Completed ) {
    return;
  }

#if ANKI_DEV_CHEATS

  // NOTE: these (intentionally) don't call CheckForUXStateChange, so they may not have immediate impact until
  // something else happens
  // TODO:(bn) console func for that too

  if( sAddFakeSourcePlaying != 0 ) {
    LOG_WARNING("AlexaImpl.Update.AddingFakePlayingSource",
                "Adding source %llu from console var request",
                sAddFakeSourcePlaying);
    _playingSources.insert(sAddFakeSourcePlaying);
    sAddFakeSourcePlaying = 0;
  }

  if( sRemoveFakeSourcePlaying != 0 ) {
    LOG_WARNING("AlexaImpl.Update.RemovingFakePlayingSource",
                "Removing source %llu from console var request",
                sRemoveFakeSourcePlaying);
    _playingSources.erase(sRemoveFakeSourcePlaying);
    sRemoveFakeSourcePlaying = 0;
  }
#endif
  
  if( _runSetNetworkConnectionError ) {
    _runSetNetworkConnectionError = false;
    SetNetworkConnectionError();
  }
  
  if( _observer != nullptr ) {
    _observer->Update();
  }
  if( _ttsMediaPlayer != nullptr ) {
    _ttsMediaPlayer->Update();
  }
  if( _alertsMediaPlayer != nullptr ) {
    _alertsMediaPlayer->Update();
  }
  if( _audioMediaPlayer != nullptr ) {
    _audioMediaPlayer->Update();
  }
  if( _notificationsMediaPlayer != nullptr ) {
    _notificationsMediaPlayer->Update();
  }
  
  // check if the idle timer _timeToSetIdle_s has expired
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( kAlexaHackCheckForSystemClockSyncPeriod_s > 0.0f &&
      currTime_s >= _lastWallTimeCheck_s + kAlexaHackCheckForSystemClockSyncPeriod_s ) {
    // HACK: NTP can cause the time to jump into the future, and the alexa client's timers won't properly ring
    // if this happens after initialization. OSState::IsWallTimeSynced() exists to try to detect this, but
    // appears to be unreliable (see JIRA VIC-4527), so use this hack instead.

    constexpr const int kTimeToConsiderJump_s = 5;

    auto wallTime = WallTime::getInstance()->GetApproximateTime();
    auto delta = wallTime - _lastWallTime;
    auto deltaSeconds = std::chrono::duration_cast<std::chrono::seconds>(delta).count();

    if( std::abs( deltaSeconds - kAlexaHackCheckForSystemClockSyncPeriod_s ) > kTimeToConsiderJump_s ) {
      PRINT_NAMED_WARNING("AlexaImpl.Update.TimeJumpDetected.ResetTimers",
                          "Detected a time jump of %lld seconds (in %f BS seconds), refreshing timers",
                          deltaSeconds,
                          kAlexaHackCheckForSystemClockSyncPeriod_s);
      _client->ReinitializeAllTimers();
    }

    _lastWallTime = wallTime;
    _lastWallTimeCheck_s = currTime_s;
  }

  // TODO: merge _nextUXStateCheckTime_s logic into _timeToSetIdle_s
  bool checkedUXState = false;
  if( _timeToSetIdle_s >= 0.0f && currTime_s >= _timeToSetIdle_s && _playingSources.empty() ) {
    if( _dialogState != DialogUXState::IDLE) {
      _dialogState = DialogUXState::IDLE;
      LOG_INFO( "AlexaImpl.Update.SetDialogStateIdle", "the timer to set idle elapsed, updating dialog state to idle" );
    }
    checkedUXState = true;
    CheckForUXStateChange();
  }

  if( (_nextUXStateCheckTime_s > 0.0f) && (currTime_s >= _nextUXStateCheckTime_s) ) {
    _nextUXStateCheckTime_s = 0.0f;
    if( !checkedUXState ) {
      CheckForUXStateChange();
    }
  }

  CheckStateWatchdog();
  // check watchdog
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::CheckStateWatchdog()
{
  // TODO:(bn) for now this only detects being stuck in "speaking"

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currTime_s > _lastWatchdogCheckTime_s + kAlexaImplWatchdogCheckTime_s ) {

    if( _uxState == AlexaUXState::Speaking ) {
      // attempt to detect "forever face" bugs that could happen if we get out of sync with one of the
      // speakers. If we think we're speaking, but nothing is playing, and this state persists for a while,
      // then we are likely "stuck"

      // NOTE: there is no code here to check if alerts are active (foreground or background). This is
      // intentional, if the alerts are active we hold the state speaking, but we do that because we expect
      // the Alerts player to start making noise momentarily

      bool anyActivePlayers = false;
      for(const auto& playerPtr : {_ttsMediaPlayer, _alertsMediaPlayer, _audioMediaPlayer, _notificationsMediaPlayer}) {
        anyActivePlayers |= playerPtr->IsActive();
      }

      const bool haveStuckTimer = (_possibleStuckStateStartTime_s >= 0.0f);

      if( haveStuckTimer && anyActivePlayers ) {
        // we thought we might be stuck previously, but now we know we aren't, yay!
        LOG_INFO( "AlexaImpl.CheckStateWatchdog.NotStuck",
                  "previously thought we might be stuck, but a player is active so we aren't stuck (here at least)" );
        // reset stuck timer
        _possibleStuckStateStartTime_s = -1.0f;
      }
      else if( haveStuckTimer && !anyActivePlayers ) {
        // we still appear to be stuck
        const bool stuckForTooLong = (currTime_s >= _possibleStuckStateStartTime_s + kAlexaImplWatchdogFailTime_s);
        if( stuckForTooLong ) {
          // we're in a bad place, try to un-stick the state
          LOG_ERROR("AlexaImpl.CheckStateWatchdog.FAIL",
                    "-------------------- Watchdog timer expired --------------------");

          AttemptToFixStuckInSpeakingBug();
          _possibleStuckStateStartTime_s = -1.0f;
        }
        // else, we're stuck but it hasn't been long enough yet so wait a little longer
      }
      else if( !haveStuckTimer && !anyActivePlayers ) {
        // uh oh, we are speaking but there are no players active. This could be OK if we are just holding
        // speaking for a few seconds.

        if( _possibleStuckStateStartTime_s < 0.0f ) {
          _possibleStuckStateStartTime_s = currTime_s;
          LOG_INFO( "AlexaImpl.CheckStateWatchdog.WatchdogActive",
                    "At t=%f detected that no players are active, so we might be stuck (but probably not)",
                    currTime_s );
        }
      }
      else {
        // LOG_INFO( "AlexaImpl.CheckStateWatchdog.HonkeyDorey", "ALL GOOD active=%d", anyActivePlayers );
      }
    }
    else {
      if( _possibleStuckStateStartTime_s > 0.0f ) {
        LOG_INFO( "AlexaImpl.CheckStateWatchdog.Cleared",
                  "State changed to '%s' to clearing possibly stuck state",
                  EnumToString(_uxState) );
        _possibleStuckStateStartTime_s = -1.0f;
      }
    }


    _lastWatchdogCheckTime_s = currTime_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::AttemptToFixStuckInSpeakingBug()
{
  LOG_ERROR("AlexaImpl.WatchdogFailure.AttempToFix", "Attempting to fix watchdog state failure");

  // could be a stuck playing source, so clear that out
  const size_t sourceCounts = _playingSources.size();
  _playingSources.clear();

  // could also be a stuck alert, so let's kill those too
  const bool alertWasActive = _alertActive;
  _alertActive = false;

  const bool alertWasBackground = _backgroundAlertActive;
  _backgroundAlertActive = false;

  const size_t alertsSize = _alertStates.size();
  _alertStates.clear();

  CheckForUXStateChange();

  const bool recovered = ANKI_VERIFY(_uxState == AlexaUXState::Idle,
                                     "AlexaImple.WatchdogFailFail",
                                     "Failed to reset state properly (still %s), force setting ux state....",
                                     EnumToString(_uxState) );

  DASMSG(watchdog_recovery, "alexa.watchdog_failure_recovery",
         "alexa ux state attempted to recover from ux failure");
  DASMSG_SET(s1, recovered ? "SUCCESS" : "FAILURE", "Success or failure of recovery");
  DASMSG_SET(s2, EnumToString(_uxState), "UX state after recovery (should be Idle if success)");
  DASMSG_SET(i1, sourceCounts, "Number of sources that were supposedly playing");
  DASMSG_SET(i2, alertsSize, "Number of alerts that were supposedly active");
  DASMSG_SET(i3, alertWasActive, "Was alert counted as active?");
  DASMSG_SET(i4, alertWasBackground, "Was alert counted as active in the background?");
  DASMSG_SEND();

  if( !recovered ) {
    // uh oh, something went _really_ wrong, let's clear the dialog state, force the ux state and send it off
    // so we at least might be able to get vector back
    _dialogState = avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE;
    _uxState = AlexaUXState::Idle;
    _audioActive = false;
    _audioActiveLastChangeTime_s = 0.0f;
    _onAlexaUXStateChanged( _uxState );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::UpdateAsyncInit()
{
  if( (_initState == InitState::Uninitialized) ||
      (_initState == InitState::Completed)     ||
      (_initState == InitState::Failed) )
  {
    return;
  }
  
  if( !_initThread.joinable() ) {
    _initState = InitState::Initing;
    _initThread = std::thread(&AlexaImpl::InitThread, this);
    return;
  }
  
  if( (_initState == InitState::ThreadComplete) || (_initState == InitState::ThreadFailed) ) {
    // loading is now done
    _initThread.join();
    _initThread = std::thread();
    const bool success = (_initState == InitState::ThreadComplete);
    _initState = success ? InitState::Completed : InitState::Failed;
    if( _initCompleteCallback ) {
      _initCompleteCallback( success );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::SetLocale( const Util::Locale& locale )
{
  if( ANKI_VERIFY( _client != nullptr,
                   "AlexaImpl.SetLocale.NotInitialized",
                   "Could not set locale without a client" ) )
  {
    const std::string kSettingKey = "locale";
    
    // possible values: de-DE, en-AU, en-CA, en-GB, en-IN, en-US, es-ES, es-MX, fr-FR, ja-JP
    // ( https://developer.amazon.com/docs/alexa-voice-service/settings.html )
    // obtain the en-COUNTRY locale string from the passed-in locale's country. Note that if a
    // user goes into their alexa app and changes locale to something like fr-CA, then goes into chewie
    // and selects CA, we send to amazon en-CA, and they'll have to go back into the alexa app if they
    // need to reselect french.
    std::string settingValue;
    switch( locale.GetCountry() ) {
      case Util::Locale::CountryISO2::US:
      {
        settingValue = "en-US";
        break;
      }
      case Util::Locale::CountryISO2::CA:
      {
        settingValue = "en-CA";
        break;
      }
      case Util::Locale::CountryISO2::GB:
      {
        settingValue = "en-GB";
        break;
      }
      case Util::Locale::CountryISO2::AU:
      {
        settingValue = "en-AU";
        break;
      }
      default:
        break;
    }
    
    // Only notify the client if the user selected a country we expect (i.e., those listed in chewie)
    if( !settingValue.empty() ) {
      _client->ChangeSetting( kSettingKey, settingValue );
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Logout()
{
  bool logoutCalled = false;

  if( _client ) {
    auto regManager = _client->GetRegistrationManager();
    if( regManager ) {
      regManager->logout();
      logoutCalled = true;
    }
  }

  if( !logoutCalled ) {
    DASMSG(user_sign_out_result, "alexa.user_sign_out_result", "User signed out of AVS SDK");
    DASMSG_SET(s1, "FAILURE", "SUCCESS or FAILURE (note that this instance always sends failure)");
    DASMSG_SET(s2, "INTERNAL_ERROR", "error type (only INTERNAL_ERROR currently exists, meaning could not call logout)");
    DASMSG_SEND();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::StopAlert()
{
  if( _client ) {
    _client->StopAlerts();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples )
{
  if( _microphone != nullptr ) {
    _microphone->AddSamples( samples, nSamples );
  }
#if ANKI_DEV_CHEATS
  if( _debugMicrophone != nullptr ) {
    _debugMicrophone->AddSamples( samples, nSamples );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::shared_ptr<std::istream>> AlexaImpl::GetConfigs() const
{
  std::vector<std::shared_ptr<std::istream>> configJsonStreams;
  
  const auto* dataLoader = _context->GetDataLoader();
  const auto* dataPlatform = _context->GetDataPlatform();
  if( (dataLoader == nullptr) || (dataPlatform == nullptr) ) {
    return configJsonStreams;
  }
  
  std::string configJson = dataLoader->GetAlexaConfig();
  auto* osState = OSState::getInstance();
  const uint32_t esn = osState->GetSerialNumber();
  Util::StringReplace( configJson, "<SERIAL_NUMBER>", std::to_string(esn) );
  
  const std::string pathToPersistent = _alexaPersistentFolder;
  Util::StringReplace( configJson, "<ALEXA_STORAGE_PATH>", pathToPersistent );
  
  configJsonStreams.push_back( std::shared_ptr<std::istringstream>( new std::istringstream( configJson ) ) );
  return configJsonStreams;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnDirective(const std::string& directive, const std::string& payload)
{
  if( directive == "Play" || directive == "Speak" || directive == "SetAlert" || directive == "DeleteAlert" ) {
    _lastPlayDirective_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  // TODO: is there a Stop directive we can listed to? that should force the Idle ux state or at least reset idle timers
  // "StopCapture" might be it. needs further investigation to see how it overlaps with music audio
  
  if( kLogAlexaDirectives ) {
    Json::Value json;
    Json::Reader reader;
    const float bsTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    bool success = reader.parse( payload, json );
    if( success ) {
      const bool append = true;
      Util::FileUtils::WriteFile( _alexaCacheFolder + kDirectivesFile,
                                  std::to_string(bsTime) + " " + directive + ": " + json.toStyledString(),
                                  append );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnAuthStateChange( avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
                                   avsCommon::sdkInterfaces::AuthObserverInterface::Error error )
{
  using State = avsCommon::sdkInterfaces::AuthObserverInterface::State;
  using Error = avsCommon::sdkInterfaces::AuthObserverInterface::Error;
  
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING("AlexaImpl.OnAuthStateChange", "authStateChanged newState=%d, error=%d", (int)newState, (int)error);


  auto sendResultDas = [](bool success, avsCommon::sdkInterfaces::AuthObserverInterface::Error error) {
    DASMSG(user_sign_in_result, "alexa.user_sign_in_result", "Result of initial user sign in attempt");
    DASMSG_SET(s1, success ? "SUCCESS" : "FAILURE", "SUCCESS or FAILURE");
    DASMSG_SET(s2, AuthErrorToString(error), "Error reason (from AVS SDK)");
    DASMSG_SEND();
  };
  
  // if not SUCCESS or AUTHORIZATION_PENDING, fail. AUTHORIZATION_PENDING seems like a valid error
  // to me: "Waiting for user to authorize the specified code pair."
  if( (error != Error::SUCCESS) && (error != Error::AUTHORIZATION_PENDING) ) {
    LOG_WARNING( "AlexaImpl.onAuthStateChange.Error", "Alexa authorization experiences error (%d)", (int)error );

    sendResultDas(false, error);

    const bool errFlag = true;
    SetAuthState( AlexaAuthState::Uninitialized, "", "", errFlag );
    return;
  }

  if( _authState == AlexaAuthState::WaitingForCode ) {
    const bool success = (newState == State::REFRESHED);

    sendResultDas(success, error);
  }

  switch( newState ) {
    case State::UNINITIALIZED:
    {
      const bool errFlag = false;
      SetAuthState( AlexaAuthState::Uninitialized, "", "", errFlag );
    }
      break;
    case State::EXPIRED:
    case State::UNRECOVERABLE_ERROR:
    {
      const bool errFlag = true;
      SetAuthState( AlexaAuthState::Uninitialized, "", "", errFlag );
    }
      break;
    case State::REFRESHED:
    {
      const bool errFlag = false;
      SetAuthState( AlexaAuthState::Authorized, "", "", errFlag );
    }
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnRequestAuthorization( const std::string& url, const std::string& code )
{
  const bool errFlag = false;
  SetAuthState( AlexaAuthState::WaitingForCode, url, code, errFlag );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnDialogUXStateChanged( DialogUXState state )
{
  LOG_INFO("AlexaImpl.OnDialogUXStateChanged", "from '%s' to '%s'",
           DialogUXStateToString(_dialogState),
           DialogUXStateToString(state));


  _dialogState = state;
  if( _dialogState != DialogUXState::LISTENING ) {
    _isTapOccurring = false;
  }
  CheckForUXStateChange();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::CheckForUXStateChange()
{
  const auto oldState = _uxState;
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // When playing sequential audio (e.g. flash news briefing) there is often a very short gap between one
  // source ending and the next starting. So, if one is playing, consider it playing, but also consider it
  // playing if it stopped recently
  const bool audioPlaying = _audioActive || (currTime_s - _audioActiveLastChangeTime_s <= kTimeToHoldSpeakingBetweenAudioPlays_s);

  // Consider a timer / alarm active if it's really active or if it's in the background, since background
  // timers will jump to foreground as soon as nothing higher priority (i.e. Dialog) is playing
  const bool alertActive = _alertActive || _backgroundAlertActive;

  const bool anyPlaying = !_playingSources.empty() || alertActive || audioPlaying;
  
  // reset idle timer
  _timeToSetIdle_s = -1.0f;
  
  switch( _dialogState ) {
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::FINISHED:
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE:
    {
      // if nothing is playing, it could be because something is about to play. We only know because a
      // directive for "Play" or "Speak" has arrived. If one has arrived recently, don't switch to idle yet,
      // and instead set a timer so that it will switch to idle only if no other state change occurs.
      // Sometimes, if the delay is long enough, the robot will momentarily break to normal behavior before
      // returning to alexa. Maybe we need a base delay. Note that we also consider this delay if an alerts
      // directive is received, because we somtimes receive the alerts directive before the corresponding TTS
      // (e.g. "Alarm set for 5pm").
      if( anyPlaying ) {
        _uxState = AlexaUXState::Speaking;
      } else if( _lastPlayDirective_s < 0.0f || (currTime_s > _lastPlayDirective_s + kAlexaMaxIdleDelay_s) ) {
        // "Play" wasn't received within kAlexaMaxIdleDelay_s seconds ago. Set to Idle
        _uxState = AlexaUXState::Idle;
      } else if( kAlexaIdleDelay_s > 0.0f ) {
        // wait kAlexaIdleDelay_s before setting to idle
        _timeToSetIdle_s = _lastPlayDirective_s + kAlexaIdleDelay_s;
      } else {
        _uxState = AlexaUXState::Idle;
      }
    }
      break;
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::THINKING:
    {
      _uxState = AlexaUXState::Thinking;
    }
      break;
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::LISTENING:
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::EXPECTING:
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
  
  if( (oldState != _uxState) && (_onAlexaUXStateChanged != nullptr) ) {
    DEV_ASSERT( _uxState != AlexaUXState::Error, "AlexaImpl.CheckForUXStateChange.NoError" );
    _onAlexaUXStateChanged( _uxState );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnSourcePlaybackChange( SourceId id, bool playing )
{
  // TODO:(bn) this could cause "forever face" bugs if it gets out of sync with the actual media player
  // state. This state duplication should be resolved (both are in anim, no need for it)

  if( playing ) {
    auto res = _playingSources.insert( id );
    // not fatal, but it means we don't understand the media player lifecycle correctly
    if( ANKI_VERIFY( res.second, "AlexaImpl.OnSourcePlaybackChange.Exists", "Source %llu was already playing", id ) ) {
      CheckForUXStateChange();
    }
  } else {
    auto it = _playingSources.find( id );
    if( it != _playingSources.end() ) {
      _playingSources.erase( it );
      CheckForUXStateChange();
    } else {
      // this may be ok, depending on whether or not there was an error
      LOG_WARNING( "AlexaImpl.OnSourcePlaybackChange.NotFound", "Source %llu not found", id );
    }
  }

#if ANKI_DEV_CHEATS

  std::stringstream ss;
  for( const auto& sid : _playingSources ) {
    ss << ((int)sid) << ", ";
  }

  LOG_INFO("AlexaImpl.OnSourcePlaybackChange",
           "Source %llu set to %s, %zu are now playing: %s",
           id,
           playing ? "playing" : "not playing",
           _playingSources.size(),
           ss.str().c_str());
#endif

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnInternetConnectionChanged( bool connected )
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING("AlexaImpl.OnInternetConnectionChanged", "Connected=%d", connected);
  _internetConnected = connected;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnAVSConnectionChanged( const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
                                        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason )
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING("AlexaImpl.OnAVSConnectionChanged", "status=%d, reason=%d", status, reason);
  if( status == avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status::CONNECTED ) {
    _avsEverConnected = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnSendComplete( avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status )
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING("AlexaImpl.OnSendComplete", "status '%s'", MessageStatusToString(status));

  DASMSG(send_complete, "alexa.response_to_request", "SDK responded to a sent message");
  DASMSG_SET(s1, MessageStatusToString(status), "AVS-provided message status (e.g. SUCCESS, TIMEDOUT, ...)");
  DASMSG_SEND();

  using Status = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status;
  switch( status ) {
    case Status::PENDING: // The message has not yet been processed for sending.
    case Status::SUCCESS: // The message was successfully sent.
    case Status::SUCCESS_NO_CONTENT: // The message was successfully sent but the HTTPReponse had no content.
      // don't "unset" the error here, since we don't know what the successful message accomplished. it could
      // still be in an error state.
      break;
      
    case Status::NOT_CONNECTED: // The send failed because AVS was not connected.
    case Status::TIMEDOUT: // The send failed because of timeout waiting for AVS response.
    {
      // consider these a lost connection if we were connected
      if( _internetConnected ) {
        OnInternetConnectionChanged(false);
      }

      SetNetworkConnectionError();
      break;
    }

    case Status::CANCELED: // The send failed due to server canceling it before the transmission completed.
    {
      // may have lost connection, or may be a server-side issue (I think...)
      SetNetworkConnectionError();
      break;
    }

    case Status::NOT_SYNCHRONIZED: // The send failed because AVS is not synchronized.
    case Status::PROTOCOL_ERROR: // The send failed due to an underlying protocol error.
    case Status::INTERNAL_ERROR: // The send failed due to an internal error within ACL.
    case Status::SERVER_INTERNAL_ERROR_V2: // The send failed due to an internal error on the server which sends code 500.
    case Status::REFUSED: // The send failed due to server refusing the request.
    case Status::THROTTLED: // The send failed due to excessive load on the server.
    case Status::BAD_REQUEST: // The send failed due to invalid request sent by the user.
    case Status::SERVER_OTHER_ERROR: // The send failed due to unknown server error.
    {
      // sending the last request ended in failure. show the error face and play audio
      SetNetworkError( AlexaNetworkErrorType::HavingTroubleThinking );
      break;
    }
      
    case Status::INVALID_AUTH: // The access credentials provided to ACL were invalid.
    {
      // sending the last request failed because auth was revoke. show the error face and play audio
      SetNetworkError( AlexaNetworkErrorType::AuthRevoked );
      break;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnSDKLogout()
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING( "AlexaImpl.OnLogout", "User logged out" );

  // NOTE:(bn) This doesn't get called if we log out, nor if you de-register from Amazon, not sure when it would
  // get called
  DASMSG(user_sign_out_result,
         "alexa.user_remote_sign_out",
         "User remotely signed out of AVS SDK (unclear when this happens)");
  DASMSG_SEND();

  if( _onLogout != nullptr ) {
    _onLogout();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnNotificationsIndicator( avsCommon::avs::IndicatorState state )
{
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING( "AlexaImpl.OnNotificationsIndicator", "Indicator: %d", (int)state );
  
  const bool changed = (_notificationsIndicator != state);
  if( changed && _onNotificationsChanged ) {
    const bool hasIndicator = (state == avsCommon::avs::IndicatorState::ON);
    _onNotificationsChanged( hasIndicator );
  }
  _notificationsIndicator = state;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnAlertState( const std::string& alertID, capabilityAgents::alerts::AlertObserverInterface::State state )
{
  using State = capabilityAgents::alerts::AlertObserverInterface::State;
  _alertStates[alertID] = state;
  const bool oldAlertActive = _alertActive;
  _alertActive = false;
  _backgroundAlertActive = false;
  for( auto& alertPair : _alertStates ) {
    bool canBeCancelled;
    bool inBackground = false;
    switch( alertPair.second ) {
      case State::STARTED: // The alert has started.
      case State::FOCUS_ENTERED_FOREGROUND: // The alert has entered the foreground.
        canBeCancelled = true;
        break;

      case State::FOCUS_ENTERED_BACKGROUND: // The alert has entered the background.
        inBackground = true;

        // fall through
      case State::READY: // The alert is ready to start, and is waiting for channel focus.
      case State::STOPPED: // The alert has stopped due to user or system intervention.
      case State::SNOOZED: // The alert has snoozed.
      case State::COMPLETED: // The alert has completed on its own.
      case State::PAST_DUE: // The alert has been determined to be past-due, and will not be rendered.
      case State::ERROR: // The alert has encountered an error.
        canBeCancelled = false;
        break;
    }
    _alertActive |= canBeCancelled;
    _backgroundAlertActive |= inBackground;
  }
  // TODO (VIC-11517): downgrade. for now this is useful in webots
  LOG_WARNING( "AlexaImpl.OnAlertState",
               "alert '%s' changed to state '%s', _alertActive=%d, _backgroundAlertActive=%d, %zu alerts tracked",
               alertID.c_str(),
               AlertStateToString(state),
               _alertActive,
               _backgroundAlertActive,
               _alertStates.size());
  if( oldAlertActive != _alertActive ) {
    // note: this is ok to only have two options, not three (e.g., "unknown") since _alertActive
    // is initialized as false, in which case if the initial assignment to _alertActive is false, we don't
    // need to call CheckForUXStateChange anyway
    CheckForUXStateChange();
  }

  // TODO:(bn) clean up / consolidate this logic with above
  switch( state ) {
    case State::STARTED: // The alert has started.
    case State::FOCUS_ENTERED_FOREGROUND: // The alert has entered the foreground.
    case State::FOCUS_ENTERED_BACKGROUND: // The alert has entered the background.
    case State::READY: // The alert is ready to start, and is waiting for channel focus.
    case State::SNOOZED: // The alert has snoozed.
      break;


    case State::PAST_DUE: // The alert has been determined to be past-due, and will not be rendered.
    case State::ERROR: // The alert has encountered an error.
    case State::COMPLETED: // The alert has completed on its own.
    case State::STOPPED: // The alert has stopped due to user or system intervention.
    {
      const size_t numErased = _alertStates.erase(alertID);
      ANKI_VERIFY(numErased == 1,
                  "AlexaImpl.OnAlertState.BadAlertRemove",
                  "Erase of alert %s removed %zu elements",
                  alertID.c_str(),
                  numErased);
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnPlayerActivity( alexaClientSDK::avsCommon::avs::PlayerActivity state )
{
  const bool playing = ( state == alexaClientSDK::avsCommon::avs::PlayerActivity::PLAYING );
  if( playing != _audioActive ) {
    _audioActive = playing;
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _audioActiveLastChangeTime_s = currTime_s;

    LOG_INFO( "AlexaImpl.OnPlayerActivity", "new state = %s",
              alexaClientSDK::avsCommon::avs::playerActivityToString(state).c_str() );

    CheckForUXStateChange();
    // call CheckForUXStateChange() again once kTimeToHoldSpeakingBetweenAudioPlays_s expires
    // todo: merge _nextUXStateCheckTime_s logic into _timeToSetIdle_s
    _nextUXStateCheckTime_s = std::max(_nextUXStateCheckTime_s, currTime_s + kTimeToHoldSpeakingBetweenAudioPlays_s + 0.1f);
    
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code, bool errFlag )
{
  // always send WaitingForCode in case url or code changed
  const bool changed = (_authState != state) || (state == AlexaAuthState::WaitingForCode);
  _authState = state;
  if( (_onAlexaAuthStateChanged != nullptr) && (changed || errFlag) ) {
    _onAlexaAuthStateChanged( state, url, code, errFlag );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::NotifyOfTapToTalk()
{
  if( _client != nullptr ) {
    if( !_isTapOccurring ) {
      _client->StopForegroundActivity();
      // check info known about connection before trying. these often don't get updated until sending fails
      if( !_client->IsAVSConnected() ) {
        SetNetworkConnectionError();
      } else {
        if( !_alertActive ) {
          ANKI_VERIFY( _client->NotifyOfTapToTalk( *_tapToTalkAudioProvider ).get(),
                       "AlexaImpl.NotifyOfTapToTalk.Failed",
                       "Failed to notify tap to talk" );
          _isTapOccurring = true;
        }
      }
    } else {
      _client->NotifyOfTapToTalkEnd();
      _isTapOccurring = false;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::NotifyOfWakeWord( uint64_t fromIndex, uint64_t toIndex )
{
  
  if( !_client->IsAVSConnected() ) {
    // run SetNetworkConnectionError on main thread
    _runSetNetworkConnectionError = true;
    return;
  }

#if ANKI_DEV_CHEATS
  if( _debugMicrophone ) {
    const uint64_t totalNumSamples = _debugMicrophone->GetTotalNumSamples();

    static int sAudioDumpIdx = 0;
    const std::string filename = _alexaCacheFolder + "audioInput_" + std::to_string(sAudioDumpIdx++) + ".pcm";
    std::ofstream fileOut;
    std::ios_base::openmode mode = std::ios::out | std::ofstream::binary;
    fileOut.open(filename,mode);
    if( fileOut.is_open() ) {
      const size_t buffSizebytes = 1024 * 1024;
      char buf[buffSizebytes] = {0};

      // TODO:(bn) move this code to a thread or dispatch queue?
      auto reader = _debugMicrophone->GetReader();
      auto wordSize = reader->getWordSize();
      auto numWords = buffSizebytes / wordSize;
      size_t totalNumRead = 0;

      ssize_t numRead = 1;
      BOUNDED_WHILE ( 1000, numRead > 0 ) {
        numRead = reader->read(buf, numWords);
        if( numRead <= 0 ) {
          break;
        }
        totalNumRead += numRead;
        fileOut.write(buf, numRead * wordSize);
      }
      fileOut.flush();
      fileOut.close();

      // "absolute" index is total bytes ever, not the current size of the ring buffer
      const uint64_t debugEndIdx =  totalNumRead - (totalNumSamples - toIndex);
      const uint64_t debugStartIdx =  debugEndIdx - (toIndex - fromIndex);
      LOG_INFO("AlexaImpl.WroteAudioInput",
               "Wrote mic data to file '%s'. Trigger range %llu to %llu",
               filename.c_str(),
               debugStartIdx,
               debugEndIdx);
    }
  }
#endif
  
  // this can generate SdsReader:seekFailed:reason=seekOverwrittenData if the time indices contain overwritten data
  _keywordObserver->onKeyWordDetected( _wakeWordAudioProvider->stream, "ALEXA", fromIndex, toIndex, nullptr);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::RemoveCallbacksForShutdown()
{
  if( _observer ) {
    _observer->Shutdown();
  }
  
  if( _initThread.joinable() ) {
     // shut down init thread in prep for destruction since destruction could occur on a thread
    _initThread.join();
    _initThread = {};
  }
  
  _onAlexaAuthStateChanged = nullptr;
  _onAlexaUXStateChanged = nullptr;
  _onLogout = nullptr;
  _onNetworkError = nullptr;
  _onNotificationsChanged = nullptr;
  _initCompleteCallback = nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion AlexaImpl::GetFirmwareVersion() const
{
  int major = 0;
  int minor = 0;
  int incremental = 0;
  int build = 0;
  OSState::getInstance()->GetOSBuildVersion(major, minor, incremental, build);
  // reversible, simpler than crc, but obviously might not be unique and may overflow
  std::string concatVersion = std::to_string(major)
                              + std::to_string(minor)
                              + std::to_string(incremental)
                              + std::to_string(build);
  // make sure it is < 2147483647 by cropping any trailing digits
  if( concatVersion.size() > 9 ) {
    concatVersion = concatVersion.substr(0,9);
  }
  avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion version = std::atoi(concatVersion.c_str());
  return version;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::SetNetworkConnectionError()
{
  if( _internetConnected ) {
    // the AVS internet connection check is slow, so let's double check by getting our IP address
    const bool updateIPCheck = true;
    const std::string ip = OSState::getInstance()->GetIPAddress(updateIPCheck);
    const bool hasIP = !ip.empty();
    LOG_INFO("AlexaImpl.SetNetworkConnectionError.IPCheck", "ip addr: '%s'", ip.c_str());

    if( !hasIP ) {
      // the internet connection check is pretty slow, and we don't have an IP, so we definitely don't have
      // internet. Update the status here
      OnInternetConnectionChanged( hasIP );
    }
  }
  // note that we might have a (local) IP address, but no internet, so don't set us as connected based on the
  // IP check

  // check if it's because of the internet (this flag only gets updated every 5 mins, so it gets checked second)
  if( _internetConnected ) {
    LOG_INFO("AlexaImpl.SetNetworkConnectionError.Unconnected.HavingTrouble",
             "AVS not conntected, but internet reported as being connected");
    SetNetworkError( AlexaNetworkErrorType::HavingTroubleThinking );
  } else if( _avsEverConnected ) {
    LOG_INFO("AlexaImpl.SetNetworkConnectionError.Unconnected.LostConnection",
             "AVS not conntected, internet was but is no longer");
    SetNetworkError( AlexaNetworkErrorType::LostConnection );
  } else {
    LOG_INFO("AlexaImpl.SetNetworkConnectionError.Unconnected.NoInitialConnection",
             "AVS not conntected, internet never was");
    SetNetworkError( AlexaNetworkErrorType::NoInitialConnection );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::SetNetworkError( AlexaNetworkErrorType errorType )
{
  // NOTE: while a network error is a user-facing error, and it _should_ change the AlexaUXState, AlexaImpl is agnostic
  // that that ux state. Instead, AlexaUXState::Error is considered only by the parent, Alexa. This is because we
  // need to play audio for the duration of that state, and AlexaImpl may get destroyed if the error involves
  // authentication.
  if( _onNetworkError != nullptr ) {
    _onNetworkError( errorType );
  }
}
  
#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::ConfirmShutdown()
{
  _shutdownChecker.PrintRemaining();
  AlexaClient::ConfirmShutdown();
}
#endif

} // namespace Vector
} // namespace Anki
