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

#include "cozmoAnim/audio/cozmoAudioController.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioSwitchTypes.h"

#include "util/dispatchQueue/dispatchQueue.h"
#include "cozmoAnim/animContext.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"

namespace Anki {
namespace Vector{
  
  namespace {
    constexpr int kAudioBufferSize = 1 << 16;
    constexpr int kMaxReadSize = 4096;
    mp3dec_t mp3d;
    
    constexpr uint32_t kMinPlayableFrames = 8192;
  }

using namespace alexaClientSDK;
using SourceId = AlexaSpeaker::SourceId;

AlexaSpeaker::AlexaSpeaker()
  : _dispatchQueue(Util::Dispatch::Create("AlexaSpeaker"))
{
  _mp3Buffer.reset( new AudioDataBuffer{kAudioBufferSize,kMaxReadSize} );
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
}

void AlexaSpeaker::Update()
{
  if( _audioController == nullptr ) {
    return; // not init yet
  }
  
  
  bool needsToPlay = false;
  {
    //std::lock_guard<std::mutex> lock{ _mutex };
    if( _state == State::Playable ) {
      PRINT_NAMED_WARNING("WHATNOW","Update: needsToPlay");
      needsToPlay = true;
      _state = State::Playing;
    } else if ( _state == State::Preparing ) {
      PRINT_NAMED_WARNING("WHATNOW","Update: still preparing");
    }
  }
  
  if( needsToPlay ) {
    
    // reset plugin
    {
      auto* pluginInterface = _audioController->GetPluginInterface();
      DEV_ASSERT(nullptr != pluginInterface, "TextToSpeechComponent.PrepareAudioEngine.InvalidPluginInterface");
      auto* plugin = pluginInterface->GetStreamingWavePortalPlugIn();
      constexpr Anki::AudioEngine::PlugIns::StreamingWavePortalPlugIn::PluginId_t kTtsPluginId = 0;
      plugin->ClearAudioData(kTtsPluginId);
      // todo: lock for _waveData?
      plugin->AddDataInstance(_waveData, kTtsPluginId);
    }
    
    
    using namespace Anki::AudioMetaData;
    using namespace AudioEngine;
    using AudioTtsProcessingStyle = SwitchState::Robot_Vic_External_Processing;
    
    //const auto gameObject = AudioEngine::ToAudioGameObject( GameObjectType::VoiceRecording );
    //const auto eventID = AudioEngine::ToAudioEventId( GameEvent::GenericEvent::Play__Robot_Vic__External_Voice_Message );
    const auto gameObject = AudioEngine::ToAudioGameObject( GameObjectType::TextToSpeech );
    const auto eventID = AudioEngine::ToAudioEventId( GameEvent::GenericEvent::Play__Robot_Vic__External_Voice_Text );
    
    // set switch state
    {
      const auto switchGroup = AudioMetaData::SwitchState::SwitchGroupType::Robot_Vic_External_Processing;
      const auto style = AudioTtsProcessingStyle::Unprocessed;
      _audioController->SetSwitchState( static_cast<AudioEngine::AudioSwitchGroupId>(switchGroup),
                                       static_cast<AudioEngine::AudioSwitchStateId>(style),
                                       static_cast<AudioEngine::AudioGameObject>(gameObject) );
    }
    
    // post event
    {
      auto* callbackContext = new AudioCallbackContext();
      callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
      callbackContext->SetExecuteAsync( false ); // callback should run on main thread
      callbackContext->SetEventCallbackFunc( []( const AudioCallbackContext* thisContext, const AudioCallbackInfo& callbackInfo ) {
        PRINT_NAMED_ERROR("WHATNOW", "Audio finished streaming (callback)");
      });
      
      const auto playingID = _audioController->PostAudioEvent(eventID, gameObject );
      if (AudioEngine::kInvalidAudioPlayingId == playingID) {
        PRINT_NAMED_ERROR("WHATNOW", "Could not play (post)");
      }
    }
    
  }
  
}
  
SourceId  AlexaSpeaker::setSource (std::shared_ptr< avsCommon::avs::attachment::AttachmentReader > attachmentReader, const avsCommon::utils::AudioFormat *format)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker set source1");
  m_source = std::move(attachmentReader);
  
  using AudioFormat = avsCommon::utils::AudioFormat;
  if( format !=  nullptr) {
    std::stringstream ss;
    ss << "encoding=" << format->encoding << ", endianness=" << format->endianness;
    PRINT_NAMED_WARNING("WHATNOW", "format: %s, sampleRateHz=%u, sampleSize=%u, numChannels=%u, dataSigned=%d, interleaved=%d",
                        ss.str().c_str(), format->sampleRateHz, format->sampleSizeInBits, format->numChannels, format->dataSigned, format->layout == AudioFormat::Layout::INTERLEAVED);
  }
  
