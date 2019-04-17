/**
 * File: alexaPlaybackRecognizerComponent.h
 *
 * Author: Jordan Rivas
 * Created: 02/01/19
 *
 * Description: Run "Alexa" speech recognizer on the audio engine's master output. The recognizer is provided by the
 *              SpeechRecognizerSystem and ran on a worker thread. The SpeechRecognizerSystem responds when there is
 *              a trigger detected in the playback stream.
 *
 * Copyright: Anki, Inc. 2019
 *
 */


#ifndef __AnimProcess_AlexaPlaybackRecognizerComponent_H__
#define __AnimProcess_AlexaPlaybackRecognizerComponent_H__


#include "audioEngine/plugins/akAlsaSinkPluginTypes.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "util/container/fixedCircularBuffer.h"
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

// Forward Declarations
struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;
namespace Anki {
namespace AudioEngine {
namespace PlugIns {
class AkAlsaSinkPlugIn;
}
}
namespace AudioUtil {
struct SpeechRecognizerCallbackInfo;
}
namespace Util {
class Locale;
}
namespace Vector {
namespace Anim {
  class AnimContext;
}
class SpeechRecognizerSystem;
class SpeechRecognizerPryonLite;
}
}
  
namespace Anki {
namespace Vector {

class AlexaPlaybackRecognizerComponent {
    
public:

  AlexaPlaybackRecognizerComponent( const Anim::AnimContext* context,
                                    SpeechRecognizerSystem& speechRecognizerSystem );

  virtual ~AlexaPlaybackRecognizerComponent();

  // Setup instance
  bool Init();

  // The recognizer runs when active
  void SetRecognizerActivate( bool activate );

  // Locale has been updated notify worker thread to change models
  void PendingLocaleUpdate();

  // Check if the class was properly constructed
  bool GetIsReady() const { return _isInitialized; }


private:
  
  using SinkPluginTypes = Anki::AudioEngine::PlugIns::AkAlsaSinkPluginTypes;

  // Resampler constants
  static constexpr size_t kRecognizerSampleRate = 16000;
  static constexpr size_t kResamplerBufferSize  = 512;
  static constexpr size_t kRecognizerBufferSize = 160; // 10 ms of single channel audio

  // Buffer constants
  // Add extra buffer for overflow
  static constexpr uint32_t kAudioEngOutBufferSize = SinkPluginTypes::kAkAlsaSinkBufferCount + 1;
  // Need lowest common denominator of 512 (resample buffer size) and 160 (10 ms of data for recognizer)
  // We always want to use continuous memory when writing or reading
  static constexpr size_t kResamplerCircularBufferSize = kResamplerBufferSize * 5;

  // Handle to objects
  const Anim::AnimContext*                  _context = nullptr;         // This a pointer to a unique_ptr()'s instance
  SpeechRecognizerPryonLite*                _recognizer = nullptr;      // This a pointer to a unique_ptr()'s instance
  SpeechRecognizerSystem&                   _speechRecSys;
  SpeexResamplerState*                      _resamplerState = nullptr;
  AudioEngine::PlugIns::AkAlsaSinkPlugIn*   _sinkPlugin = nullptr;

  // State flags
  std::atomic_bool                _isActive{ false };
  bool                            _isInitialized = false;
  bool                            _processThreadStop = false;
  bool                            _pendingLocaleUpdate = false;

  // Buffers
  template <class T, size_t kCapacity>
  using CircularBuffer = Util::FixedCircularBuffer<T, kCapacity>;
  CircularBuffer<SinkPluginTypes::AudioChunk, kAudioEngOutBufferSize>                 _audioEngOutBuffer;
  CircularBuffer<SinkPluginTypes::AudioChunk*, kAudioEngOutBufferSize>                _validOutBufferPtrs;
  CircularBuffer<SinkPluginTypes::AkSinkAudioSample_t, kResamplerCircularBufferSize>  _resampledAudioBuffer;

  // Threads
  std::thread               _processAudioThread;
  std::condition_variable   _processAudioThreadCondition;
  std::mutex                _audioEngOutputBufferMutex;


  // Private Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Callback function for when audio engine processes a period
  void SinkPluginCallback( const SinkPluginTypes::AudioChunk& chunk, bool hasData );

  // Working thread run loop
  void ProcessAudioLoop();

  // Helper method to do work on worker thread
  void ProcessAudio( const SinkPluginTypes::AudioChunk* inData );

};

} // namespace Vector
} // namespace Anki

#endif // __AnimProcess_AlexaPlaybackRecognizerComponent_H__
