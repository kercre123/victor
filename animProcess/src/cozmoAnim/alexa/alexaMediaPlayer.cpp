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


#include "cozmoAnim/alexa/alexaMediaPlayer.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "util/console/consoleInterface.h"
#include "util/container/ringBuffContiguousRead.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/SDS/ReaderPolicy.h>

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

namespace {
  // TODO (VIC-9853): this is too big because we read all data from an internet stream instead of only when wwise needs new data.
  constexpr int kAudioBufferSize = 1 << 18;
  
  // max read size for _mp3Buffer
  constexpr size_t kMaxReadSize = 8192;
  
  // context for mp3 decoder
  mp3dec_t mp3d;
  
  // how many frames should be buffered before hitting "play" in wwise
  constexpr uint32_t kMinPlayableFrames = 81920;
  
  using AGE = AudioMetaData::GameEvent::GenericEvent;
  constexpr AGE kGameObjectTTS = AGE::Play__Robot_Vic__External_Alexa_Playback_Voice;
  constexpr AGE kGameObjectAudio = AGE::Play__Robot_Vic__External_Alexa_Playback_Media;
  constexpr AGE kGameObjectAlerts = AGE::Play__Robot_Vic__External_Alexa_Playback_Alerts;
  constexpr AGE kGameObjectNotifications = AGE::Play__Robot_Vic__External_Alexa_Playback_Notifications;
  constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kPluginIdTTS = 10;
  constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kPluginIdAudio = 11;
  constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kPluginIdAlerts = 12;
  constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kPluginIdNotifications = 13;
  // TODO (VIC-11585): use a different game object
  const auto kGameObject = AudioEngine::ToAudioGameObject( AudioMetaData::GameObjectType::Default );
  
