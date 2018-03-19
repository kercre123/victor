/**
 * File: cozmoAudioController.h
 *
 * Author: Jordan Rivas
 * Created: 09/07/2017
 *
 * Description: Cozmo interface to Audio Engine
 *              - Subclass of AudioEngine::AudioEngineController
 *              - Implement Cozmo specific audio functionality
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_CozmoAudioController_H__
#define __Anki_Cozmo_CozmoAudioController_H__

#include "audioEngine/audioEngineController.h"
#include <memory>

namespace Anki {
namespace AudioEngine {
class SoundbankLoader;
}
namespace Cozmo {
class AnimContext;
namespace Audio {


class CozmoAudioController : public AudioEngine::AudioEngineController
{

public:

  CozmoAudioController(const AnimContext* context);

  virtual ~CozmoAudioController();
  
  // Save session profiler capture to a file
  bool WriteProfilerCapture( bool write );
  // Save session audio output to a file
  bool WriteAudioOutputCapture( bool write );
  
  // Control Robot's master volume
  // Valid Volume values are [0.0 - 1.0]
  void SetRobotMasterVolume( AudioEngine::AudioRTPCValue volume,
                             AudioEngine::AudioTimeMs timeInMilliSeconds = 0,
                             AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear );
  
  // Control Robot's procedural audio volume (a.k.a. movement sounds)
  // Valid Volume values are [0.0 - 1.0]
  void SetProceduralAudioVolume( AudioEngine::AudioRTPCValue volume,
                                 AudioEngine::AudioTimeMs timeInMilliSeconds = 0,
                                 AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear );


private:
  
  std::unique_ptr<AudioEngine::SoundbankLoader> _soundbankLoader;
  
  // Register CLAD Game Objects
  void RegisterCladGameObjectsWithAudioController();
 
};

}
}
}

#endif /* __Anki_Cozmo_CozmoAudioController_H__ */
