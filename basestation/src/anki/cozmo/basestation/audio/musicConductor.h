//
//  MusicConductor.hpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 6/14/16.
//
//

#ifndef MusicConductor_hpp
#define MusicConductor_hpp

#include "AudioEngine/audioTypes.h"
#include "AudioEngine/audioCallback.h"
#include "util/helpers/noncopyable.h"
#include <mutex>

namespace Anki {
namespace Cozmo {
namespace Audio {
  
class AudioController;
  
class MusicConductor : Util::noncopyable {
  
public:
  
  static constexpr uint32_t kZeroDuration = 0;
  
  MusicConductor( AudioController& audioController,
                  AudioEngine::AudioGameObject musicGameObject,
                  AudioEngine::AudioStateGroupId musicGroupId,
                  AudioEngine::AudioEventId startEventId,
                  AudioEngine::AudioEventId stopEventId );
  
  void SetMusicState( AudioEngine::AudioStateId stateId,
                      bool interrupt = false,
                      uint32_t minDuratoin_ms = kZeroDuration );
  
  // Need to add method to start listening to music callbacks
  // AudioEngine::AudioCallbackContext* callbackContext = nullptr
  
  // Need to tick conductor
  void UpdateTick();
  
  void StopMusic();
  
private:
  
  AudioController&          _audioController;
  bool                      _isPlaying      = false;
  AudioEngine::AudioStateId _currentStateId = AudioEngine::kInvalidAudioStateId;
  double                    _currentStateExpirationTimeStamp_ms = 0.0;
  AudioEngine::AudioStateId _pendingStateId = AudioEngine::kInvalidAudioStateId;
  uint32_t                  _pendingStateMinDuration_ms = kZeroDuration;
  std::mutex                _lock;
  
  // App specific values
  const AudioEngine::AudioGameObject    _musicGameObject;
  const AudioEngine::AudioStateGroupId  _musicStateId;
  const AudioEngine::AudioEventId       _startEventId;
  const AudioEngine::AudioEventId       _stopEventId;
  
  // Post the new music state to audio engine and reset vars
  // Note: This is not thread safe, must lock when calling method
  void UpdateMusicState();

};
  
} // Audio
} // Cozmo
} // Anki


#endif /* MusicConductor_hpp */