  #define LOG_CHANNEL "Alexa"
  #define LOG(x, ...) LOG_INFO("Alexa.SpeakerInfo", "%s: " x, _name.c_str(), ##__VA_ARGS__)
#if ANKI_DEV_CHEATS
  const bool kSaveDebugAudio = true;
#endif
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMediaPlayer::AlexaMediaPlayer( avsCommon::sdkInterfaces::SpeakerInterface::Type type,
                                    const std::string& name,
                                    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory )
  : avsCommon::utils::RequiresShutdown{"AlexaMediaPlayer_" + name}
  , _type( type )
  , _name( name )
  , _dispatchQueue(Util::Dispatch::Create("AlexaMediaPlayer"))
{
  _mp3Buffer.reset( new Util::RingBuffContiguousRead<uint8_t>{ kAudioBufferSize, kMaxReadSize } );
  
  _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  _settings.mute = false;
  
  _contentFetcherFactory = contentFetcherFactory;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMediaPlayer::~AlexaMediaPlayer()
{
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::Init( const AnimContext* context )
{
  DEV_ASSERT(nullptr != context, "RadioAudioComponent.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "RadioAudioComponent.InvalidAudioController");
  
  _audioController  = context->GetAudioController();
  DEV_ASSERT_MSG( _audioController != nullptr, "AudioMediaPlayer.Init.InvalidAudioController", "" );
  
  mp3dec_init( &mp3d );
  
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
      std::string ss = _saveFolder + "speaker_" + _name + std::to_string(i) + ".mp3";
      if( Util::FileUtils::FileExists(ss) ) {
        Util::FileUtils::DeleteFile( ss );
        Util::FileUtils::DeleteFile( _saveFolder + "speaker_" + _name + std::to_string(i) + ".pcm" );
      } else {
        break;
      }
    }
#endif
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
    using namespace Anki::AudioMetaData;
    using namespace AudioEngine;
    GameEvent::GenericEvent event = GameEvent::GenericEvent::Invalid;
    if( _name == "TTS" ) {
      event = kGameObjectTTS;
    } else if( _name == "Alerts" ) {
      event = kGameObjectAlerts;
    } else if( _name == "Audio" ) {
      event = kGameObjectAudio;
    } else if( _name == "Notifications" ) {
      event = kGameObjectNotifications;
    }
    const auto eventID = ToAudioEventId( event );
    auto* callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread
    callbackContext->SetEventCallbackFunc( [this]( const AudioCallbackContext* thisContext, const AudioCallbackInfo& callbackInfo ) {
      CallOnPlaybackFinished( _playingSource );
    });

    const auto playingID = _audioController->PostAudioEvent(eventID, kGameObject, callbackContext );
    if (AudioEngine::kInvalidAudioPlayingId == playingID) {
      PRINT_NAMED_ERROR( "AlexaMediaPlayer.Update.CouldNotPlay", "Speaker '%s' could not play", _name.c_str() );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMediaPlayer::CallOnPlaybackFinished( SourceId id )
{
  _executor.submit([this, id]() {
    LOG( "CallOnPlaybackFinished. Setting state=Idle" );
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
  _sourceReaders[_sourceID] = std::move(attachmentReader);
  _sourceTypes[_sourceID] = SourceType::AttachmentReader;
  
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
  
  return setSource(reader, nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SourceId AlexaMediaPlayer::setSource( std::shared_ptr<std::istream> stream, bool repeat )
{
  LOG( "set source with stream" );

  _sourceTypes[_sourceID] = SourceType::Stream;
  _sourceStreams[_sourceID] = stream;
  return _sourceID++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::play( SourceId id )
{
  LOG( "play" );
  _playingSource = id;
  
  Util::Dispatch::Async( _dispatchQueue, [id, this](){
    SetState(State::Preparing);
    
    using AttachmentReader = avsCommon::avs::attachment::AttachmentReader;
    
    bool sentPlaybackStarted = false;
    
    // reset vars
    _offset_ms = 0;
    _firstPass = true;
    _mp3Buffer->Reset();
    
    // new waveData instance to hold data passed to wwise
    _waveData = AudioEngine::PlugIns::StreamingWavePortalPlugIn::CreateDataInstance();
    // reset plugin
    {
      auto* pluginInterface = _audioController->GetPluginInterface();
      DEV_ASSERT(nullptr != pluginInterface, "TextToSpeechComponent.PrepareAudioEngine.InvalidPluginInterface");
      auto* plugin = pluginInterface->GetStreamingWavePortalPlugIn();
      int pluginID = 0;
      if( _name == "TTS" ) {
        pluginID = kPluginIdTTS;
      } else if( _name == "Alerts" ) {
        pluginID = kPluginIdAlerts;
      } else if( _name == "Audio" ) {
        pluginID = kPluginIdAudio;
      } else if( _name == "Notifications" ) {
        pluginID = kPluginIdNotifications;
      }
      plugin->ClearAudioData( pluginID );
      plugin->AddDataInstance( _waveData, pluginID );
    }
    
    // buffer to read mp3 data from source. this is an unnecessary copy if RingBuffContiguousRead had a
    // raw writable pointer
    std::array<uint8_t,kMaxReadSize> input;
    
    int numBlocks = 0;
    const int kMaxBlocks = 10000; // number of times we can try to check a blocking reader before giving up
    
    while( (_state == State::Preparing) || (_state == State::Playing) || (_state == State::Playable) ) {
      
      if( !sentPlaybackStarted ) {
        for( auto& observer : _observers ) {
          LOG( "source %d onPlaybackStarted", (int)id);
          observer->onPlaybackStarted( id );
        }
        sentPlaybackStarted = true;
      }
      
      // read from one of the sources
      bool statusOK = false;
      size_t numRead = 0;
      AttachmentReader::ReadStatus readStatus;
      
      if( _sourceTypes[id] == SourceType::AttachmentReader ) {
        const size_t size = (size_t)_sourceReaders[id]->getNumUnreadBytes();
        // it's possible that size==0 if the download hasn't started. ignore size for now, and try reading below,
        // and use the readStatus of that to determine what to do
        
        const size_t toRead = (size == 0) ? input.size() : std::min(size, input.size());
        
        readStatus = AttachmentReader::ReadStatus::OK;
        numRead = _sourceReaders[id]->read( input.data(), toRead, &readStatus );
        if( numRead > 0 ) {
          // this won't add if it's full! which is not what we want unless it's a streaming radio station
          _mp3Buffer->AddData( input.data(), (int)toRead );
        }
        
        statusOK = (readStatus == AttachmentReader::ReadStatus::OK)
                              || (readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK)
                              || (readStatus == AttachmentReader::ReadStatus::OK_TIMEDOUT);
        if( readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK && (++numBlocks > kMaxBlocks) ) {
          // Too many OK_WOULDBLOCKs. Canceling
          statusOK = false;
        } else if( readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK ) {
          // todo: not this
          std::this_thread::sleep_for( std::chrono::milliseconds(50) );
        }
        
      } else if( _sourceTypes[id] == SourceType::Stream ) {
        // read one char so that we can then check the available data in the istream.
        char firstChar = 0;
        _sourceStreams[id]->get(firstChar);
        size_t available = _sourceStreams[id]->rdbuf()->in_avail();
        size_t size = std::min(kMaxReadSize - 2, available); // -2 for newline and null term
        if( size > 0 ) {
          input[0] = firstChar;
          _sourceStreams[id]->read( (char*)input.data() + 1, size );
          // this won't add if it's full! which is not what we want unless it's a streaming radio station
          _mp3Buffer->AddData( input.data(), (int)size );
        }
        numRead = size;
        statusOK = !_sourceStreams[id]->eof();
      } else {
        ANKI_VERIFY( false && "Unexpected source type", "AlexaMediaPlayer.play.UnexpectedSourceType", "" );
      }
      
      // if flush, then any data left in the ring buff will get decoded
      const bool flush = !statusOK;
      // decode everything in the buffer thus far, leaving only something that perhaps wasnt a full mp3 frame
      const int timeDecoded_ms = Decode( _waveData, flush );
      _offset_ms += timeDecoded_ms;
      
      // sleep to avoid downloading data faster than it is played. todo: not this...
      const int sleepFor_ms = 0.8f * timeDecoded_ms;
      if( !flush && sleepFor_ms > 0 ) {
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
    
    if( _sourceTypes[id] == SourceType::AttachmentReader ) {
      _sourceReaders[id]->close();
      _sourceReaders[id].reset();
    } else if( _sourceTypes[id] == SourceType::Stream ) {
      _sourceStreams[id].reset();
    }
    
  
  });
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::stop( SourceId id )
{
  LOG( "stop" );
  
  _audioController->StopAllAudioEvents( kGameObject );
   
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
  LOG( "pause NOT IMPLEMENTED" );
  
  if( _sourceTypes.find( id ) == _sourceTypes.end() ) {
    return false;
  }
  
  if( _state == State::Idle ) {
    return false;
  }
  
  _executor.submit([id, this]() {
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackPaused for source %d", (int)id );
      observer->onPlaybackPaused( id );
    }
  });
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::resume( SourceId id )
{
  LOG( "resume NOT IMPLEMENTED" );
  
  if( _sourceTypes.find( id ) == _sourceTypes.end() ) {
    return false;
  }
  
  if( _state != State::Idle ) {
    return false;
  }
  
  _executor.submit([id, this]() {
    for( auto& observer : _observers ) {
      LOG( "calling onPlaybackResumed for source %d", (int)id );
      observer->onPlaybackResumed( id );
    }
  });
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
int AlexaMediaPlayer::Decode( const StreamingWaveDataPtr& data, bool flush )
{
  mp3dec_frame_info_t info;
  
  using namespace AudioEngine;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  
  float decoded_ms = 0.0f;
  
  while( true ) {
    
    // it would also make sense to exit this loop if not flush and Size() < kMaxReadSize
    size_t buffSize = std::min(_mp3Buffer->Size(), kMaxReadSize);
    const unsigned char* inputBuff = (buffSize == 0) ? nullptr : _mp3Buffer->ReadData( (int)buffSize );
    
    if( inputBuff == nullptr ) {
      // need to wait for more data
      break;
    }
    
    int samples = mp3dec_decode_frame(&mp3d, inputBuff, (int)buffSize, pcm, &info);
    int consumed = info.frame_bytes;
    if( info.frame_bytes > 0  ) {
      
      if( samples > 0 || flush ) {
        SaveMP3( inputBuff, consumed );
        
        bool success = _mp3Buffer->AdvanceCursor( consumed );
        if( !success ) {
          PRINT_NAMED_ERROR( "AlexaMediaPlayer.Decode.ReadPtr", "Could not move read pointer by %d (size=%zu)",
                             consumed, _mp3Buffer->Size() );
        }
      } else {
        // no samples
        break;
      }
      
      if( samples > 0 ) {
        // valid wav data!
        if( _firstPass ) {
          _firstPass = false;
          LOG( "DECODING: channels=%d, hz=%d, layer=%d, bitrate=%d", info.channels, info.hz, info.layer, info.bitrate_kbps );
          
        }
        
        decoded_ms += 1000 * float(samples) / info.hz;
        
        // the lib provides # samples as per channel
        samples *= info.channels;
        
        if( info.channels == 2 ) {
          // audio engine only supports mono
          int j = 0;
          for( int i=0; i<samples; i += 2, j += 1) {
            short left = pcm[i];
            short right = pcm[i + 1];
            short monoSample = (int(left) + right) / 2;
            pcm[j] = monoSample;
          }
          samples = samples/2;
          info.channels = 1;
        }
        
        if( samples > 0 ) {
          SavePCM( pcm, samples );
        }
      
        // todo: minimp3 should be able to output in float to avoid this allocation and copy, but it
        // seems to clip for me when in float
        StandardWaveDataContainer waveContainer(info.hz, info.channels, samples);
        waveContainer.CopyWaveData( pcm, samples );
        
        data->AppendStandardWaveData( std::move(waveContainer) );
        
        const auto numFrames = data->GetNumberOfFramesReceived();
        if( (_state == State::Preparing) && numFrames >= kMinPlayableFrames ) {
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
    const auto path = _saveFolder + "speaker_" + _name + std::to_string(_playingSource) + ".pcm";
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
    const auto path = _saveFolder + "speaker_" + _name + std::to_string(_playingSource) + ".mp3";
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
  LOG( "setting volume to %d UNIMPLEMENTED", _settings.volume );
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
  LOG( "adjusting volume by %d to %d UNIMPLEMENTED", delta, _settings.volume );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaMediaPlayer::setMute( bool mute )
{
  _settings.mute = mute;
  LOG( "setting mute=%d UNIMPLEMENTED", mute );
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
void AlexaMediaPlayer::SetState( State state )
{
  _state = state;
  LOG("State: %s", StateToString());
}


} // namespace Vector
} // namespace Anki
