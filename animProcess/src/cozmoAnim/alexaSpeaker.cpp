#include "cozmoAnim/alexaSpeaker.h"
#include "util/logging/logging.h"

#include <fcntl.h>
#include <unistd.h>
#include <array>

#define MINIMP3_IMPLEMENTATION
#if defined(VICOS)
  #define HAVE_SSE 0
#endif
#include "cozmoAnim/minimp3.h"
#include "cozmoAnim/audioDataBuffer.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"

#include "cozmoAnim/audio/cozmoAudioController.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioSwitchTypes.h"

#include "util/dispatchQueue/dispatchQueue.h"
#include "cozmoAnim/animContext.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"

#include <AVSCommon/AVS/SpeakerConstants/SpeakerConstants.h>
#include <AVSCommon/Utils/SDS/ReaderPolicy.h>

namespace Anki {
namespace Vector{
  using namespace alexaClientSDK;
  using SourceId = AlexaSpeaker::SourceId;
  
SourceId AlexaSpeaker::m_sourceID=1; // 0 is invalid
  
  namespace {
    constexpr int kAudioBufferSize = 1 << 18;
    constexpr size_t kMaxReadSize = 8192;
    mp3dec_t mp3d;
    
    constexpr uint32_t kMinPlayableFrames = 10*8192;
    constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kAlexaPluginId = 5;
    
    const auto kGameObject = AudioEngine::ToAudioGameObject( AudioMetaData::GameObjectType::Default );
    #define LOG(x, ...) PRINT_NAMED_WARNING("WHATNOW", "Speaker_%s: " x, _name.c_str(), ##__VA_ARGS__)
    
