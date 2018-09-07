/**
 * File: radioAudioComponent.cpp
 *
 * Author:
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016-2018
 *
 */

#include "cozmoAnim/radioAudioComponent.h"
#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/streamingWavePortalPlugIn.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
//#include "cozmoAnim/audioLayerBuffer.h"
#include "cozmoAnim/audioDataBuffer.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/console/consoleInterface.h"

#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioSwitchTypes.h"

#include <fcntl.h>
#include <unistd.h>



//#define MINIMP3_FLOAT_OUTPUT
//#ifdef MINIMP3_FLOAT_OUTPUT
//  using BufferType = float;
//#else
//  using BufferType = short;
//#endif
//
#define MINIMP3_IMPLEMENTATION
#if defined(VICOS)
  #define HAVE_SSE 0
#endif
#include "cozmoAnim/minimp3.h"

namespace {

  // How many frames do we need before utterance is playable?
  CONSOLE_VAR_RANGED(uint32_t, kMinPlayableFrames, "Radio", 8192, 0, 65536);
  
  constexpr int kAudioBufferSize = 1 << 16;
  constexpr int kMaxReadSize = 1024;
  
  // dumps to raw audio file
  CONSOLE_VAR(bool, kRadioSavePCM, "Radio", true);
  mp3dec_t mp3d;
}

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RadioAudioComponent::RadioAudioComponent()
  : _dispatchQueue(Util::Dispatch::Create("RadioAudioComponent"))
{
  _mp3Buffer.reset( new AudioDataBuffer{kAudioBufferSize,kMaxReadSize} );
  _first = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RadioAudioComponent::~RadioAudioComponent()
{
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::Init(const AnimContext* context)
{
  DEV_ASSERT(nullptr != context, "RadioAudioComponent.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "RadioAudioComponent.InvalidAudioController");
  
  _audioController  = context->GetAudioController();
  
  mp3dec_init(&mp3d);
}
  
void RadioAudioComponent::Update()
{
  if( _audioController == nullptr ) {
    return; // not init yet
  }
  
  bool needsToPlay = false;
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    if( _state == State::Playable ) {
      needsToPlay = true;
      _state = State::Playing;
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::AddData( uint8_t* data, int size )
{
  _mp3Buffer->AddData( data, size );
  
  {
    std::lock_guard<std::mutex> lk(_mutex);
    _hasData = true;
  }
  _cv.notify_one();
  
  // launch a thread to process the new data
  // does nothing if already processing/playing
  PlayIncomingAudio();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::Stop()
{
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    if( _state != State::None ) {
      _state = State::ClearingUp;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::PlayIncomingAudio()
{
  
  {
    std::lock_guard<std::mutex> lock{ _mutex };
    if( _state != State::None ) {
      return;
    }
    _state = State::Preparing;
  }
  
  // Get an empty data instance shared ptr
  _waveData = AudioEngine::PlugIns::StreamingWavePortalPlugIn::CreateDataInstance();
  PRINT_NAMED_WARNING("WHATNOW", "initializing bundle=%p", _waveData.get());
  
  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this](){
    
    bool done = false;

    while( !done ) {
      
      {
        // todo: we don't need a lock here and a lock in _mp3buffer
        std::unique_lock<std::mutex> lock(_mutex);
        if( !_hasData ) {
          _cv.wait(lock, [this]{return _hasData;});
        }
      }
      
      DecodeAudioData(_waveData);
      
      // there's a bug here bc of the two locks.. if data are added in between Decoding getting eof and setting hasData false
      // here, it will need to wait for the next AddData
      
      {
        std::lock_guard<std::mutex> lock{ _mutex };
        _hasData = false;
        done |= (_state == State::ClearingUp) || (_state == State::None);
      
        if( !done && (_state == State::Preparing) && _waveData->GetNumberOfFramesReceived() >= kMinPlayableFrames ) {
          _state = State::Playable;
        }
      
        if (done) {
          _waveData->DoneProducingData();
          
          //SavePCM( (BufferType*)nullptr );
          SavePCM( nullptr );
          
          _state = State::None;
          // dump any data
          _mp3Buffer->Reset();
          _hasData = false;
          _first = true;
        } else {
          //PRINT_NAMED_WARNING("WHATNOW", "initializing bundle=%p, frames=%d", _waveData.get(), _waveData->GetNumberOfFramesReceived());
        }
      }
    }
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::DecodeAudioData(const StreamingWaveDataPtr& data)
{
  
  // decode available mp3 data into wav data
  mp3dec_frame_info_t info;
  
  using namespace AudioEngine;
  //StandardWaveDataContainer waveContainer(0, 0, MINIMP3_MAX_SAMPLES_PER_FRAME);

//#ifdef MINIMP3_FLOAT_OUTPUT
  //float* pcm = waveContainer.audioBuffer;
//#else
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
//#endif
  
  
  
  while( true ) {
    const int buffSize = kMaxReadSize;
    unsigned char* inputBuff = _mp3Buffer->ReadData(buffSize);
    
    //int buffSize = _mp3Buffer->GetNextPointer( 256, inputBuff );
    
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
        
        // for whatever reason, the lib provides # samples as per channel
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
        
        if( kRadioSavePCM ) {
          SavePCM( pcm, samples );
        }
        
        PRINT_NAMED_WARNING("WHATNOW", "LOOP: decoded, state=%s", StateToString() );
        
        // todo: minimp3 should be able to output in float to avoid this allocation and copy, but it
        // seems to clip for me when in float
        StandardWaveDataContainer waveContainer(info.hz, info.channels, samples);
        waveContainer.CopyWaveData( pcm, samples );
        
        data->AppendStandardWaveData( std::move(waveContainer) );
        
        PRINT_NAMED_WARNING("WHATNOW", "LOOP: decoded, state=%s", StateToString() );
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
  
  PRINT_NAMED_WARNING("WHATNOW", "LOOP: #frames=%d, bundle=%p", data->GetNumberOfFramesReceived(), data.get());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void RadioAudioComponent::SavePCM( float* buff, size_t size )
//{
//  static int _fd = -1;
//  if (_fd < 0) {
//    const auto path = "radio.pcm";
//    _fd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
//  }
//  if( size > 0 && (buff != nullptr) ) {
//    (void) write(_fd, buff, size * sizeof(float));
//  } else {
//    close(_fd);
//    _fd = -1;
//  }
//}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioAudioComponent::SavePCM( short* buff, size_t size )
{
  static int _fd = -1;
  if (_fd < 0) {
    const auto path = "radio.pcm";
    _fd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
  }
  if( size > 0 && (buff != nullptr) ) {
    (void) write(_fd, buff, size * sizeof(short));
  } else {
    close(_fd);
    _fd = -1;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const RadioAudioComponent::StateToString() const
{
  switch( _state ) {
    case State::None: return "None";
    case State::Preparing: return "Preparing";
    case State::Playable: return "Playable";
    case State::Playing: return "Playing";
    case State::ClearingUp: return "ClearingUp";
  };
}
  

} // end namespace Vector
} // end namespace Anki
