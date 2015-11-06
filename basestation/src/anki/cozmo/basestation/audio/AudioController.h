//
//  AudioController.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/3/15.
//
//

#ifndef AudioController_h
#define AudioController_h

#include <stdio.h>

#include <DriveAudioEngine/audioTypes.h>
#include <util/helpers/noncopyable.h>
#include <util/dispatchQueue/iTaskHandle.h>

namespace AudioEngine {
  class AudioEngineController;
}

namespace Anki {
namespace Util {
namespace Dispatch {
  class Queue;
}
namespace Data {
  class DataPlatform;
}
  
}
namespace Cozmo {
  
class AudioController : public Anki::Util::noncopyable
{
  
public:
  
  // TODO: Remove Singleton
  inline static AudioController* getInstance();
  static void removeInstance();
  
  AudioController();
  ~AudioController();

  
  bool Initialize(Util::Data::DataPlatform* dataPlatfrom);
  // TODO: Need to add Language Local, pathToZipFile, zipBasePath, assetManager & assetManagerBasePath for RAMS & Android
 
  // TODO: Add Callback stuff
  AudioEngine::AudioEventID PostAudioEvent( const std::string& eventName,
                                            AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject );
  
  AudioEngine::AudioEventID PostAudioEvent( AudioEngine::AudioEventID eventId,
                                            AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject );
  
  bool SetState( AudioEngine::AudioStateGroupId stateGroupId,
                 AudioEngine::AudioStateId stateId );
  
  bool SetSwitchState( AudioEngine::AudioSwitchGroupId switchGroupId,
                       AudioEngine::AudioSwitchStateId switchStateId,
                       AudioEngine::AudioGameObject gameObject );
  
  bool SetParameter( AudioEngine::AudioParameterId parameterId,
                     AudioEngine::AudioRTPCValue rtpcValue,
                     AudioEngine::AudioGameObject gameObject,
                     AudioEngine::AudioTimeMs valueChangeDuration = 0,
                     AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear );
  

protected:
  
  static AudioController*             _singletonInstance;
  AudioEngine::AudioEngineController* _audioEngine;             // Audio Engine
  Util::Dispatch::Queue*              _dispatchQueue;           // The dispatch queue we're ticking on
  Anki::Util::TaskHandle              _taskHandle;              // Handle to our tick callback task
  
  
  bool _isInitialized;
  
  // Tick Audio Engine
  void Update();
  
};
  
// TODO: Remove Singleton
inline AudioController* AudioController::getInstance()
{
  // If we haven't already instantiated the singleton, do so now.
  if( nullptr == _singletonInstance ) {
    _singletonInstance = new AudioController();
  }
  return _singletonInstance;
}
  
} // Cozmo
} // Anki



#endif /* AudioController_h */