    #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)
  }



AlexaSpeaker::AlexaSpeaker( avsCommon::sdkInterfaces::SpeakerInterface::Type type,
                           const std::string& name,
                           std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory )
  : _type( type )
  , _name( name )
  , _dispatchQueue(Util::Dispatch::Create("AlexaSpeaker"))
  
{
  _mp3Buffer.reset( new AudioDataBuffer{kAudioBufferSize,kMaxReadSize} );
  
  _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  _settings.mute = false;
  
  m_contentFetcherFactory = contentFetcherFactory;
}

AlexaSpeaker::~AlexaSpeaker()
{
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
}
  
void AlexaSpeaker::Init(const AnimContext* context)
{
  DEV_ASSERT(nullptr != context, "RadioAudioComponent.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "RadioAudioComponent.InvalidAudioController");
  
  _audioController  = context->GetAudioController();
  
  mp3dec_init(&mp3d);
  
  const Util::Data::DataPlatform* platform = context->GetDataPlatform();
  std::string saveFolder = platform->pathToResource( Util::Data::Scope::Cache, "" );
  saveFolder = Util::FileUtils::AddTrailingFileSeparator( saveFolder );
  for( int i=1; i<100; ++i ) {
    std::string ss = saveFolder + "speaker_" + _name + std::to_string(i) + ".mp3";
    //PRINT_NAMED_WARNING("WHATNOW", "looking for file %s", ss.c_str());
    if( Util::FileUtils::FileExists(ss) ) {
      Util::FileUtils::DeleteFile( ss );
      Util::FileUtils::DeleteFile( saveFolder + "speaker_" + _name + std::to_string(i) + ".pcm" );
    }
  }
}

void AlexaSpeaker::Update()
{
  if( _audioController == nullptr ) {
    return; // not init yet
  }
  
  
  
  bool needsToPlay = false;
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    
    if( _state == State::Playable ) {
      LOG("Update: needsToPlay");
      needsToPlay = true;
      SetState( State::Playing );
    } 
  }
  
  if( needsToPlay ) {

    // post event
    {
      using namespace Anki::AudioMetaData;
      using namespace AudioEngine;
      GameEvent::GenericEvent event = GameEvent::GenericEvent::Invalid;
      if( _name == "TTS" ) {
        event = GameEvent::GenericEvent::Play__Dev_Robot_Vic__External_Alexa_Playback_01;
      } else if( _name == "Alerts" ) {
        event = GameEvent::GenericEvent::Play__Dev_Robot_Vic__External_Alexa_Playback_02;
      } else if( _name == "Audio" ) {
        event = GameEvent::GenericEvent::Play__Dev_Robot_Vic__External_Alexa_Playback_03;
      }
      const auto eventID = ToAudioEventId( event );
      auto* callbackContext = new AudioCallbackContext();
      callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
      callbackContext->SetExecuteAsync( true );
      callbackContext->SetEventCallbackFunc( [this]( const AudioCallbackContext* thisContext, const AudioCallbackInfo& callbackInfo ) {
        PRINT_NAMED_ERROR("WHATNOW", "Audio finished streaming (callback) %llu %hhu", callbackInfo.gameObjectId, callbackInfo.callbackType);
        CallOnPlaybackFinished( m_playingSource );
      });

      const auto playingID = _audioController->PostAudioEvent(eventID, kGameObject, callbackContext );
      if (AudioEngine::kInvalidAudioPlayingId == playingID) {
        PRINT_NAMED_ERROR("WHATNOW", "Could not play (post)");
      }
    }
  }
  
}
  
void AlexaSpeaker::CallOnPlaybackFinished( SourceId id )
{
  m_executor.submit([this, id]() {
    LOG("CallOnPlaybackFinished. Setting state=Idle");
    SetState(State::Idle);
    for( auto& observer : m_observers ) {
      LOG( "calling onPlaybackFinished for source %d", (int)id);
      observer->onPlaybackFinished( id );
    }
  });
}
  
SourceId  AlexaSpeaker::setSource (std::shared_ptr< avsCommon::avs::attachment::AttachmentReader > attachmentReader,
                                   const avsCommon::utils::AudioFormat *format)
{
  // todo:
  /* Implementations must make a call to onPlaybackStopped() with the previous SourceId when a new source is set if the previous source was in a non-stopped state. Any calls to a MediaPlayerInterface after an onPlaybackStopped() call will fail, as the MediaPlayer has "reset" its state.*/
  LOG( " speaker %d set source1", (int)_type);
  m_sourceReaders[m_sourceID] = std::move(attachmentReader);
  _sourceTypes[m_sourceID] = SourceType::AttachmentReader;
  
  using AudioFormat = avsCommon::utils::AudioFormat;
  if( format !=  nullptr) {
    std::stringstream ss;
    ss << "encoding=" << format->encoding << ", endianness=" << format->endianness;
    LOG( "format: %s, sampleRateHz=%u, sampleSize=%u, numChannels=%u, dataSigned=%d, interleaved=%d",
                        ss.str().c_str(), format->sampleRateHz, format->sampleSizeInBits, format->numChannels, format->dataSigned, format->layout == AudioFormat::Layout::INTERLEAVED);
  }
  
  return m_sourceID++;
}

SourceId   AlexaSpeaker::setSource (const std::string &url, std::chrono::milliseconds offset)
{
  LOG( " speaker %d set source2 = %s", (int)_type, url.c_str());
  if( url.empty() ) {
    return 0;
  }
  
  
  if( m_urlConverter != nullptr ) {
    m_urlConverter->shutdown();
  }
  m_urlConverter = playlistParser::UrlContentToAttachmentConverter::create(m_contentFetcherFactory, url, shared_from_this(), offset);
  if (!m_urlConverter) {
    ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "badUrlConverter"));
    return 0;
  }
  auto attachment = m_urlConverter->getAttachment();
  if (!attachment) {
    ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "badAttachmentReceived"));
    return 0;
  }
  std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader = attachment->createReader(avsCommon::utils::sds::ReaderPolicy::NONBLOCKING);
  if (!reader) {
    ACSDK_ERROR(LX("setSourceUrlFailed").d("reason", "failedToCreateAttachmentReader"));
    return 0;
  }
  
  return setSource(reader, nullptr);
}

SourceId   AlexaSpeaker::setSource (std::shared_ptr< std::istream > stream, bool repeat)  {
  LOG( " speaker %d set source3", (int)_type);
  _sourceTypes[m_sourceID] = SourceType::Stream;
  m_sourceStreams[m_sourceID] = stream;
  return m_sourceID++;
}