  return m_sourceID++;
}

SourceId   AlexaSpeaker::setSource (const std::string &url, std::chrono::milliseconds offset)
{
  PRINT_NAMED_WARNING("WHATNOW", " speaker set source2");
  return m_sourceID++;
}

SourceId   AlexaSpeaker::setSource (std::shared_ptr< std::istream > stream, bool repeat)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker set source3");
  return m_sourceID++;
}

bool   AlexaSpeaker::play (SourceId id)
{
  PRINT_NAMED_WARNING("WHATNOW", " speaker play");
  
  // is the callback supposed to be on the caller thread? forgot what docs said
  //m_executor.submit([id, this]() {
  Util::Dispatch::Async(_dispatchQueue, [id, this](){
    _state = State::Preparing;
    
    // this is buggy af. don't look
    
    using AttachmentReader = avsCommon::avs::attachment::AttachmentReader;
    
    static int _fd = -1;
    
    bool sent=false;
    _offset_ms = 0;
    _first = true;
    _waveData = AudioEngine::PlugIns::StreamingWavePortalPlugIn::CreateDataInstance();
    _mp3Buffer->Reset();
    
    std::array<uint8_t,kMaxReadSize> input;
    while( (_state == State::Preparing) || (_state == State::Playing) ) {
      
      if( !sent ) {
        for( auto& observer : m_observers ) {
          PRINT_NAMED_WARNING("WHATNOW", "calling onPlaybackStarted for source %d", (int)id);
          observer->onPlaybackStarted( id );
        }
        sent = true;
      }
      
      const size_t size = (size_t)m_source->getNumUnreadBytes();
      if( size == 0 ) {
        // this can either happen when the download hasnt started, or when it finished. use the readStatus to figure out which
        PRINT_NAMED_WARNING("WHATNOW", "nothing to read yet!");
      }
      
      // keep going if size to read is 0, since it's the readStatus that matters
      const size_t toRead = (size == 0) ? input.size() : std::min(size, input.size());
      
      auto readStatus = AttachmentReader::ReadStatus::OK;
      auto numRead = m_source->read( input.data(), toRead, &readStatus );
      if( numRead > 0 ) {
        _mp3Buffer->AddData( input.data(), toRead );
      }
      
      {
        std::stringstream ss;
        ss << readStatus;
        PRINT_NAMED_WARNING("WHATNOW", "expected %d read %d, status %s", size, numRead, ss.str().c_str());
      }
      
      const bool statusOK = (readStatus == AttachmentReader::ReadStatus::OK)
                            || (readStatus == AttachmentReader::ReadStatus::OK_WOULDBLOCK)
                            || (readStatus == AttachmentReader::ReadStatus::OK_TIMEDOUT);
      bool flush = (numRead == 0) || !statusOK;
      //if( (numRead > 0) && statusOK ) {
      PRINT_NAMED_WARNING("WHATNOW", "flush=%d", flush);
      // decode everything in the buffer thus far, leaving perhaps something that wasnt a full mp3 frame
      const int timeDecoded_ms = Decode( _waveData, flush );
      PRINT_NAMED_WARNING("WHATNOW", "decoded %d ms", timeDecoded_ms);
      
      if (_fd < 0) {
        const auto path = "/data/data/com.anki.victor/cache/speaker.mp3";
        _fd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
      }
      (void) write(_fd, input.data(), numRead * sizeof(uint8_t));
      
      const int sleepFor_ms = 0.3f * timeDecoded_ms;
      _offset_ms += timeDecoded_ms;
      if( !flush && sleepFor_ms > 0 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(sleepFor_ms) );
      }
    
      if( flush ) {
        _waveData->DoneProducingData();
        std::stringstream ss;
        ss << readStatus;
        PRINT_NAMED_WARNING("WHATNOW", "ending because readStatus=%s", ss.str().c_str());
//        // idk why this is needed
//        std::this_thread::sleep_for( std::chrono::milliseconds(3500) );
        break;
      }
      
      PRINT_NAMED_WARNING("WHATNOW", "state=%s", StateToString());
    }
    
    m_source->close();
    m_source.reset();
    _state = State::Idle;
    //if( _offset_ms > 0 ) {
    
    //}
    SavePCM( nullptr );
    _offset_ms = 0;
    //_mp3Buffer->Reset();
    
    if( _fd >= 0 ) {
      close(_fd);
      _fd = -1;
    }
  
    for( auto& observer : m_observers ) {
      PRINT_NAMED_WARNING("WHATNOW", "calling onPlaybackFinished for source %d", (int)id);
      observer->onPlaybackFinished( id );
    }
  });
  
  return true;
}

 bool   AlexaSpeaker::stop (SourceId id)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker stop");
  
  m_executor.submit([id, this]() {
    if( _state == State::Playing ) {
      _state = State::Stopping;
    }
    _offset_ms = 0;
    for( auto& observer : m_observers ) {
      PRINT_NAMED_WARNING("WHATNOW", "calling onPlaybackStopped for source %d", (int)id);
      observer->onPlaybackStopped( id );
    }
  });
  return true;
}

 bool   AlexaSpeaker::pause (SourceId id)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker pause");
  
  m_executor.submit([id, this]() {
    for( auto& observer : m_observers ) {
      PRINT_NAMED_WARNING("WHATNOW", "calling onPlaybackPaused for source %d", (int)id);
      observer->onPlaybackPaused( id );
    }
  });
  return true;
}

 bool   AlexaSpeaker::resume (SourceId id)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker resume");
  
  m_executor.submit([id, this]() {
    for( auto& observer : m_observers ) {
      PRINT_NAMED_WARNING("WHATNOW", "calling onPlaybackResumed for source %d", (int)id);
      observer->onPlaybackResumed( id );
    }
  });
  return true;
}

