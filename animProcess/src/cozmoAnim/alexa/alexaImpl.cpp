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
#include "cozmoAnim/alexa/alexaMediaPlayer.h"
#include "cozmoAnim/alexa/alexaObserver.h"
#include "cozmoAnim/alexa/alexaRevokeAuthObserver.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
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
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/SQLiteSettingStorage.h>
#include <ESP/DummyESPDataProvider.h>

#include <chrono>

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
  // Conversion factor from milliseconds to samples
  static const unsigned int kMillisToSamples = kSampleRate_Hz / 1000;
  
  const std::string kDirectivesFile = "directives.txt";
  const std::string kAlexaFolder = "alexa";
  
  // If the DialogUXState is set to Idle, but a directive was received to Play, then wait this long before
  // setting to idle
  CONSOLE_VAR_RANGED(float, kAlexaIdleDelay_s, "Alexa", 2.0f, 0.0f, 10.0f);
  // If the DialogUXState was set to Idle, but the last Play directive was older than this amount, then
  // switch to Idle
  CONSOLE_VAR_RANGED(float, kAlexaMaxIdleDelay_s, "Alexa", 3.0f, 0.0f, 10.0f);
  
  CONSOLE_VAR(bool, kLogAlexaDirectives, "Alexa", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::AlexaImpl()
  : _authState{ AlexaAuthState::Uninitialized }
  , _uxState{ AlexaUXState::Idle }
  , _dialogState{ DialogUXState::IDLE }
  , _notificationsIndicator{ avsCommon::avs::IndicatorState::UNDEFINED }
{
  static_assert( std::is_same<SourceId, avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId>::value,
                 "Our shorthand SourceId type differs" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::~AlexaImpl()
{
  if( _capabilitiesDelegate ) {
    // TODO (VIC-11427): this is likely leaking something when commented out, but running shutdown() causes a lock
    //_capabilitiesDelegate->shutdown();
  }
  
  // First clean up anything that depends on the the MediaPlayers.
  // This would be a userInputManager or interactionManager, for instance.
  // We don't have those yet.
  
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
bool AlexaImpl::Init( const AnimContext* context )
{
  _context = context;
  
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
  
  auto configs = GetConfigs();
  if( configs.empty() ) {
    return false;
  }
  
  // TODO: docs say this should be called before any other threads start. I'm guessing theyre talking
  // about alexa threads.
  if( !avsCommon::avs::initialization::AlexaClientSDKInit::initialize( configs ) ) {
    CRITICAL_SDK("Failed to initialize SDK!");
    return false;
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
                   std::bind(&AlexaImpl::OnNotificationsIndicator, this, std::placeholders::_1) );
  
  const auto& rootConfig = avsCommon::utils::configuration::ConfigurationNode::getRoot();
  
  // Creating customerDataManager which will be used by the registrationManager and all classes that extend
  // CustomerDataHandler
  // TODO (VIC-9837) see what else this baby can do
  auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
  
  // Creating the deviceInfo object
  std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create( rootConfig );
  if( !deviceInfo ) {
    CRITICAL_SDK("Creation of DeviceInfo failed!");
    return false;
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
    return false;
  }
  
  // Create HTTP Put handler
  std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPut> httpPut = avsCommon::utils::libcurlUtils::HttpPut::create();
  
  if( !authDelegate ) {
    CRITICAL_SDK("Creation of AuthDelegate failed!");
    return false;
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
    return false;
  }
  _capabilitiesDelegate->addCapabilitiesObserver( _observer );
  
  auto messageStorage = certifiedSender::SQLiteMessageStorage::create( rootConfig );
  
  auto notificationsStorage
    = capabilityAgents::notifications::SQLiteNotificationsStorage::create( rootConfig );
  
  // Creating the alert storage object to be used for rendering and storing alerts.
  auto audioFactory = std::make_shared<applicationUtilities::resources::audio::AudioFactory>();
  auto alertStorage
    = capabilityAgents::alerts::storage::SQLiteAlertStorage::create( rootConfig,
                                                                     audioFactory->alerts() );
  // setup media player interfaces
  auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
  _ttsMediaPlayer = std::make_shared<AlexaMediaPlayer>( avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
                                                        "TTS",
                                                        httpContentFetcherFactory );
  _ttsMediaPlayer->Init(context);
  _alertsMediaPlayer = std::make_shared<AlexaMediaPlayer>( avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
                                                           "Alerts",
                                                           httpContentFetcherFactory );
  _alertsMediaPlayer->Init( context );
  _audioMediaPlayer = std::make_shared<AlexaMediaPlayer>( avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
                                                          "Audio",
                                                          httpContentFetcherFactory );
  _audioMediaPlayer->Init( context );
  _notificationsMediaPlayer = std::make_shared<AlexaMediaPlayer>( avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
                                                                  "Notifications",
                                                                  httpContentFetcherFactory );
  _notificationsMediaPlayer->Init( context );
  
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
    return false;
  }
  
  const avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion = GetFirmwareVersion();
  
  
  _client = AlexaClient::Create( deviceInfo,
                                 customerDataManager,
                                 authDelegate,
                                 std::move(messageStorage),
                                 std::move(alertStorage),
                                 std::move(notificationsStorage),
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
    return false;
  }
  
  _client->SetDirectiveCallback( std::bind(&AlexaImpl::OnDirective, this, std::placeholders::_1, std::placeholders::_2) );
  
  _client->AddSpeakerManagerObserver( _observer );
  _client->AddInternetConnectionObserver( _observer );
  _client->AddNotificationsObserver( _observer );
  
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
    return false;
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
  
  _capabilitiesDelegate->addCapabilitiesObserver( _client );
  
  // try connecting
  _client->Connect( _capabilitiesDelegate );
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Update()
{
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
  if( _timeToSetIdle_s >= 0.0f && currTime_s >= _timeToSetIdle_s && _playingSources.empty() ) {
    _dialogState = DialogUXState::IDLE;
    CheckForUXStateChange();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::Logout()
{
  if( _client ) {
    auto regManager = _client->GetRegistrationManager();
    if( regManager ) {
      regManager->logout();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::StopForegroundActivity()
{
  if( _client ) {
    _client->StopForegroundActivity();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples )
{
  if( _microphone != nullptr ) {
    _microphone->AddSamples( samples, nSamples );
  }
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
  if( directive == "Play" || directive == "Speak" ) {
    _lastPlayDirective_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  // TODO: is there a Stop directive we can listed to? that should force the Idle ux state or at least reset idle timers
  // "StopCapture" might be it. needs further investigation to see how it overlaps with music audio
  
  if( kLogAlexaDirectives ) {
    Json::Value json;
    Json::Reader reader;
    bool success = reader.parse( payload, json );
    if( success ) {
      const bool append = true;
      Util::FileUtils::WriteFile( _alexaCacheFolder + kDirectivesFile, directive + ": " + json.toStyledString(), append );
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
  
  // if not SUCCESS or AUTHORIZATION_PENDING, fail. AUTHORIZATION_PENDING seems like a valid error
  // to me: "Waiting for user to authorize the specified code pair."
  if( (error != Error::SUCCESS) && (error != Error::AUTHORIZATION_PENDING) ) {
    LOG_ERROR( "AlexaImpl.onAuthStateChange.Error", "Alexa authorization experiences error (%d)", (int)error );
    const bool errFlag = true;
    SetAuthState( AlexaAuthState::Uninitialized, "", "", errFlag );
    return;
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
  const bool anyPlaying = !_playingSources.empty();
  
  // reset idle timer
  _timeToSetIdle_s = -1.0f;
  
  switch( _dialogState ) {
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::FINISHED:
    case avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState::IDLE:
    {
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      // if nothing is playing, it could be because something is about to play. We only know because
      // a directive for "Play" or "Speak" has arrived. If one has arrived recently, don't switch to idle yet,
      // and instead set a timer so that it will switch to idle only if no other state change occurs.
      // Sometimes, if the delay is long enough, the robot will momentarily break to normal behavior before
      // returning to alexa. Maybe we need a base delay
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
  if( playing ) {
    auto res = _playingSources.insert( id );
    // not fatal, but it means we don't understand the media player lifecycle correctly
    if( ANKI_VERIFY( res.second, "AlexaImpl.OnSourcePlaybackChange.Exists", "Source %llu was already playing", id ) ) {
      CheckForUXStateChange();
    }
  } else {
    auto it = _playingSources.find( id );
    if( ANKI_VERIFY( it != _playingSources.end(),
                     "AlexaImpl.OnSourcePlaybackChange.NotFound", "Source %llu not found", id ) )
    {
      _playingSources.erase( it );
      CheckForUXStateChange();
    }
  }
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
  LOG_WARNING("AlexaImpl.OnSendComplete", "status=%d", status);
  using Status = avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status;
  switch( status ) {
    case Status::PENDING: // The message has not yet been processed for sending.
    case Status::SUCCESS: // The message was successfully sent.
    case Status::SUCCESS_NO_CONTENT: // The message was successfully sent but the HTTPReponse had no content.
      // don't "unset" the error here, since we don't know what the successful message accomplished. it could
      // still be in an error state.
      break;
      
    case Status::NOT_CONNECTED: // The send failed because AVS was not connected.
    case Status::NOT_SYNCHRONIZED: // The send failed because AVS is not synchronized.
    case Status::TIMEDOUT: // The send failed because of timeout waiting for AVS response.
    case Status::PROTOCOL_ERROR: // The send failed due to an underlying protocol error.
    case Status::INTERNAL_ERROR: // The send failed due to an internal error within ACL.
    case Status::SERVER_INTERNAL_ERROR_V2: // The send failed due to an internal error on the server which sends code 500.
    case Status::REFUSED: // The send failed due to server refusing the request.
    case Status::CANCELED: // The send failed due to server canceling it before the transmission completed.
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
      // check info known about connection before trying. these often don't get updated until sending fails
      if( !_client->IsAVSConnected() ) {
        // check if it's because of the internet (this flag only gets updated every 5 mins, so it gets checked second)
        if( _internetConnected ) {
          SetNetworkError( AlexaNetworkErrorType::HavingTroubleThinking );
        } else if( _avsEverConnected ) {
          SetNetworkError( AlexaNetworkErrorType::LostConnection );
        } else {
          SetNetworkError( AlexaNetworkErrorType::NoInitialConnection );
        }
      } else {
        ANKI_VERIFY( _client->NotifyOfTapToTalk( *_tapToTalkAudioProvider ).get(),
                     "AlexaImpl.NotifyOfTapToTalk.Failed",
                     "Failed to notify tap to talk" );
        _isTapOccurring = true;
      }
    } else {
      _client->NotifyOfTapToTalkEnd();
      _isTapOccurring = false;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::NotifyOfWakeWord( long from_ms, long to_ms )
{
  
  if( !_client->IsAVSConnected() ) {
    // check if it's because of the internet (this flag only gets updated every 5 mins, so it gets checked second)
    if( _internetConnected ) {
      SetNetworkError( AlexaNetworkErrorType::HavingTroubleThinking );
    } else if( _avsEverConnected ) {
      SetNetworkError( AlexaNetworkErrorType::LostConnection );
    } else {
      SetNetworkError( AlexaNetworkErrorType::NoInitialConnection );
    }
    
    return;
  }
  
    
  avsCommon::avs::AudioInputStream::Index fromIndex;
  avsCommon::avs::AudioInputStream::Index toIndex;
  
  const int kWindowHalfBuffer = 100;
  
  if ( from_ms < 0 ) {
    fromIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;
  } else {
    fromIndex = from_ms; // first move into larger type
    fromIndex = kMillisToSamples*fromIndex; // milliseconds to samples @ 16k audio
    if( fromIndex > kWindowHalfBuffer ) {
      fromIndex -= kWindowHalfBuffer;
    }
  }
  
  const size_t totalNumSamples = _microphone->GetTotalNumSamples();
  
  if ( to_ms < 0 ) {
    toIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;
  } else {
    toIndex = to_ms;
    toIndex = kMillisToSamples*toIndex + kWindowHalfBuffer;
    if( toIndex > totalNumSamples ) {
      toIndex = totalNumSamples;
    }
  }
  
  // NOTE: this only works if our extra VAD is off, since otherwise the mic samples wont match the sample indices provided by sensory.
  // To get around this, we simply take the size (in # samples) of the window and
  // back up from the most recent sample by that amount. It seems to work...
  if ( from_ms >= 0 && to_ms >= 0 ) {
    avsCommon::avs::AudioInputStream::Index dIndex = toIndex - fromIndex;
    toIndex = totalNumSamples;
    fromIndex = toIndex - dIndex;
  }
  
  // this can generate SdsReader:seekFailed:reason=seekOverwrittenData if the time indices contain overwritten data
  _keywordObserver->onKeyWordDetected( _wakeWordAudioProvider->stream, "ALEXA", fromIndex, toIndex, nullptr);
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

} // namespace Vector
} // namespace Anki