bool   AlexaSpeaker::play (SourceId id)
{
  LOG( " speaker play");
  m_playingSource = id;
  
  // is the callback supposed to be on the caller thread? forgot what docs said
  //m_executor.submit([id, this]() {
  Util::Dispatch::Async(_dispatchQueue, [id, this](){
    LOG("Setting state=Preparing");
    SetState(State::Preparing);
    
    // this is buggy af. don't look
    
    using AttachmentReader = avsCommon::avs::attachment::AttachmentReader;
    
    static int _fd = -1;
    
    bool sent=false;
    _offset_ms = 0;
    _first = true;
    
    _waveData = AudioEngine::PlugIns::StreamingWavePortalPlugIn::CreateDataInstance();
    {
      auto* pluginInterface = _audioController->GetPluginInterface();
      DEV_ASSERT(nullptr != pluginInterface, "TextToSpeechComponent.PrepareAudioEngine.InvalidPluginInterface");
      auto* plugin = pluginInterface->GetStreamingWavePortalPlugIn();
      int pluginID = kAlexaPluginId;
      if( _name == "Alerts" ) {
        ++pluginID;
      } else if( _name == "Audio" ) {
        pluginID += 2;
      }
      plugin->ClearAudioData( pluginID );
      plugin->AddDataInstance(_waveData, pluginID);
    }
    
    _mp3Buffer->Reset();
    
    std::array<uint8_t,kMaxReadSize> input;
    
    int numBlocks = 0;
    const int kMaxBlocks = 10000;
    while( (_state == State::Preparing) || (_state == State::Playing) ) {
      
      if( !sent ) {
        for( auto& observer : m_observers ) {
          LOG( "calling onPlaybackStarted for source %d", (int)id);
          observer->onPlaybackStarted( id );
        }
        sent = true;
      }
      
      bool statusOK = false;
      size_t numRead = 0;
      AttachmentReader::ReadStatus readStatus;
      if( _sourceTypes[id] == SourceType::AttachmentReader ) {
        const size_t size = (size_t)m_sourceReaders[id]->getNumUnreadBytes();
        if( size == 0 ) {
          // this can either happen when the download hasnt started, or when it finished. use the readStatus to figure out which
          LOG( "nothing to read yet!");
        }
        
        // keep going if size to read is 0, since it's the readStatus that matters
        const size_t toRead = (size == 0) ? input.size() : std::min(size, input.size());
        
        readStatus = AttachmentReader::ReadStatus::OK;
        numRead = m_sourceReaders[id]->read( input.data(), toRead, &readStatus );
        if( numRead > 0 ) {
          _mp3Buffer->AddData( input.data(), toRead );
        }
        
        {
          std::stringstream ss;
          ss << readStatus;
          LOG( "expected %d read %d, status %s", size, numRead, ss.str().c_str());
        }
        statusOK = (readStatus == AttachmentReader::ReadStatus::OK)
                              || (readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK)
                              || (readStatus == AttachmentReader::ReadStatus::OK_TIMEDOUT);
        if( readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK && ++numBlocks > kMaxBlocks ) {
          PRINT_NAMED_WARNING("WHATNOW", "Too many OK_WOULDBLOCK. Canceling");
          statusOK = false;
        } else if( readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK ) {
          // whatever
          std::this_thread::sleep_for( std::chrono::milliseconds(50) );
        }
        
      } else if( _sourceTypes[id] == SourceType::Stream ) {
        static std::vector<uint8_t> buffer;
        
        
        char firstChar = 0;
        m_sourceStreams[id]->get(firstChar);
        size_t available = m_sourceStreams[id]->rdbuf()->in_avail();
        size_t size = std::min(kMaxReadSize - 2, available); // -2 for newline and null term
        if( size > 0 ) {
          input[0] = firstChar;
          m_sourceStreams[id]->read((char*)input.data() + 1, size);
          // todo: dont add if full! curently we sleep instead
          _mp3Buffer->AddData( input.data(), size );
        }
        numRead = size;
        statusOK = !m_sourceStreams[id]->eof();
        LOG( "istream read %d bytes, eof=%d", size, !statusOK);
      }
      
      
      bool flush = !statusOK;
      //if( (numRead > 0) && statusOK ) {
      LOG( "flush=%d", flush);
      // decode everything in the buffer thus far, leaving perhaps something that wasnt a full mp3 frame
      const int timeDecoded_ms = Decode( _waveData, flush );
      _offset_ms += timeDecoded_ms;
      LOG( "decoded %d ms for a total offset %llu", timeDecoded_ms, _offset_ms);
      
      if (_fd < 0) {
        const auto path = "/data/data/com.anki.victor/cache/speaker_" + _name + std::to_string(m_playingSource) + ".mp3";
        _fd = open(path.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0644);
      }
      (void) write(_fd, input.data(), numRead * sizeof(uint8_t));
      
      const int sleepFor_ms = 0.3f * timeDecoded_ms;
      
      if( !flush && sleepFor_ms > 0 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(sleepFor_ms) );
      }
    
      if( flush ) {
        _waveData->DoneProducingData();
        if( _sourceTypes[id] == SourceType::AttachmentReader ) {
          std::stringstream ss;
          ss << readStatus;
          LOG( "ending because readStatus=%s", ss.str().c_str());
        } else {
          LOG( "ending (flush)");
        }
        break;
      }
      
      LOG( "state=%s", StateToString());
    }
    
    SavePCM( nullptr );
    //_offset_ms = 0;
    //_mp3Buffer->Reset();
    
    {
      std::lock_guard<std::mutex> lock{ _mutex };
    
      if( _state == State::Preparing ) {
        LOG( "done with loop, setting to Playable because was still preparing");
        SetState(State::Playable);
//      } else if( _state != State::Playable ) {
//        LOG( "done with loop, setting to Idle");
//        _state = State::Idle;
      }
    }
    
    if( _sourceTypes[id] == SourceType::AttachmentReader ) {
      m_sourceReaders[id]->close();
      m_sourceReaders[id].reset();
    } else if( _sourceTypes[id] == SourceType::Stream ) {
      m_sourceStreams[id].reset();
    }
    
    
    if( _fd >= 0 ) {
      close(_fd);
      _fd = -1;
    }
  
  });
  
  return true;
}

 bool   AlexaSpeaker::stop (SourceId id)  {
  LOG( " speaker stop");
  
  _audioController->StopAllAudioEvents( kGameObject );
   
  m_executor.submit([id, this]() {
    
    //if( _state == State::Playing ) {
      LOG("Setting _state=Idle");
    SetState(State::Idle);
    //}
    _offset_ms = 0;
    for( auto& observer : m_observers ) {
      LOG( "calling onPlaybackStopped for source %d", (int)id);
      observer->onPlaybackStopped( id );
    }
  });
  return true;
}

