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


#include "player.h"

#include "attachmentReader.h"
#include "streamReader.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "util/console/consoleInterface.h"
#include "util/container/ringBuffContiguousRead.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"
#include "util/math/math.h"
#include "util/time/universalTime.h"

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/SDS/ReaderPolicy.h>

// HACK
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerSystem.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerTHFSimple.h"
#include "speex/speex_resampler.h"

// include minimp3
// TODO: minimp3 should be able to convert directly to float by adding a flag. this would save a copy.
#define MINIMP3_IMPLEMENTATION
#if defined(VICOS)
#  define HAVE_SSE 0
#endif
#include "minimp3/minimp3.h"

// for saving PCM to disk
#include <fcntl.h>
#include <unistd.h>
#include <array>

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;

using SourceId = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;
SourceId AlexaMediaPlayer::_sourceID = 1; // 0 is invalid

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

  // max read size for _mp3Buffer
  constexpr size_t kMaxReadSize = 16384;
  constexpr size_t kMinAudioToDecode = 8192;
  constexpr size_t kDownloadSize = 8192;
  constexpr float  kIdealBufferSize_sec = 2.0f;
  constexpr float  kMinBufferSize_sec = 2.5f;

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
  #define LOG_CHANNEL "Alexa"
  #define LOG(x, ...) LOG_INFO("AlexaMediaPlayer.SpeakerInfo", "%s: " x, _audioInfo.name.c_str(), ##__VA_ARGS__)
