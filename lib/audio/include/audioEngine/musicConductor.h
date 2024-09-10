/*
 * File: musicConductor.h
 *
 * Author: Jordan Rivas
 * Created: 06/14/16
 *
 * Description: This class tracks and manages the current Music state and transitions.
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#ifndef __AnkiAudio_MusicConductor_H__
#define __AnkiAudio_MusicConductor_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include "audioEngine/audioCallback.h"
#include "util/helpers/noncopyable.h"
#include <mutex>


namespace Anki {
namespace AudioEngine {
  
class AudioEngineController;
  
struct AUDIOENGINE_EXPORT MusicConductorConfig {
  AudioGameObject   musicGameObject = kInvalidAudioGameObject;
  AudioStateGroupId musicGroupId    = kInvalidAudioStateGroupId;
  AudioEventId      startEventId    = kInvalidAudioEventId;
  AudioEventId      stopEventId     = kInvalidAudioEventId;
};

  
class AUDIOENGINE_EXPORT MusicConductor : Anki::Util::noncopyable {
  
public:
  
  static constexpr uint32_t kZeroDuration = 0;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  MusicConductor( AudioEngineController& audioEngine, const MusicConductorConfig& config );
  
  void SetMusicState( AudioStateId stateId,
                      bool interrupt = false,
                      uint32_t minDuratoin_ms = kZeroDuration );
  
  // Need to add method to start listening to music callbacks
  // AudioCallbackContext* callbackContext = nullptr
  
  // Need to tick conductor
  void UpdateTick();
  
  void StopMusic();
  
private:
  
  AudioEngineController&    _audioEngine;
  bool                      _isPlaying      = false;
  AudioEngine::AudioStateId _currentStateId = AudioEngine::kInvalidAudioStateId;
  double                    _currentStateExpirationTimeStamp_ms = 0.0;
  AudioEngine::AudioStateId _pendingStateId = AudioEngine::kInvalidAudioStateId;
  uint32_t                  _pendingStateMinDuration_ms = kZeroDuration;
  std::mutex                _lock;
  
  // App specific values
  const AudioGameObject    _musicGameObject;
  const AudioStateGroupId  _musicStateId;
  const AudioEventId       _startEventId;
  const AudioEventId       _stopEventId;
  
  // Post the new music state to audio engine and reset vars
  // Note: This is not thread safe, must lock when calling method
  void UpdateMusicState();

};
  
} // AudioEngine
} // Anki


#endif /* __AnkiAudio_MusicConductor_H__ */
