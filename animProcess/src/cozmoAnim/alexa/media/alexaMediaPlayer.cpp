/**
 * File: alexaMediaPlayer.cpp
 *
 * Author: ross
 * Created: Oct 20 2018
 *
 * Description: A streaming mp3 decoder for Alexa's voice and other audio.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

/***************************************************************************************************
NOTE:  This is shitty prototype code. I'm putting it in master so that Alexa will start speaking sooner.
TODO (VIC-9853): re-implement this properly. I think it should more closely resemble MediaPlayer.h/cpp,
                 and there should not be a thread::sleep
*************************************************************************************************/

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


#include "cozmoAnim/alexa/media/alexaMediaPlayer.h"

#include "attachmentReader.h"
#include "streamReader.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/container/ringBuffContiguousRead.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"
#include "util/logging/DAS.h"
#include "util/math/math.h"

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/SDS/ReaderPolicy.h>

#include <mpg123.h>

// for saving PCM to disk
#include <fcntl.h>
#include <unistd.h>
#include <array>

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;

using SourceId = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;
std::atomic<SourceId> AlexaMediaPlayer::_nextAvailableSourceID{1}; // 0 is invalid

using PluginId_t = Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t;

struct AudioInfo {
  AudioEngine::AudioGameObject gameObject;
  PluginId_t pluginId;
  alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type speakerType;
  std::string name;

  AudioMetaData::GameEvent::GenericEvent playEvent;
  AudioMetaData::GameEvent::GenericEvent pauseEvent;
  AudioMetaData::GameEvent::GenericEvent resumeEvent;
  AudioMetaData::GameParameter::ParameterType volumeParameter;
};

namespace {
  constexpr int kAudioBufferSize = 1 << 16;

  constexpr size_t kDownloadSize = 8192;
  constexpr float  kIdealBufferSize_sec = 2.0f;
  constexpr float  kMinBufferSize_sec = 2.5f;
  
  // assume the input stream is an unsupported format and abort streaming after this many bytes are
  // processed. this is arbitrary based on how much header info you'd actually expect in an mp3. Sometimes
  // sources may provide large headers with their copyright blurbs, but probably not even this much.
  // Note that if the stream ends prior to downloading this many bytes, we already exit cleanly.
  constexpr size_t kMaxInvalidBytesBeforeAbort = 512*1024; // 512 kB
  constexpr size_t kMaxInvalidFramesBeforeAbort = 5;

  constexpr int kMaxPlayTries = 20;

  // define wwise objects depending on player type
  constexpr PluginId_t kPluginIdTTS = 10;
  constexpr PluginId_t kPluginIdAudio = 11;
  constexpr PluginId_t kPluginIdAlerts = 12;
  constexpr PluginId_t kPluginIdNotifications = 13;

  const std::unordered_map<AlexaMediaPlayer::Type, AudioInfo> sAudioInfo{
    {AlexaMediaPlayer::Type::TTS,
      {AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::AlexaVoice),
      kPluginIdTTS,
      alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
      "TTS",
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Voice_Play,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Voice_Pause,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Voice_Resume,
      AudioMetaData::GameParameter::ParameterType::Robot_Alexa_Volume_Master}},
    {AlexaMediaPlayer::Type::Audio,
      {AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::AlexaMedia),
      kPluginIdAudio,
      alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
      "Audio",
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Media_Play,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Media_Pause,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Media_Resume,
      AudioMetaData::GameParameter::ParameterType::Robot_Alexa_Volume_Master}},
    {AlexaMediaPlayer::Type::Alerts,
      {AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::AlexaAlerts),
      kPluginIdAlerts,
      alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
      "Alerts",
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Alerts_Play,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Alerts_Pause,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Alerts_Resume,
      AudioMetaData::GameParameter::ParameterType::Robot_Alexa_Volume_Alerts}},
    {AlexaMediaPlayer::Type::Notifications,
      {AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::AlexaNotifications),
      kPluginIdNotifications,
      alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
      "Notifications",
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Notifications_Play,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Notifications_Pause,
      AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Alexa__External_Notifications_Resume,
      AudioMetaData::GameParameter::ParameterType::Robot_Alexa_Volume_Master}}
  };

  const char* kSaveSettings_VolumeKey     = "volume";
  const char* kSaveSettings_MuteKey       = "mute";

  #define LOG_CHANNEL "Alexa"
  #define LOG(x, ...) LOG_INFO("AlexaMediaPlayer.SpeakerInfo", "%s: " x, _audioInfo.name.c_str(), ##__VA_ARGS__)