std::chrono::milliseconds   AlexaSpeaker::getOffset (SourceId id)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker getOffset");
    
  return std::chrono::milliseconds{_offset_ms};
}

 uint64_t   AlexaSpeaker::getNumBytesBuffered ()  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker getNumBytesBuffered");
  return 0;
}

 void   AlexaSpeaker::setObserver (std::shared_ptr< avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface > playerObserver)  {
  PRINT_NAMED_WARNING("WHATNOW", " speaker setObserver");
  m_observers.insert( playerObserver );
}
  
int AlexaSpeaker::Decode(const StreamingWaveDataPtr& data, bool flush)
{
  mp3dec_frame_info_t info;
  
  using namespace AudioEngine;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
  
  float millis_decoded = 0.0f;
  
  while( true ) {
    
    const int buffSize = flush ? _mp3Buffer->Size() : kMaxReadSize;
    unsigned char* inputBuff = _mp3Buffer->ReadData(buffSize, flush);
    
    if( buffSize == 0 || inputBuff == nullptr ) {
      // need to wait for more data
      PRINT_NAMED_WARNING("WHATNOW", "Needs more data (%s)", StateToString() );
      break;
    }
    
    int samples = mp3dec_decode_frame(&mp3d, inputBuff, buffSize, pcm, &info);
    //std::cout << "samples=" << samples << ", frame_bytes=" << info.frame_bytes << std::endl;
    int consumed = info.frame_bytes;
    if( info.frame_bytes > 0  ) {
      bool success = _mp3Buffer->AdvanceCursor( consumed );
      ANKI_VERIFY(success, "WHATNOW", "Could not advance by %d", consumed);
      
      if( samples > 0 ) {
        // valid wav data!
        if( _first ) {
          _first = false;
          PRINT_NAMED_WARNING("WHATNOW", "DECODING: channels=%d, hz=%d, layer=%d, bitrate=%d", info.channels, info.hz, info.layer, info.bitrate_kbps);
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
          //std::lock_guard<std::mutex> lock{ _mutex };
          const auto numFrames = data->GetNumberOfFramesReceived();
          //PRINT_NAMED_WARNING("WHATNOW", "LOOP: decoded. available frames=%d. decoded %d ms, state=%s", numFrames, (int)millis_decoded, StateToString());
          if( (_state == State::Preparing) && numFrames >= kMinPlayableFrames ) {
            _state = State::Playable;
          }
        }

        //outData.insert( outData.end(), std::begin(pcm), std::begin(pcm) + samples );
      } else {
        PRINT_NAMED_WARNING("WHATNOW", "LOOP: skipped (frame_bytes=%d), state=%s", info.frame_bytes, StateToString() );
        //std::cout << "skipped" << std::endl;
      }
    } else {
      PRINT_NAMED_WARNING("WHATNOW", "LOOP: no bytes (%s)", StateToString() );
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
      case State::Stopping: return "Stopping";
    };
  }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaSpeaker::SavePCM( short* buff, size_t size )
{
  PRINT_NAMED_WARNING("WHATNOW", "saving pcm size=%zu", size);
  static int pcmfd = -1;
  if (pcmfd < 0) {
    const auto path = "/data/data/com.anki.victor/cache/speaker.pcm";
    pcmfd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
  }
  
  if( size > 0 && (buff != nullptr) ) {
    (void) write(pcmfd, buff, size * sizeof(short));
  } else {
    close(pcmfd);
    pcmfd = -1;
  }
}
  
} // namespace Vector
} // namespace Anki