bool   AlexaSpeaker::pause (SourceId id)  {
  LOG( " speaker pause");
  if( _sourceTypes.find( id ) == _sourceTypes.end() ) {
    return false;
  }
  if( _state == State::Idle ) {
    return false;
  }
  
  m_executor.submit([id, this]() {
    for( auto& observer : m_observers ) {
      LOG( "calling onPlaybackPaused for source %d", (int)id);
      observer->onPlaybackPaused( id );
    }
  });
  return true;
}

bool AlexaSpeaker::resume (SourceId id)  {
  LOG( " speaker resume");
  
  if( _sourceTypes.find( id ) == _sourceTypes.end() ) {
    return false;
  }
  if( _state != State::Idle ) {
    return false;
  }
  
  m_executor.submit([id, this]() {
    for( auto& observer : m_observers ) {
      LOG( "calling onPlaybackResumed for source %d", (int)id);
      observer->onPlaybackResumed( id );
    }
  });
  return true;
}

std::chrono::milliseconds   AlexaSpeaker::getOffset (SourceId id)  {
  LOG( " speaker getOffset");
    
  return std::chrono::milliseconds{_offset_ms};
}

 uint64_t   AlexaSpeaker::getNumBytesBuffered ()  {
  LOG( " speaker getNumBytesBuffered");
  return 0;
}

 void   AlexaSpeaker::setObserver (std::shared_ptr< avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface > playerObserver)  {
  LOG( " speaker setObserver");
  m_observers.insert( playerObserver );
}
  