#if ANKI_DEV_CHEATS
  CONSOLE_VAR(bool, kSaveAlexaAudio, "Alexa", false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMediaPlayer::AlexaMediaPlayer( Type type,
                                    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory )
  : avsCommon::utils::RequiresShutdown{"AlexaMediaPlayer_" + sAudioInfo.at(type).name}
  , _type( type )
  , _state(State::Idle)
  , _stateBeforePause(State::Idle)
  , _playableClip( AudioEngine::kInvalidAudioEventId )
  , _mp3Buffer( new Util::RingBuffContiguousRead<uint8_t>{ kAudioBufferSize, kMaxReadSize } )
  , _dataValidity{ DataValidity::Unknown }
  , _dispatchQueue(Util::Dispatch::Create(("APlayer_" + sAudioInfo.at(type).name).c_str()))
  , _contentFetcherFactory( contentFetcherFactory )
  , _audioInfo( sAudioInfo.at(_type) )
  , _shuttingDown( false )
  , _playLoopRunning( false )
{
 // FIXME: How do we get the inital state from persistant storage
  _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  _settings.mute = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMediaPlayer::~AlexaMediaPlayer()
{
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
  
  if( _decoderHandle != nullptr ) {
    mpg123_delete( _decoderHandle );
  }
  mpg123_exit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::Init( const Anim::AnimContext* context )
{
  DEV_ASSERT(nullptr != context, "AudioMediaPlayer.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "AudioMediaPlayer.InvalidAudioController");

  _audioController  = context->GetAudioController();
  DEV_ASSERT_MSG( _audioController != nullptr, "AudioMediaPlayer.Init.InvalidAudioController", "" );

  mpg123_init();
  int initRet;
  _decoderHandle = mpg123_new(NULL, &initRet);
  if( _decoderHandle == nullptr ) {
    LOG_ERROR( "AlexaMediaPlayer.Init.DecoderFail", "Unable to create mpg123 handle: %s", mpg123_plain_strerror(initRet));
    return;
  }
  
  // resync all the way up to the end of the stream
  mpg123_param( _decoderHandle, MPG123_RESYNC_LIMIT, -1, 0 );
  // use a small read-ahead buffer for frame syncing
  mpg123_param( _decoderHandle, MPG123_ADD_FLAGS, MPG123_SEEKBUFFER, 0 );
  // Note: call mpg123_param( _decoderHandle, MPG123_VERBOSE, 2, 0) to enable verbose logging
  
  // in dev, remove any debug audio files from last time
  const Util::Data::DataPlatform* platform = context->GetDataPlatform();
  if( platform != nullptr ) {
    _cacheSaveFolder = platform->pathToResource( Util::Data::Scope::Cache, "alexa" );
    _persistentSaveFolder = platform->pathToResource( Util::Data::Scope::Persistent, "alexa" );

    _cacheSaveFolder = Util::FileUtils::AddTrailingFileSeparator( _cacheSaveFolder );
    _persistentSaveFolder = Util::FileUtils::AddTrailingFileSeparator( _cacheSaveFolder );

    if( !_cacheSaveFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _cacheSaveFolder ) ) {
      Util::FileUtils::CreateDirectory( _cacheSaveFolder );
    }
    DEV_ASSERT(Util::FileUtils::DirectoryExists( _persistentSaveFolder ), "AudioMediaPlayer.Init.NoPersistentSaveFolder");

#if ANKI_DEV_CHEATS
    for( int i=1; i<100; ++i ) {
      std::string ss = _cacheSaveFolder + "speaker_" + _audioInfo.name + std::to_string(i) + ".mp3";
      if( Util::FileUtils::FileExists(ss) ) {
        Util::FileUtils::DeleteFile( ss );
        Util::FileUtils::DeleteFile( _cacheSaveFolder + "speaker_" + _audioInfo.name + std::to_string(i) + ".pcm" );
      }
    }
#endif
  }

  // load our volume settings and update vector's volume accordingly
  LoadSettings();
  SetPlayerVolume();
  
  _audioPlaybackFinishedPtr = std::make_shared<AudioCallbackType>([this](SourceId id){
    CallOnPlaybackFinished( id );
  });
  _audioPlaybackErrorPtr = std::make_shared<AudioCallbackType>([this](SourceId id){
    CallOnPlaybackError( id );
  });

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::Update()
{
  if( _audioController == nullptr ) {
    return; // not init yet
  }

  // switch from Playable to Playing, or from ClipPlayable to ClipPlaying
  
  if( _state == State::Playable ) {
    
    LOG("Update: needsToPlay");
    SetState( State::Playing );
  
    // post event
    using namespace AudioEngine;
    const auto eventID = ToAudioEventId( _audioInfo.playEvent );
    auto* callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread
    std::weak_ptr<AudioCallbackType> weakOnFinished = _audioPlaybackFinishedPtr;
    callbackContext->SetEventCallbackFunc( [weakOnFinished,id=_playingSource]( const AudioCallbackContext* thisContext,
                                                                               const AudioCallbackInfo& callbackInfo ) {
      // if the user logs out, this callback could fire after AlexaMediaPlayer is destoryed. check that here.
      auto callback = weakOnFinished.lock();
      if( callback && (*callback) ) {
        (*callback)( id );
      }
    });

    const auto playingID = _audioController->PostAudioEvent(eventID, _audioInfo.gameObject, callbackContext );
    if (AudioEngine::kInvalidAudioPlayingId == playingID) {
      LOG_ERROR( "AlexaMediaPlayer.Update.CouldNotPlay", "Speaker '%s' could not play", _audioInfo.name.c_str() );
    }
    
  } else if( _state == State::ClipPlayable ) {
    
    if( ANKI_VERIFY( _playableClip != AudioEngine::kInvalidAudioEventId,
                     "AlexaMediaPlayer.Update.PlayingInvalidClip",
                     "State was playable but clip was invalid" ) )
    {
      
      SetState( State::ClipPlaying );
      // post event
      using namespace AudioEngine;
      auto* callbackContext = new AudioCallbackContext();
      callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
      callbackContext->SetExecuteAsync( false ); // run on main thread
      std::weak_ptr<AudioCallbackType> weakOnFinished = _audioPlaybackFinishedPtr;
      std::weak_ptr<AudioCallbackType> weakOnError = _audioPlaybackErrorPtr;
      const bool invalidData = _dataValidity == DataValidity::Invalid;
      callbackContext->SetEventCallbackFunc( [weakOnFinished,
                                              weakOnError,
                                              id=_playingSource,
                                              invalidData]
                                             ( const AudioCallbackContext* thisContext,
                                               const AudioCallbackInfo& callbackInfo )
      {
        // if the user logs out, this event callback could fire after AlexaMediaPlayer is destroyed. check that here.
        if( invalidData ) {
          auto callback = weakOnError.lock();
          if( callback && *callback ) {
            (*callback)( id );
          }
        }
        // not sure if weakOnFinished (CallOnPlaybackFinished) needs to be called if weakOnError was also called
        auto callback = weakOnFinished.lock();
        if( callback && *callback ) {
          (*callback)( id );
        }
      });
      
      const auto playingID = _audioController->PostAudioEvent( _playableClip, _audioInfo.gameObject, callbackContext );
      if( AudioEngine::kInvalidAudioPlayingId == playingID ) {
        LOG_ERROR( "AlexaMediaPlayer.Update.CouldNotPlayClip",
                   "Speaker '%s' could not play clip %d",
                   _audioInfo.name.c_str(), _playableClip );
      }
      _clipStartTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() / 1000.0f;
    } else {
      // ClipPlayable did not have a valid clip
      SetState( State::Idle );
    }
  } else if( _state == State::ClipPlaying ) {
    // artificially update _offset_ms
    const float currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() / 1000.0f;
    _offset_ms += (currTime_ms - _clipStartTime_ms);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::CallOnPlaybackFinished( SourceId id, bool runOnCaller )
{
  auto func = [this, id]() {
    if( _playingSource == id ) {
      LOG( "CallOnPlaybackFinished %d, Setting state=Idle", (int)id );
      SetState(State::Idle);
    }
    else {
      LOG( "CallOnPlaybackFinished %llu, but %llu is playing, so keep state as %s",
           id,
           _playingSource,
           StateToString() );
      LogPlayingSourceMismatchEvent("CallOnPlaybackFinished", id, _playingSource);
    }
    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    for( auto& observer : _observers ) {
      observer->onPlaybackFinished( id );
    }
  };
  if( runOnCaller ) {
    func();
  } else {
    _executor.submit(func);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::CallOnPlaybackError( SourceId id )
{
  _executor.submit([this, id]() {
    if( _playingSource == id ) {
      LOG( "CallOnPlaybackError %d, Setting state=Idle", (int)id );
      SetState(State::Idle);
    }
    else {
      LOG( "CallOnPlaybackError %llu, but source %llu is playing, so keep state as %s",
           id,
           _playingSource,
           StateToString() );
      LogPlayingSourceMismatchEvent("CallOnPlaybackError", id, _playingSource);
    }
    static const std::string kPlaybackError = "The device had trouble with playback";
    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    for( auto& observer : _observers ) {
      observer->onPlaybackError( id,
                                 avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                                 kPlaybackError );
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::GetNewSourceID()
{
  return _nextAvailableSourceID++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( std::shared_ptr< avsCommon::avs::attachment::AttachmentReader > attachmentReader,
                                      const avsCommon::utils::AudioFormat *format )
{
  const SourceId sourceID = GetNewSourceID();
  LOG( "set source %llu with attachment reader (%zu previous readers)", sourceID, _readers.size() );

  OnNewSourceSet();

  _readers[sourceID].reset( new AttachmentReader( std::move(attachmentReader) ) );

  using AudioFormat = avsCommon::utils::AudioFormat;
  if( format !=  nullptr) {
    std::stringstream ss;
    ss << "encoding=" << format->encoding << ", endianness=" << format->endianness;
    LOG( "format: %s, sampleRateHz=%u, sampleSize=%u, numChannels=%u, dataSigned=%d, interleaved=%d",
         ss.str().c_str(), format->sampleRateHz, format->sampleSizeInBits, format->numChannels,
         format->dataSigned, format->layout == AudioFormat::Layout::INTERLEAVED );
  }

  return sourceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( const std::string &url, std::chrono::milliseconds offset )
{
  LOG( "set source with URL=%s", url.c_str() );
  if( url.empty() || _shuttingDown ) {
    return 0;
  }

  if( _urlConverter != nullptr ) {
    _urlConverter->shutdown();
  }
  // Note: calling UrlContentToAttachmentConverter::create() can lead to an SDK thread to call this class's onError(), so
  // clear the relevant flag here instead of at the beginning of the play loop
  _errorDownloading = false;
  _urlConverter = playlistParser::UrlContentToAttachmentConverter::create(_contentFetcherFactory, url, shared_from_this(), offset);
  if (!_urlConverter) {
    return 0;
  }
  auto attachment = _urlConverter->getAttachment();
  if( !attachment ) {
    return 0;
  }
  std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader = attachment->createReader(avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);
  if( !reader ) {
    return 0;
  }

  return setSource( std::move(reader), nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( std::shared_ptr<std::istream> stream, bool repeat )
{
  const SourceId sourceID = GetNewSourceID();
  LOG( "set source %llu with stream, repeat %s (%zu previous readers)",
       sourceID,
       repeat ? "true" : "false",
       _readers.size() );

  OnNewSourceSet();

  _readers[sourceID].reset( new StreamReader( std::move(stream), repeat ) );
  return sourceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::OnNewSourceSet()
{
  // setting a new source, so if we aren't currently idle then we need to stop the previous source
  const State state = _state;
  if( state != State::Idle ) {
    LOG( "Stop playing source %llu from state %s because new source is set",
         _playingSource,
         StateToString(state) );

    DASMSG(alexa_multi_source,
           "alexa.stop_previous_media",
           "The AVS SDK told us to play another piece of media without stopping the last (this is a health / debugging issue)");
    DASMSG_SET(s1, _audioInfo.name, "Audio speaker channel (e.g. TTS, Alerts)");
    DASMSG_SET(s2, StateToString(state), "Audio processing state");
    DASMSG_SEND();

    stop(_playingSource);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::play( SourceId id )
{
  LOG( "play source %llu", id );
  _nextSourceToPlay = id;

  Util::Dispatch::Async( _dispatchQueue, [id, this](){

    std::unique_lock<std::mutex> lk{_playLoopMutex};

    for(int tries = 0; tries < kMaxPlayTries && !_shuttingDown && id == _nextSourceToPlay; ++tries) {
      if( ExchangeState(State::Idle, State::Preparing) ) {
        // success!
        break;
      }
      else {
        lk.unlock();
        LOG("play: state is not idle, sleeping to let it go idle (try %d)", tries);
        // TODO:(bn) condition variable or something, this doesn't seem to happen often though
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lk.lock();
        // loop around again
      }
    }

    if( _state != State::Preparing || _shuttingDown || id != _nextSourceToPlay ) {
      LOG("Failed to set state to preparing, state is %s, giving up on playing source %llu",
          StateToString(),
          id);
      return;
    }

    // at this point, we hold the play loop mutex, and we are the _only_ playing async job for _this_ media
    // player

    _playLoopRunning = true;
    _playingSource = id;

#if defined(ANKI_PLATFORM_VICOS)
    // Setup the thread's affinity mask
    // We don't want to run this on the same core as the Audio Engine to reduce stuttering
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    CPU_SET(1, &cpu_set);
    int error = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (error != 0) {
      LOG_ERROR("AlexaMediaPlayer.play", "SetAffinityMaskError %d", error);
    }
#endif

    bool sentPlaybackStarted = false;

    // reset vars
    _offset_ms = 0;
    _firstPass = true;
    _dataValidity = DataValidity::Unknown;
    _attemptedDecodeBytes = 0;
    _invalidFrames = 0;
    _mp3Buffer->Reset();
    
    mpg123_open_feed( _decoderHandle );
    if( _decoderHandle == nullptr ) {
      LOG_ERROR( "AlexaMediaPlayer.Init.DecoderFeedFail", "Unable to reset decoder feed" );
      return;
    }

    // new waveData instance to hold data passed to wwise
    _waveData = AudioEngine::PlugIns::StreamingWavePortalPlugIn::CreateDataInstance();

    // reset plugin
    {
      auto* pluginInterface = _audioController->GetPluginInterface();
      DEV_ASSERT(nullptr != pluginInterface, "AlexaMediaPlayer.play.InvalidPluginInterface");
      auto* plugin = pluginInterface->GetStreamingWavePortalPlugIn();
      plugin->ClearAudioData( _audioInfo.pluginId );
      plugin->AddDataInstance( _waveData, _audioInfo.pluginId );
    }

    // buffer to read mp3 data from source. this is an unnecessary copy if RingBuffContiguousRead had a
    // raw writable pointer
    std::array<uint8_t,kDownloadSize> input;

    int numBlocks = 0;
    const int kMaxBlocks = 10000; // number of times we can try to check a blocking reader before giving up

    bool invalidData = false;

    auto stateOk = [this] {
      switch (_state) {
        case State::Preparing:
        case State::Playing:
        case State::Playable:
        case State::Paused:
          return true;
        case State::ClipPlayable:
        case State::ClipPlaying:
        case State::Idle:
          return false;
      }
    };
    while( stateOk() && !_shuttingDown ) {

      // todo: not this
      // loop idle while paused until resumed
      if( _state == State::Paused ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }

      if( !sentPlaybackStarted ) {
        LOG( "source %d onPlaybackStarted", (int)id);
        std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
        for( auto& observer : _observers ) {
          observer->onPlaybackStarted( id );
        }
        sentPlaybackStarted = true;
      }

      // read from one of the sources
      bool statusOK = false;

      const auto& reader = _readers[id];

      AlexaReader::Status readStatus = AlexaReader::Status::Ok;
      while( !_shuttingDown ) {
        const size_t spaceForWrite = _mp3Buffer->Capacity() - _mp3Buffer->Size();
        const auto numUnreadInStream = reader->GetNumUnreadBytes();
        // it's possible that size==0 if the download hasn't started. ignore size for now, and try reading below,
        // and use the readStatus of that to determine what to do
        const size_t toReadFromStream = (numUnreadInStream == 0) ? input.size() : std::min(numUnreadInStream, input.size());
        if( spaceForWrite < toReadFromStream ) {
          break;
        }
        const auto bytesRead = reader->Read( input.data(), toReadFromStream, readStatus );
        if( bytesRead > 0 ) {
          // this won't add if there aren't bytesRead available space, hence the compairson to spaceForWrite above
          _mp3Buffer->AddData( input.data(), (int)bytesRead );
        }
        if( readStatus != AlexaReader::Status::Ok ) {
          // LOG( "source %llu readStatus %s", id, AlexaReader::StatusToString(readStatus) );
          break;
        }
      }

      statusOK = (readStatus == AlexaReader::Status::Ok) || (readStatus == AlexaReader::Status::WouldBlock);
      if( readStatus == AlexaReader::Status::WouldBlock ) {
        if( ++numBlocks > kMaxBlocks ) {
          // Too many OK_WOULDBLOCKs. Canceling
          statusOK = false;
        } else {
          // todo: not this
          // Sleep is to wait for audio to buffer
          std::this_thread::sleep_for( std::chrono::milliseconds(50) );
        }
      }

      // if flush, then any data left in the ring buff will get decoded
      const bool flush = !statusOK && !_shuttingDown;
      // decode everything in the buffer thus far, leaving only something that perhaps wasnt a full mp3 frame
      const int timeDecoded_ms = Decode( _waveData, flush );
      _offset_ms += timeDecoded_ms;

      // sleep to avoid downloading data faster than it is played. todo: not this...
      if( !flush && timeDecoded_ms > 0 && !_shuttingDown ) {
        const auto samplesInBuffer = _waveData->GetNumberOfFramesReceived() - _waveData->GetNumberOfFramesPlayed();
        size_t sleepFor_ms;
        if (samplesInBuffer > _idealBufferSampleSize) {
          // Sleep until we have about _idealBufferSampleSize of samples left in buffer
          sleepFor_ms = ((samplesInBuffer - _idealBufferSampleSize) * 1000) / _waveData->GetSampleRate();
        }
        else if ( _state == State::Preparing ) {
          // Keep working if are trying to get enough data to start playback
          sleepFor_ms = 15;
        }
        else {
          // Smaller buffer then we would like
          sleepFor_ms = Util::Clamp(timeDecoded_ms, 25, static_cast<const int>(timeDecoded_ms * 0.5f));
        }
        // LOG("Sleep for %zu ms", sleepFor_ms);
        std::this_thread::sleep_for( std::chrono::milliseconds(sleepFor_ms) );
      }

      if( _dataValidity == DataValidity::Unknown ) {
        if( _attemptedDecodeBytes > kMaxInvalidBytesBeforeAbort ) {
          LOG_WARNING("AlexaMediaPlayer.Play.InvalidData.DecodeError",
                      "%s: source %llu Attempted to decode %zu bytes (> limit of %zu), setting data invalid",
                      _audioInfo.name.c_str(),
                      id,
                      _attemptedDecodeBytes,
                      kMaxInvalidBytesBeforeAbort);
          invalidData = true; // this will cause a break further down
        } else if( _invalidFrames > kMaxInvalidFramesBeforeAbort ) {
          LOG_WARNING("AlexaMediaPlayer.Play.InvalidData.FrameError",
                      "%s: source %llu Attempted to decode %d frames (> limit of %zu), setting data invalid",
                      _audioInfo.name.c_str(),
                      id,
                      _invalidFrames,
                      kMaxInvalidFramesBeforeAbort);
          invalidData = true; // this will cause a break further down
        }
      }

      if( flush || invalidData || _shuttingDown || _errorDownloading ) {
        LOG( "done producing data on waveData %p for source %llu (flush:%d invalid:%d 404:%d)",
             _waveData.get(),
             id,
             flush,
             invalidData,
             (int)_errorDownloading );
        _waveData->DoneProducingData();
        break;
      }
    } // end while

    // flush debug dumps
    SavePCM( nullptr );
    SaveMP3( nullptr );
    
    invalidData |= _errorDownloading;

    if( !invalidData && (_state == State::Preparing) && (_dataValidity == DataValidity::Valid) ) {
      // if the above loop exits and the decoder has prepared valid data (it decoded at least 1 sample),
      // then switch the state to Playable. normally this happens when enough samples have been decoded,
      // but sometimes downloading finishes before that
      ExchangeState(State::Preparing, State::Playable);
    } else if( invalidData ) {
      // if the decoder loop was aborted because of invalid data, switch to playing a clip instead. this
      // play-thread method will end, and Update() will check the new state (ClipPlayable) and switch to that.
      // we should only be in State::Preparing at this stage, since data validity is tied to not being able to
      // decode a single byte
      ASSERT_NAMED_EVENT( _state == State::Preparing, "AlexaMediaPlayer.play.InvalidButNotPreparing", "state='%s'", StateToString() );
      _dataValidity = DataValidity::Invalid;
      using namespace AudioEngine;
      using GenericEvent = AudioMetaData::GameEvent::GenericEvent;
      _playableClip = ToAudioEventId( GenericEvent::Play__Robot_Vic_Alexa__Avs_System_Prompt_Error_Cannot_Play_Song );
      SetState( State::ClipPlayable );
      // to be determined is whether the sdk will do something unexpected if we close the readers
      // below when switching to playing a clip.
      if( _errorDownloading ) {
        DASMSG(alexa_unavailable_audio,
               "alexa.unavailable_audio",
               "404 error when trying to download audio");
        DASMSG_SEND();
      } else {
        DASMSG(alexa_unsupported_audio_format,
               "alexa.unsupported_audio_format",
               "The audio decoder had trouble with the given media");
        DASMSG_SEND();
      }
      _errorDownloading = false;
    }

    _readers[id]->Close();
    _readers[id].reset();
    _readers.erase(id);

    if( _shuttingDown ) {
      _audioController->StopAllAudioEvents( _audioInfo.gameObject );
    }

    LOG( "playing complete for source %llu", id );

    _playLoopRunning = false;
    _playLoopRunningCondition.notify_all();

  }); // end Dispatch Async

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::stop( SourceId id )
{
  LOG( "stop source %llu", id );
  return StopInternal( id );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::StopInternal( SourceId id, bool runOnCaller )
{
  _audioController->StopAllAudioEvents( _audioInfo.gameObject );

  auto onStop = [id, this]() {

    {
      // first attempt to grab the playing lock to avoid race conditions with the ::play Async call
      std::unique_lock<std::mutex> lk{_playLoopMutex, std::try_to_lock};

      // if we got the lock, then the play thread isn't running. If not, then it _is_ running with the lock,
      // which means that it should stop on it's own

      // if we are playing or are about to play the given source, stop it
      if( _playingSource == id ||
          _nextSourceToPlay == id) {
        SetState(State::Idle);
        const bool stoppingPlaying = (_playingSource == id) && (_nextSourceToPlay <= id);
        const bool stoppingBeforePlay = (id > _playingSource) && (_nextSourceToPlay == id);
        if( stoppingBeforePlay || stoppingPlaying ) {
          _nextSourceToPlay = 0;
        }
        _offset_ms = 0;
      }
      else {
        LOG("AlexaMediaPlayer.StopInternal.OnStop.OtherSourcePlaying Stopping source %llu, but %llu is playing, so leaving state as '%s'",
            id,
            _playingSource,
            StateToString());
        LogPlayingSourceMismatchEvent("StopInternal", id, _playingSource);
      }

      // let the play mutex guard go out of scope here. NOTE: we may not actually hold it since we used a try_to_lock
    }

    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackStopped for source %d", (int)id );
      observer->onPlaybackStopped( id );
    }
  };
  if( runOnCaller ) {
    onStop();
  } else {
    _executor.submit(onStop);
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::pause( SourceId id )
{
  if( _playingSource != id && _nextSourceToPlay != id ) {
    LOG( "pause source %llu FAIL, _playingSource %llu, _nextSourceToPlay %llu",
         id,
         _playingSource,
         _nextSourceToPlay);
    LogPlayingSourceMismatchEvent("pause", id, _playingSource);
    return false;
  }

  if( _state == State::Idle ) {
    LOG( "pause source %llu FAIL _state == State::Idle", id );
    return false;
  }

  _executor.submit([id, this]() {

    // it's possible this could run a few iterations if the play thread is changing the state, but should be exceedingly rare
    for(int tries = 0; tries < kMaxPlayTries; ++tries) {
      const State state = _state;
      _stateBeforePause = state;
      if( ExchangeState( _stateBeforePause, State::Paused ) ) {
        // exchange success!
        LOG( "pause source %llu from state '%s'", id, StateToString(_stateBeforePause) );
        break;
      }
      else {
#if ANKI_DEV_CHEATS
        LOG( "pause source %llu try %d failed, _stateBeforePause %s, _state %s",
             id,
             tries,
             StateToString(_stateBeforePause),
             StateToString(_state));
#endif
      }
    }

    if( _state != State::Paused ) {
      return;
    }

    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackPaused for source %d", (int)id );
      observer->onPlaybackPaused( id );
    }
  });

  const auto eventID = AudioEngine::ToAudioEventId( _audioInfo.pauseEvent );
  _audioController->PostAudioEvent(eventID, _audioInfo.gameObject );
  // LOG( "pause return TRUE");
 return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::resume( SourceId id )
{
  if( _playingSource != id ) {
    LOG( "resume source %llu FAIL because playing source is %llu", id, _playingSource);
    LogPlayingSourceMismatchEvent("resume", id, _playingSource);
    return false;
  }

  if( _state != State::Paused ) {
    LOG( "resume source %llu FAIL because _state '%s' != State::Paused", id, StateToString() );
    return false;
  }

  _executor.submit([id, this]() {

    // set state back to what it was previously
    const bool exchangeOK = ExchangeState(State::Paused, _stateBeforePause);
    if( !exchangeOK ) {
      LOG( "resume source %llu ERROR, state should have been paused, now is %s", id, StateToString() );
      return;
    }

    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackResumed for source %d", (int)id );
      observer->onPlaybackResumed( id );
    }
  });

  const auto eventID = AudioEngine::ToAudioEventId( _audioInfo.resumeEvent );
  _audioController->PostAudioEvent(eventID, _audioInfo.gameObject );
 // LOG( "resume RETURN True" );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::chrono::milliseconds AlexaMediaPlayer::getOffset( SourceId id )
{
  if( id == _playingSource ) {
    return std::chrono::milliseconds{_offset_ms};
  }
  else {
    LOG_ERROR("AlexaMediaPlayer.GetOffset.SourceMismatch",
              "requesting source for id %llu, but last playing is %llu",
              id,
              _playingSource);
    LogPlayingSourceMismatchEvent("getOffset", id, _playingSource);
    return std::chrono::milliseconds{0};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint64_t AlexaMediaPlayer::getNumBytesBuffered()
{
  LOG( "getNumBytesBuffered NOT IMPLEMENTED" );
  // This should return "the number of bytes queued up in the media player buffers."
  // _attemptedDecodeBytes is the number of bytes downloaded, but we still need the number of bytes played (likely bytes
  // actually played, not just sent to wwise). I have no idea how the return value of this method is used.
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::setObserver( std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver )
{
  std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
  if( playerObserver == nullptr ) {
    // this seems to happen _after_ we doShutDown of some media players. Ideally we would only remove the observer that
    // the caller originally added, but that's not the API.
    _observers.clear();
  } else {
    _observers.insert( playerObserver );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::OnSettingsChanged() const
{
  SetPlayerVolume();
  SaveSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SetPlayerVolume() const
{
  const float playerVolume = _settings.mute ? 0.0f : (_settings.volume / 100.0f);

  const auto parameterId = AudioEngine::ToAudioParameterId( _audioInfo.volumeParameter );
  const auto parameterValue = AudioEngine::ToAudioRTPCValue( playerVolume );
  _audioController->SetParameter( parameterId, parameterValue, AudioEngine::kInvalidAudioGameObject );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int AlexaMediaPlayer::Decode( const StreamingWaveDataPtr& data, bool flush )
{
  float decoded_ms = 0.0f;

  // it may also make sense to exit this loop if not flush and Size() < kMaxReadSize
  size_t buffSize = _mp3Buffer->GetContiguousSize(); // guaranteed to be at least min{Size(), kMaxReadSize}
  const unsigned char* inputBuff = nullptr;
  if( buffSize != 0) {
    inputBuff = _mp3Buffer->ReadData( (int)buffSize );
  }

  if( inputBuff == nullptr ) {
    // need to wait for more data
    return 0;
  }
  
  SaveMP3( inputBuff, buffSize );
  
  // mpg123 keeps its own buffer, so even if it needs more data, it will hold on to a partial frame, and we
  // can advance the read cursor here
  bool success = _mp3Buffer->AdvanceCursor( (int)buffSize );
  if( !success ) {
    LOG_ERROR( "AlexaMediaPlayer.Decode.ReadPtr", "Could not move read pointer by %zu (size=%zu)",
               buffSize, _mp3Buffer->Size() );
  }
  
  _attemptedDecodeBytes += buffSize;

  size_t decodedSize = 0;
  unsigned char* outBuff = (unsigned char*) _decodedPcm.data();
  const int outBuffSize = (int)_decodedPcm.size() * sizeof(short);
  
  int decodeRes = mpg123_feed ( _decoderHandle, inputBuff, buffSize );
  if( decodeRes == MPG123_ERR ) {
    LOG_ERROR( "AlexaMediaPlayer.Decode.FeedError", "decoder error: %s", mpg123_strerror(_decoderHandle) );
  } else if( decodeRes == MPG123_NEED_MORE ) {
    return 0;
  }
  
  decodeRes = mpg123_framebyframe_next( _decoderHandle );
  while( !_shuttingDown && (decodeRes != MPG123_ERR) && (decodeRes != MPG123_NEED_MORE) ) {
    
    if( decodeRes == MPG123_NEW_FORMAT ) {
      int enc = 0;
      mpg123_getformat( _decoderHandle, &_mediaInfo.sampleRate, &_mediaInfo.channels, &enc );
      
      _idealBufferSampleSize = static_cast<size_t>( _mediaInfo.sampleRate * kIdealBufferSize_sec );
      _minPlaybackBufferSize = static_cast<size_t>( _mediaInfo.sampleRate * kMinBufferSize_sec );
      LOG( "DECODING: channels=%d, hz=%ld, encoding=%d, _minPlaybackBufferSize=%zu, _idealBufferSampleSize=%zu",
          _mediaInfo.channels, _mediaInfo.sampleRate, enc, _minPlaybackBufferSize, _idealBufferSampleSize );
    }
    
    
    
    unsigned long header = 0;
    unsigned char* body = nullptr;
    size_t bodySize = 0;
    decodeRes = mpg123_framedata( _decoderHandle, &header, &body, &bodySize );
    if( decodeRes == MPG123_OK ) {
      
      bool validHeader = true;
      if( header != 0 ) {
        // this decoder does a poor job at differentiating between MP3 and AAC. Check the "reserved" fields
        // https://www.mp3-tech.org/programmer/frame_header.html
        const int byte2 = (header >> 16) & 0xFF;
        const bool reserved1 = (byte2 & 0b00011000) == 0b00001000;
        const bool reserved2 = (byte2 & 0b00000110) == 0;
        if( reserved1 || reserved2 ) {
          validHeader = false;
          ++_invalidFrames;
          if( _invalidFrames >= kMaxInvalidFramesBeforeAbort ) {
            _dataValidity = DataValidity::Unknown;
            return decoded_ms;
          }
        }
      }
      
      if( validHeader && (bodySize > 0) && (body != nullptr) ) {
        off_t offset = 0;
        unsigned char* decodedAudio = nullptr;
        size_t frameBytes = 0;
        decodeRes = mpg123_framebyframe_decode( _decoderHandle, &offset, &decodedAudio, &frameBytes );
        if( decodedSize + frameBytes >= outBuffSize ) {
          // there's no room in our output buffer. pass it on and start over
          if( decodedSize > 0 ) {
            DEV_ASSERT( (decodedSize & 1) == 0, "Should have decoded full shorts" );
            const size_t shortBytes = decodedSize / sizeof(short); // decoder uses bytes, we use shorts
            decoded_ms += CopyToWaveData( shortBytes, data, flush );
          }
          decodedSize = 0;
        }
        // there should be space now
        DEV_ASSERT( decodedSize + frameBytes < outBuffSize, "AlexaMediaPlayer.Decode.NoOutputSpace" );
        std::copy( decodedAudio, decodedAudio + frameBytes, outBuff + decodedSize );
        decodedSize += frameBytes;
      }
        
    } else if( decodeRes == MPG123_ERR ) {
      LOG_ERROR( "AlexaMediaPlayer.Decode.DataError", "decoder error: %s", mpg123_strerror(_decoderHandle) );
    }
    
    decodeRes = mpg123_framebyframe_next( _decoderHandle );
    if( decodeRes == MPG123_ERR ) {
      LOG_ERROR( "AlexaMediaPlayer.Decode.NextError", "decoder error: %s", mpg123_strerror(_decoderHandle) );
    }
  }
  
  if( decodeRes == MPG123_ERR ) {
    LOG_ERROR( "AlexaMediaPlayer.Decode.DataError", "decoder error: %s", mpg123_strerror(_decoderHandle) );
  }
  
  if( decodedSize > 0 || flush ) {
    DEV_ASSERT( (decodedSize & 1) == 0, "Should have decoded full shorts" );
    const size_t shortBytes = decodedSize / sizeof(short); // decoder uses bytes, we use shorts
    decoded_ms += CopyToWaveData( shortBytes, data, flush );
  }
  

  return int(decoded_ms);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float AlexaMediaPlayer::CopyToWaveData( size_t samples, const StreamingWaveDataPtr& waveData, bool flush )
{
  using namespace AudioEngine;
  
  if( samples == 0 ) {
    return 0;
  }
  
  // valid wav data!
  _dataValidity = DataValidity::Valid;
  
  uint16_t channels = static_cast<uint16_t>( _mediaInfo.channels );
  
  if( channels == 2 ) {
    // audio engine only supports mono. loop through samples and rewrite in the same buffer
    int j = 0;
    for( int i=0; i<samples; i += 2, j += 1) {
      short left = _decodedPcm[i];
      short right = _decodedPcm[i + 1];
      short monoSample = (int(left) + right) / 2;
      _decodedPcm[j] = monoSample;
    }
    samples = samples/2;
    channels = 1;
  }
  
  float decoded_ms = 1000 * float(samples) / _mediaInfo.sampleRate;
  
  if( samples > 0 ) {
    SavePCM( _decodedPcm.data(), samples );
  }
  
  StandardWaveDataContainer waveContainer{ static_cast<uint32_t>(_mediaInfo.sampleRate), channels, samples };
  waveContainer.CopyWaveData( _decodedPcm.data(), samples );
  
  waveData->AppendStandardWaveData( std::move(waveContainer) );
  
  const auto numFrames = waveData->GetNumberOfFramesReceived();
  if( (_state == State::Preparing) && ((numFrames >= _minPlaybackBufferSize) || flush) ) {
    // LOG("Now Playable, numFrames %d minFrames %zu flush %d", numFrames, _minPlaybackBufferSize, flush );
    ExchangeState(State::Preparing, State::Playable);
  }
  
  return decoded_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const AlexaMediaPlayer::StateToString(const AlexaMediaPlayer::State& state)
{
  switch( state ) {
    case State::Idle: return "Idle";
    case State::Preparing: return "Preparing";
    case State::Playable: return "Playable";
    case State::Playing: return "Playing";
    case State::Paused: return "Paused";
    case State::ClipPlayable: return "ClipPlayable";
    case State::ClipPlaying: return "ClipPlaying";
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const AlexaMediaPlayer::StateToString() const
{
  return StateToString( _state );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SavePCM( short* buff, size_t size ) const
{
#if ANKI_DEV_CHEATS
  if( !kSaveAlexaAudio ) {
    return;
  }
  static int pcmfd = -1;
  if( pcmfd < 0 ) {
    const auto path = _cacheSaveFolder + "speaker_" + _audioInfo.name + std::to_string(_playingSource) + ".pcm";
    pcmfd = open( path.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0644 );
  }

  if( size > 0 && (buff != nullptr) ) {
    (void) write( pcmfd, buff, size * sizeof(short) );
  } else if( pcmfd >= 0 ) {
    close( pcmfd );
    pcmfd = -1;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SaveMP3( const unsigned char* buff, size_t size ) const
{
#if ANKI_DEV_CHEATS
  if( !kSaveAlexaAudio ) {
    return;
  }
  static int mp3fd = -1;
  if( mp3fd < 0 ) {
    const auto path = _cacheSaveFolder + "speaker_" + _audioInfo.name + std::to_string(_playingSource) + ".mp3";
    mp3fd = open( path.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0644 );
  }

  if( size > 0 && (buff != nullptr) ) {
    (void) write( mp3fd, buff, size * sizeof(unsigned char) );
  } else if( mp3fd >= 0 ) {
    close( mp3fd );
    mp3fd = -1;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string AlexaMediaPlayer::GetSettingsFilename() const
{
  return _persistentSaveFolder + "speaker_" + _audioInfo.name + "_settings.json";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SaveSettings() const
{
  if( !_persistentSaveFolder.empty() ) {
    const std::string filename = GetSettingsFilename();

    Json::Value metadata;
    metadata[kSaveSettings_VolumeKey]   = _settings.volume;
    metadata[kSaveSettings_MuteKey]     = _settings.mute;

    Util::FileUtils::WriteFile(filename, metadata.toStyledString());

    LOG( "saved settings: volume [%d], mute [%d]", _settings.volume, _settings.mute );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::LoadSettings()
{
  if( !_persistentSaveFolder.empty() ) {
    const std::string filename = GetSettingsFilename();
    const std::string fileContents = Util::FileUtils::ReadFile( filename );

    Json::Reader reader;
    Json::Value metadata;

    // if the file exists, we should have some content
    if( !fileContents.empty() && reader.parse( fileContents, metadata ) ) {
      _settings.volume      = metadata[kSaveSettings_VolumeKey].asInt();
      _settings.mute        = metadata[kSaveSettings_MuteKey].asBool();

      LOG( "loaded settings: volume [%d], mute [%d]", _settings.volume, _settings.mute );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::getSpeakerSettings( SpeakerSettings *settings )
{
  if( settings != nullptr ) {
    *settings = _settings;
    return true;
  }
  else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::setVolume( int8_t volume )
{
  if( volume != _settings.volume ) {
    _settings.volume = volume;
    LOG( "setting volume to %d", _settings.volume );
    OnSettingsChanged();
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::adjustVolume( int8_t delta )
{
  const auto previousVolume = _settings.volume;

  _settings.volume += delta;
  if( _settings.volume < avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN;
  }
  if( _settings.volume > avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  }

  if( previousVolume != _settings.volume ) {
    LOG( "adjusting volume by %d to %d", delta, _settings.volume );
    OnSettingsChanged();
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::setMute( bool mute )
{
  if( mute != _settings.mute ) {
    _settings.mute = mute;
    LOG( "setting mute=%d ", mute );
    OnSettingsChanged();
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::onError()
{
  // Usually this means the _urlConverter experienced a 404 error when trying to make an attachment reader
  LOG( "An error occurred in the streaming of content" );
  
  // Instead of dealing the the play/stop thread loops, just set a flag that is handled in the play loop. The API here
  // unfortunately doesn't tell us what sourceID we're dealing with, so we don't know whether the error is with
  // the current play loop or another. We also don't know if the play loop will have started before or after this
  // flag is set. Set the flag and hope for the best. TODO: delete this whole class
  _errorDownloading = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::doShutdown()
{
  _shuttingDown = true;
  // wait for the DispatchQueue play loop to complete. The thread is still running, but is cleaned up during destruction
  std::unique_lock<std::mutex> lk{_playLoopMutex};
  _playLoopRunningCondition.wait( lk, [this]{ return !_playLoopRunning; } );
  
  if( _audioController ) {
    _audioController->StopAllAudioEvents( _audioInfo.gameObject );
  }
  if( _state != State::Idle ) {
    // if something is playing, notify observers of a stop, otherwise there ends up being a circular
    // ref fiasco among shared_ptrs in AlexaClient
    const bool runOnThisThread = true;
    StopInternal( _playingSource, runOnThisThread );
    // not sure if this is also needed
    CallOnPlaybackFinished(_playingSource, runOnThisThread);
  }
  // _observers must be cleared here, otherwise there ends up being a circular ref fiasco among
  // shared_ptrs in AlexaClient and AlexaImpl, even if nothing is playing. But since _observers is
  // referenced in the _executor, first stop _executor, then clear _observers.
  _executor.shutdown();
  {
    std::lock_guard<std::recursive_mutex> lg{ _observerMutex };
    _observers.clear();
  }

  if( _urlConverter != nullptr ) {
    _urlConverter->shutdown();
    _urlConverter.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type AlexaMediaPlayer::getSpeakerType()
{
  return _audioInfo.speakerType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SetState( State state )
{
  _state = state;
  LOG("State: %s", StateToString());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::ExchangeState( State expectedCurrentState, State desiredState )
{
  const bool exchanged = _state.compare_exchange_strong(expectedCurrentState, desiredState);
  if( exchanged ) {
    LOG("State: %s (exchange success from %s)", StateToString(), StateToString(expectedCurrentState));
  }
  else {
    LOG("State remains %s because exchange failed (desired was %s)!", StateToString(), StateToString(desiredState));
  }
  return exchanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::LogPlayingSourceMismatchEvent(const std::string& func, SourceId id, SourceId playingId)
{
  int sourceDelta = (int)id - (int)playingId;

  DASMSG(source_mismatch_event,
         "alexa.source_mismatch",
         "Media player source mismatch. This is a health / debugging event that is expected to happen sometimes");
  DASMSG_SET(s1, _audioInfo.name, "Audio speaker channel (e.g. TTS, Alerts)");
  DASMSG_SET(s2, func, "Calling funciton / identifier");
  DASMSG_SET(s3, StateToString(), "Current audio processing state");
  DASMSG_SET(i1, sourceDelta, "Delta between source being operated on and the playing source");
  DASMSG_SEND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::IsActive()
{
  if( _state != State::Idle ) {
    // simple case, not idle then we're active
    return true;
  }

  // now to detect if we _might become_ active, first try to grab the playing lock
  std::unique_lock<std::mutex> lk{_playLoopMutex, std::try_to_lock};
  const bool gotLock = lk.owns_lock();
  if( !gotLock ) {
    // other thread is doing stuff, so we must be active
    return true;
  }

  if( _state != State::Idle ) {
    // state updated to non-idle after grabbing the lock
    return true;
  }

  if( _nextSourceToPlay > _playingSource ) {
    // about to play a new source
    return true;
  }

  // otherwise, we must not be doing anything
  return false;
}


} // namespace Vector
} // namespace Anki
