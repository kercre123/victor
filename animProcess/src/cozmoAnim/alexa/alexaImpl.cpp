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

#include "clad/types/alexaAuthState.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "cozmoAnim/alexa/alexaAudioInput.h"
#include "cozmoAnim/alexa/alexaClient.h"
#include "cozmoAnim/alexa/alexaMediaPlayer.h"
#include "cozmoAnim/alexa/alexaObserver.h"
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
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <ContextManager/ContextManager.h>
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::AlexaImpl()
  : _currAuthState{ AlexaAuthState::Uninitialized }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaImpl::~AlexaImpl()
{
  if( _capabilitiesDelegate ) {
    // TODO: this is likely leaking something when commented out, but running shutdown() causes a lock
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
  
  avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaImpl::Init(const AnimContext* context)
{
  _context = context;
  
  const auto* dataPlatform = _context->GetDataPlatform();
  _alexaFolder = dataPlatform->pathToResource( Util::Data::Scope::Persistent, "alexa" );
  
  if( !_alexaFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _alexaFolder ) ) {
    Util::FileUtils::CreateDirectory( _alexaFolder );
  }
  
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
                   std::bind(&AlexaImpl::OnAuthStateChange, this, std::placeholders::_1, std::placeholders::_2) );
  
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
  std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage
    = alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(rootConfig);
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
    = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create( authDelegate,
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
  
  auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create( rootConfig );
  
  // Creating the alert storage object to be used for rendering and storing alerts.
  auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
  auto alertStorage
    = alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage::create( rootConfig,
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
  
  // the alerts/audio speakers dont change UX so we observe them
  _alertsMediaPlayer->setObserver( _observer );
  _audioMediaPlayer->setObserver( _observer );
  
  
  _client = AlexaClient::Create( deviceInfo,
                                 customerDataManager,
                                 authDelegate,
                                 std::move(messageStorage),
                                 std::move(alertStorage),
                                 audioFactory,
                                 {_observer},
                                 {_observer},
                                 _capabilitiesDelegate,
                                 _ttsMediaPlayer,
                                 _alertsMediaPlayer,
                                 _audioMediaPlayer,
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_ttsMediaPlayer),
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_alertsMediaPlayer),
                                 std::static_pointer_cast<avsCommon::sdkInterfaces::SpeakerInterface>(_audioMediaPlayer) );
  
  if( !_client ) {
    CRITICAL_SDK("Failed to create SDK client!");
    return false;
  }
  
  _client->SetDirectiveCallback( std::bind(&AlexaImpl::OnDirective, this, std::placeholders::_1, std::placeholders::_2) );
  
  _client->AddSpeakerManagerObserver( _observer );

  // Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
  size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize( kBufferSize,
                                                                                             kWordSize,
                                                                                             kMaxReaders );
  auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>( bufferSize );
  std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream
    = alexaClientSDK::avsCommon::avs::AudioInputStream::create( buffer, kWordSize, kMaxReaders );

  if( !sharedDataStream ) {
    CRITICAL_SDK("Failed to create shared data stream!");
    return false;
  }

  alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
  compatibleAudioFormat.sampleRateHz = kSampleRate_Hz;
  compatibleAudioFormat.sampleSizeInBits = kWordSize * CHAR_BIT;
  compatibleAudioFormat.numChannels = kNumChannels;
  compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
  compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

  // Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
  // of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
  // stream is used since this sample application will only have one microphone.

  const auto kNearField = alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD;
  
  {
    // Creating tap to talk audio provider
    bool tapAlwaysReadable = true;
    bool tapCanOverride = true;
    bool tapCanBeOverridden = true;
    _tapToTalkAudioProvider
      = std::make_shared<alexaClientSDK::capabilityAgents::aip::AudioProvider>( sharedDataStream,
                                                                                compatibleAudioFormat,
                                                                                kNearField,
                                                                                tapAlwaysReadable,
                                                                                tapCanOverride,
                                                                                tapCanBeOverridden );
  }

  // TODO: create a wakeword provider
  //{
  //  bool wakeAlwaysReadable = true;
  //  bool wakeCanOverride = false;
  //  bool wakeCanBeOverridden = true;
  //
  //
  //  _wakeWordAudioProvider = std::make_shared<capabilityAgents::aip::AudioProvider>( sharedDataStream,
  //                                                                                   compatibleAudioFormat,
  //                                                                                   kNearField,
  //                                                                                   wakeAlwaysReadable,
  //                                                                                   wakeCanOverride,
  //                                                                                   wakeCanBeOverridden );
  //
  //  auto dummyEspProvider = std::make_shared<esp::DummyESPDataProvider>();
  //  std::shared_ptr<esp::ESPDataProviderInterface> espProvider = dummyEspProvider;
  //  std::shared_ptr<esp::ESPDataModifierInterface> espModifier = dummyEspProvider;
  //  // This observer is notified any time a keyword is detected and notifies the AlexaClient to start recognizing.
  //  _keywordObserver = std::make_shared<KeywordObserver>( _client, *_wakeWordAudioProvider, espProvider );
  //}
  
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
  
  const std::string pathToPersistent = dataPlatform->pathToResource( Util::Data::Scope::Persistent, "alexa" );
  Util::StringReplace( configJson, "<ALEXA_STORAGE_PATH>", pathToPersistent );
  
  configJsonStreams.push_back( std::shared_ptr<std::istringstream>( new std::istringstream( configJson ) ) );
  return configJsonStreams;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnDirective(const std::string& directive, const std::string& payload)
{
  // todo: this will be needed for timer, and is also useful for debugging
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnAuthStateChange( avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
                                   avsCommon::sdkInterfaces::AuthObserverInterface::Error error )
{
  using State = avsCommon::sdkInterfaces::AuthObserverInterface::State;
  using Error = avsCommon::sdkInterfaces::AuthObserverInterface::Error;
  
  // if not SUCCESS or AUTHORIZATION_PENDING, fail. AUTHORIZATION_PENDING seems like a valid error
  // to me: "Waiting for user to authorize the specified code pair."
  if( (error != Error::SUCCESS) && (error != Error::AUTHORIZATION_PENDING) ) {
    LOG_ERROR( "AlexaImpl.onAuthStateChange.Error", "Alexa authorization experiences error (%d)", (int)error );
    SetAuthState( AlexaAuthState::Uninitialized, "", "" );
    return;
  }
  
  switch( newState ) {
    case State::UNINITIALIZED:
    case State::EXPIRED:
    case State::UNRECOVERABLE_ERROR:
    {
      SetAuthState( AlexaAuthState::Uninitialized, "", "" );
    }
      break;
    case State::REFRESHED:
    {
      SetAuthState( AlexaAuthState::Authorized, "", "" );
    }
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnRequestAuthorization( const std::string& url, const std::string& code )
{
  SetAuthState( AlexaAuthState::WaitingForCode, url, code );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::OnDialogUXStateChanged( avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState state )
{
  // TODO: will be needed for engine
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code )
{
  // always send WaitingForCode in case url or code changed
  const bool changed = (_currAuthState != state) || (state == AlexaAuthState::WaitingForCode);
  _currAuthState = state;
  if( (_onAlexaAuthStateChanged != nullptr) && changed ) {
    _onAlexaAuthStateChanged( state, url, code );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaImpl::NotifyOfTapToTalk()
{
  if( _client != nullptr ) {
    ANKI_VERIFY( _client->NotifyOfTapToTalk( *_tapToTalkAudioProvider ).get(),
                 "AlexaImpl.NotifyOfTapToTalk.Failed",
                 "Failed to notify tap to talk" );
  }
}

} // namespace Vector
} // namespace Anki