int AlexaSpeaker::Decode(const StreamingWaveDataPtr& data, bool flush)
{
  mp3dec_frame_info_t info;
  
  using namespace AudioEngine;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  
  float millis_decoded = 0.0f;
  
  while( true ) {
    
    int buffSize;
    if( flush ) {
      buffSize = _mp3Buffer->Size();
    } else {
      buffSize = kMaxReadSize; // doing a min causes clipping //std::min(_mp3Buffer->Size(), kMaxReadSize);
    }
    const bool debugRead = flush;
    unsigned char* inputBuff = _mp3Buffer->ReadData(buffSize, debugRead);
    
    if( buffSize == 0 || inputBuff == nullptr ) {
      // need to wait for more data
      LOG( "Needs more data (%s) for size %d (actual size=%d)", StateToString(), buffSize, _mp3Buffer->Size() );
      break;
    }
    
    int samples = mp3dec_decode_frame(&mp3d, inputBuff, buffSize, pcm, &info);
    //std::cout << "samples=" << samples << ", frame_bytes=" << info.frame_bytes << std::endl;
    int consumed = info.frame_bytes;
    if( info.frame_bytes > 0  ) {
      //LOG( "Advancing cursor by %d", consumed);
      bool success = _mp3Buffer->AdvanceCursor( consumed );
      ANKI_VERIFY(success, "WHATNOW", "Could not advance by %d", consumed);
      
      if( samples > 0 ) {
        // valid wav data!
        if( _first ) {
          _first = false;
          LOG( "DECODING: channels=%d, hz=%d, layer=%d, bitrate=%d", info.channels, info.hz, info.layer, info.bitrate_kbps);
        }
        
        millis_decoded += 1000 * float(samples) / info.hz;
        
        // the lib provides # samples as per channel
        // omfg this took forever to find
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
        
        {
          std::lock_guard<std::mutex> lock{ _mutex };
          const auto numFrames = data->GetNumberOfFramesReceived();
          //LOG( "LOOP: decoded. available frames=%d. decoded %d ms, state=%s", numFrames, (int)millis_decoded, StateToString());
          if( (_state == State::Preparing) && numFrames >= kMinPlayableFrames ) {
            LOG("Setting state=Playable");
            SetState(State::Playable);
          }
        }

        //outData.insert( outData.end(), std::begin(pcm), std::begin(pcm) + samples );
      } else {
        LOG( "LOOP: skipped (frame_bytes=%d), state=%s", info.frame_bytes, StateToString() );
        //std::cout << "skipped" << std::endl;
      }
    } else {
      LOG( "LOOP: no bytes (%s)", StateToString() );
      // need to wait for more data
      //std::cout << "done!" << std::endl;
      //PrintInfo(info);
      break;
    }
  }
  
  return int(millis_decoded);
}
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const char* const AlexaSpeaker::StateToString() const
  {
    switch( _state ) {
      case State::Idle: return "Idle";
      case State::Preparing: return "Preparing";
      case State::Playable: return "Playable";
      case State::Playing: return "Playing";
      //case State::Stopping: return "Stopping";
    };
  }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaSpeaker::SavePCM( short* buff, size_t size )
{
  //LOG( "saving pcm size=%zu", size);
  static int pcmfd = -1;
  if (pcmfd < 0) {
    const auto path = "/data/data/com.anki.victor/cache/speaker_" + _name + std::to_string(m_playingSource) + ".pcm";
    pcmfd = open(path.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0644);
  }
  
  if( size > 0 && (buff != nullptr) ) {
    (void) write(pcmfd, buff, size * sizeof(short));
  } else {
    close(pcmfd);
    pcmfd = -1;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaSpeaker::getSpeakerSettings (SpeakerSettings *settings)
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
bool AlexaSpeaker::setVolume (int8_t volume)
{
  _settings.volume = volume;
  LOG( "AlexaSpeaker %d settings volume to %d", (int)_type, _settings.volume );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaSpeaker::adjustVolume (int8_t delta)
{
  _settings.volume += delta;
  if( _settings.volume < avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MIN;
  }
  if( _settings.volume > avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX ) {
    _settings.volume = avsCommon::avs::speakerConstants::AVS_SET_VOLUME_MAX;
  }
  LOG( "AlexaSpeaker %d adjusting volume by %d to %d", (int)_type, delta, _settings.volume );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaSpeaker::setMute (bool mute)
{
  _settings.mute = mute;
  LOG( "AlexaSpeaker %d setting mute=%d", (int)_type, mute );
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaSpeaker::onError ()
{
  LOG("An error occurred in the streaming of content");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaSpeaker::SetState( State state )
{
  _state = state;
  LOG("Update: %s", StateToString());
}
  
} // namespace Vector
} // namespace Anki