#if ANKI_DEV_CHEATS
  const bool kSaveDebugAudio = true; // FIXME we probably don't want to be shiping this!!
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMediaPlayer::AlexaMediaPlayer( Type type,
                                    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory )
  : avsCommon::utils::RequiresShutdown{"AlexaMediaPlayer_" + sAudioInfo.at(type).name}
  , _type( type )
  , _state(State::Idle)
  , _mp3Buffer( new Util::RingBuffContiguousRead<uint8_t>{ kAudioBufferSize, kMaxReadSize } )
  , _dispatchQueue(Util::Dispatch::Create("AlexaMediaPlayer"))
  , _contentFetcherFactory( contentFetcherFactory )
  , _audioInfo( sAudioInfo.at(_type) )
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

  if (_speexState != nullptr) {
    speex_resampler_destroy(_speexState);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::Init( const AnimContext* context )
{
  DEV_ASSERT(nullptr != context, "RadioAudioComponent.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "RadioAudioComponent.InvalidAudioController");

  _audioController  = context->GetAudioController();
  DEV_ASSERT_MSG( _audioController != nullptr, "AudioMediaPlayer.Init.InvalidAudioController", "" );

  mp3dec_init( &_mp3decoder );

  // in dev, remove any debug audio files from last time
  const Util::Data::DataPlatform* platform = context->GetDataPlatform();
  if( platform != nullptr ) {
    _saveFolder = platform->pathToResource( Util::Data::Scope::Cache, "alexa" );
    _saveFolder = Util::FileUtils::AddTrailingFileSeparator( _saveFolder );
    if( !_saveFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( _saveFolder ) ) {
      Util::FileUtils::CreateDirectory( _saveFolder );
    }
#if ANKI_DEV_CHEATS
    for( int i=1; i<100; ++i ) {
      std::string ss = _saveFolder + "speaker_" + _audioInfo.name + std::to_string(i) + ".mp3";
      if( Util::FileUtils::FileExists(ss) ) {
        Util::FileUtils::DeleteFile( ss );
        Util::FileUtils::DeleteFile( _saveFolder + "speaker_" + _audioInfo.name + std::to_string(i) + ".pcm" );
      }
    }
#endif
  }

  if (_type == AlexaMediaPlayer::Type::TTS) {
    auto* dataPlatform = context->GetDataLoader();
    _speechRegSys = context->GetMicDataSystem()->GetSpeechRecognizerSystem();
    // Setup Callback when stream starts, want it to happen on the same thread
    _speechRegSys->InitAlexaPlayback(*dataPlatform, nullptr);
    _recognizer = _speechRegSys->GetAlexaPlaybackRecognizer();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::Update()
{
  if( _audioController == nullptr ) {
    return; // not init yet
  }

  bool needsToPlay = false;
  {
    if( _state == State::Playable ) {
      LOG("Update: needsToPlay");
      needsToPlay = true;
      SetState( State::Playing );
    }
  }

  if( needsToPlay ) {
    // post event
    using namespace AudioEngine;
    const auto eventID = ToAudioEventId( _audioInfo.playEvent );
    auto* callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread
    callbackContext->SetEventCallbackFunc( [this, id = _playingSource]( const AudioCallbackContext* thisContext,
                                                                        const AudioCallbackInfo& callbackInfo ) {
      // FIXME: this might be a bug. Need to check callback purpose, may be setting bad state
      CallOnPlaybackFinished( id );
    });

    const auto playingID = _audioController->PostAudioEvent(eventID, _audioInfo.gameObject, callbackContext );
    if (AudioEngine::kInvalidAudioPlayingId == playingID) {
      PRINT_NAMED_ERROR( "AlexaMediaPlayer.Update.CouldNotPlay", "Speaker '%s' could not play", _audioInfo.name.c_str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::CallOnPlaybackFinished( SourceId id )
{
  _executor.submit([this, id]() {
    LOG( "CallOnPlaybackFinished %d, Setting state=Idle", (int)id );
    SetState(State::Idle);
    for( auto& observer : _observers ) {
      observer->onPlaybackFinished( id );
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( std::shared_ptr< avsCommon::avs::attachment::AttachmentReader > attachmentReader,
                                      const avsCommon::utils::AudioFormat *format )
{
  // TODO (VIC-9853):
  // Implementations must make a call to onPlaybackStopped() with the previous SourceId when a new source is
  // set if the previous source was in a non-stopped state. Any calls to a MediaPlayerInterface after an
  // onPlaybackStopped() call will fail, as the MediaPlayer has "reset" its state.

  LOG( "set source with attachment reader" );
  _readers[_sourceID].reset( new AttachmentReader( std::move(attachmentReader) ) );

  using AudioFormat = avsCommon::utils::AudioFormat;
  if( format !=  nullptr) {
    std::stringstream ss;
    ss << "encoding=" << format->encoding << ", endianness=" << format->endianness;
    LOG( "format: %s, sampleRateHz=%u, sampleSize=%u, numChannels=%u, dataSigned=%d, interleaved=%d",
         ss.str().c_str(), format->sampleRateHz, format->sampleSizeInBits, format->numChannels,
         format->dataSigned, format->layout == AudioFormat::Layout::INTERLEAVED );
  }

  return _sourceID++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( const std::string &url, std::chrono::milliseconds offset )
{
  LOG( "set source with URL=%s", url.c_str() );
  if( url.empty() ) {
    return 0;
  }

  if( _urlConverter != nullptr ) {
    _urlConverter->shutdown();
  }
  _urlConverter = playlistParser::UrlContentToAttachmentConverter::create(_contentFetcherFactory, url, shared_from_this(), offset);
  if (!_urlConverter) {
    return 0;
  }
  auto attachment = _urlConverter->getAttachment();
  if (!attachment) {
    return 0;
  }
  std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader = attachment->createReader(avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);
  if (!reader) {
    return 0;
  }

  return setSource( std::move(reader), nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( std::shared_ptr<std::istream> stream, bool repeat )
{
  LOG( "set source with stream, repeat %s", repeat ? "true" : "false" );

  _readers[_sourceID].reset( new StreamReader( std::move(stream), repeat ) );
  return _sourceID++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::play( SourceId id )
{
  LOG( "play" );
  _playingSource = id;

  Util::Dispatch::Async( _dispatchQueue, [id, this](){

    const auto threadName = "APlayer_" + _audioInfo.name;
    Anki::Util::SetThreadName(pthread_self(), threadName.c_str());
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

    SetState(State::Preparing);

    bool sentPlaybackStarted = false;

    // reset vars
    _offset_ms = 0;
    _firstPass = true;
    _mp3Buffer->Reset();
    _detectedTriggers_ms = decltype(_detectedTriggers_ms){};
    if (_recognizer != nullptr) {
      _recognizer->Reset();
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

    float lastPlayedMs = 0;
    auto stateOk = [this] {
      switch (_state) {
        case State::Preparing:
        case State::Playing:
        case State::Playable:
        case State::Paused:
          return true;
        default:
          return false;
      }
    };
    while( stateOk() ) {

      UpdateDetectorState(lastPlayedMs);

      // todo: not this
      // loop idle while paused until resumed
      if( _state == State::Paused ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }

      if( !sentPlaybackStarted ) {
        for( auto& observer : _observers ) {
          LOG( "source %d onPlaybackStarted", (int)id);
          observer->onPlaybackStarted( id );
        }
        sentPlaybackStarted = true;
      }

      // read from one of the sources
      bool statusOK = false;

      const auto& reader = _readers[id];

      AlexaReader::Status readStatus = AlexaReader::Status::Ok;
      while( true ) {
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
      const bool flush = !statusOK;
      // decode everything in the buffer thus far, leaving only something that perhaps wasnt a full mp3 frame
      const int timeDecoded_ms = Decode( _waveData, flush );
      _offset_ms += timeDecoded_ms;

      // sleep to avoid downloading data faster than it is played. todo: not this...
      if( !flush && timeDecoded_ms > 0 ) {
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

      if( flush ) {
        _waveData->DoneProducingData();
        break;
      }
    }

    // flush debug dumps
    SavePCM( nullptr );
    SaveMP3( nullptr );

    if( _state == State::Preparing ) {
      SetState(State::Playable);
    }

    _readers[id]->Close();
    _readers[id].reset();

    while (!_detectedTriggers_ms.empty()) {
      if (_state == State::Idle) {
        break;
      }
      UpdateDetectorState(lastPlayedMs);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // don't let wake word stay disabled if something weird happened
    if (_speechRegSys != nullptr) {
      _speechRegSys->ReEnableAlexa();
    }
  });

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::stop( SourceId id )
{
  LOG( "stop" );

  // FIXME: This should only stop the event that this media player is playing not all every Alexa events
  _audioController->StopAllAudioEvents( _audioInfo.gameObject );

  _executor.submit([id, this]() {
    SetState(State::Idle);
    _offset_ms = 0;
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackStopped for source %d", (int)id );
      observer->onPlaybackStopped( id );
    }
  });
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::pause( SourceId id )
{
  if( _readers.find( id ) == _readers.end() ) {
    // LOG( "pause FAIL _readers.find( id ) == _readers.end()" );
    return false;
  }

  if( _state == State::Idle ) {
    // LOG( "pause FAIL _state == State::Idle" );
    return false;
  }

  _executor.submit([id, this]() {
    SetState(State::Paused);
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
  if( _readers.find( id ) == _readers.end() ) {
    // LOG( "resume _readers.find( id ) == _readers.end()" );
    return false;
  }

  if( _state != State::Paused ) {
    // LOG( "resume _state != State::Paused" );
    return false;
  }

  _executor.submit([id, this]() {
    SetState( State::Playing );
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
  return std::chrono::milliseconds{_offset_ms};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint64_t AlexaMediaPlayer::getNumBytesBuffered()
{
  LOG( "getNumBytesBuffered NOT IMPLEMENTED" );
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::setObserver( std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver )
{
  _observers.insert( playerObserver );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SetPlayerVolume(float volume)
{
  const auto parameterId = AudioEngine::ToAudioParameterId( _audioInfo.volumeParameter );
  const auto parameterValue = AudioEngine::ToAudioRTPCValue( volume );
  _audioController->SetParameter( parameterId, parameterValue, AudioEngine::kInvalidAudioGameObject );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int AlexaMediaPlayer::Decode( const StreamingWaveDataPtr& data, bool flush )
{
  using namespace AudioEngine;

  mp3dec_frame_info_t info;
  float decoded_ms = 0.0f;

  while( true ) {

    // unless we're flushing, don't bother decoding without a minimum amount of data in buffer
    if( !flush && (_mp3Buffer->Size() < kMinAudioToDecode) ) {
      break;
    }

    // it may also make sense to exit this loop if not flush and Size() < kMaxReadSize
    size_t buffSize = _mp3Buffer->GetContiguousSize(); // guaranteed to be at least min{Size(), kMaxReadSize}
    const unsigned char* inputBuff = (buffSize == 0) ? nullptr : _mp3Buffer->ReadData( (int)buffSize );

    if( inputBuff == nullptr ) {
      // need to wait for more data
      break;
    }

    int samples = mp3dec_decode_frame(&_mp3decoder, inputBuff, (int)buffSize, _decodedPcm, &info);

    int consumed = info.frame_bytes;
    if( info.frame_bytes > 0 ) {


      SaveMP3( inputBuff, consumed );

      bool success = _mp3Buffer->AdvanceCursor( consumed );
      if( !success ) {
        PRINT_NAMED_ERROR( "AlexaMediaPlayer.Decode.ReadPtr", "Could not move read pointer by %d (size=%zu)",
                           consumed, _mp3Buffer->Size() );
      }

      if( samples > 0 ) {
        // valid wav data!
        if( _firstPass ) {
          _firstPass = false;
          _idealBufferSampleSize = static_cast<size_t>( info.hz * kIdealBufferSize_sec);
          _minPlaybackBufferSize = static_cast<size_t>( info.hz * kMinBufferSize_sec );
          LOG( "DECODING: channels=%d, hz=%d, layer=%d, bitrate=%d - _minPlaybackBufferSize=%zu,_idealBufferSampleSize=%zu ",
              info.channels, info.hz, info.layer, info.bitrate_kbps,
              _minPlaybackBufferSize, _idealBufferSampleSize );

          // HACK
          if (_speexState != nullptr) {
            speex_resampler_destroy(_speexState);
            _speexState = nullptr;
          }
          if (_type == AlexaMediaPlayer::Type::TTS) {
            int error = 0;
            _speexState = speex_resampler_init(
                                               1, // num channels
                                               info.hz, // in rate, int
                                               16000, // out rate, int
                                               5, // quality 0-10
                                               &error);

            PRINT_NAMED_WARNING("AlexaMediaPlayer::Decode", "speex_resampler_init error %d", error);

            // Set callback for this thread
            SpeechRecognizerSystem::TriggerWordDetectedCallback callback = [this] (const AudioUtil::SpeechRecognizer::SpeechCallbackInfo& info) {
              _detectedTriggers_ms.push({_offset_ms + info.startTime_ms, _offset_ms + info.endTime_ms});
              LOG("Decode.Recognizer.Callback: offset, start, end %d %d %d",
                (int)_offset_ms, info.startTime_ms, info.endTime_ms);
            };
            _recognizer->SetCallback(callback);
          }
        }

        decoded_ms += 1000 * float(samples) / info.hz;

        // the lib provides # samples as per channel
        samples *= info.channels;

        if( info.channels == 2 ) {
          // audio engine only supports mono
          int j = 0;
          for( int i=0; i<samples; i += 2, j += 1) {
            short left = _decodedPcm[i];
            short right = _decodedPcm[i + 1];
            short monoSample = (int(left) + right) / 2;
            _decodedPcm[j] = monoSample;
          }
          samples = samples/2;
          info.channels = 1;
        }

//        if( samples > 0 ) {
//          SavePCM( pcm, samples );
//        }

        // todo: minimp3 should be able to output in float to avoid this allocation and copy, but it
        // seems to clip for me when in float
        StandardWaveDataContainer waveContainer(info.hz, info.channels, samples);
        waveContainer.CopyWaveData( _decodedPcm, samples );

        data->AppendStandardWaveData( std::move(waveContainer) );

        // HACK
        if ( _speexState != nullptr ) {

          // Resample stream audio for detector
//          ANKI_CPU_PROFILE("ResampleAudioChunk");
          uint32_t numSamplesProcessed = samples;
          uint32_t numSamplesWritten = _kResampleMaxSize;
          speex_resampler_process_interleaved_int(_speexState,
                                                  _decodedPcm, &numSamplesProcessed,
                                                  _resampledPcm, &numSamplesWritten);
          ANKI_VERIFY(numSamplesProcessed == samples,
                      "MicDataProcessor.ResampleAudioChunk.SamplesProcessed",
                      "Expected %d processed only processed %d", samples, numSamplesProcessed);

          if( numSamplesWritten > 0 ) {
            _recognizer->Update(_resampledPcm, numSamplesWritten);
            SavePCM( _resampledPcm, numSamplesWritten );
          }
        }

        const auto numFrames = data->GetNumberOfFramesReceived();
        if( (_state == State::Preparing) && ((numFrames >= _minPlaybackBufferSize) || flush) ) {
         // LOG("Now Playable, numFrames %d minFrames %zu flush %d", numFrames, _minPlaybackBufferSize, flush );
          SetState(State::Playable);
        }
      }
    } else {
      // no frame bytes
      break;
    }
  }

  return int(decoded_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const AlexaMediaPlayer::StateToString() const
{
  switch( _state ) {
    case State::Idle: return "Idle";
    case State::Preparing: return "Preparing";
    case State::Playable: return "Playable";
    case State::Playing: return "Playing";
    case State::Paused: return "Paused";
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::SavePCM( short* buff, size_t size ) const
{
#if ANKI_DEV_CHEATS
  if( !kSaveDebugAudio ) {
    return;
  }
  static int pcmfd = -1;
  if( pcmfd < 0 ) {
    const auto path = _saveFolder + "speaker_" + _audioInfo.name + std::to_string(_playingSource) + ".pcm";
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
  if( !kSaveDebugAudio ) {
    return;
  }
  static int mp3fd = -1;
  if( mp3fd < 0 ) {
    const auto path = _saveFolder + "speaker_" + _audioInfo.name + std::to_string(_playingSource) + ".mp3";
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
  _settings.volume = volume;
  LOG( "setting volume to %d", _settings.volume );
  SetPlayerVolume( _settings.volume / 100.0f );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::adjustVolume( int8_t delta )
{
  _settings.volume += delta;
  if( _settings.volume < avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN;
  }
  if( _settings.volume > avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  }
  LOG( "adjusting volume by %d to %d", delta, _settings.volume );
  SetPlayerVolume( _settings.volume / 100.0f );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::setMute( bool mute )
{
  _settings.mute = mute;
  LOG( "setting mute=%d ", mute );
  const int8_t volume = _settings.mute ? 0 : _settings.volume;
  SetPlayerVolume( volume / 100.0f );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::onError()
{
  LOG( "An error occurred in the streaming of content UNIMPLEMENTED" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::doShutdown()
{
  // TODO: stop any audio and threads
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
void AlexaMediaPlayer::UpdateDetectorState(float& inout_lastPlayedMs)
{
  auto playedMs = _waveData->GetSampleRate() > 0 ?
  (1000 * (float)_waveData->GetNumberOfFramesPlayed() / _waveData->GetSampleRate()) : 0;
  if (!_detectedTriggers_ms.empty()) {
    const auto& firstTrigger = _detectedTriggers_ms.front();
    if (inout_lastPlayedMs < firstTrigger.first && playedMs >= firstTrigger.first) {
      LOG("Disabling wake word @ %d ms", (int)playedMs);
      _speechRegSys->DisableAlexaTemporarily();
    }
    else if (inout_lastPlayedMs < firstTrigger.second && playedMs >= firstTrigger.second) {
      LOG("Re-enabling wake word @ %d ms", (int)playedMs);
      _detectedTriggers_ms.pop();
      _speechRegSys->ReEnableAlexa();
    }
  }
  inout_lastPlayedMs = playedMs;
}


} // namespace Vector
} // namespace Anki
