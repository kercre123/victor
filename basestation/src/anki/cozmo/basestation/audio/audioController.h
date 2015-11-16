/*
 * File: audioController.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is responsible for instantiating the Audio Engine and handling basic and app level audio
 *              functionality.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#ifndef __Basestation_Audio_AudioController_H__
#define __Basestation_Audio_AudioController_H__

#include <DriveAudioEngine/audioTypes.h>
#include <DriveAudioEngine/audioCallback.h>
#include <util/helpers/noncopyable.h>
#include <util/dispatchQueue/iTaskHandle.h>
#include <unordered_map>
#include <vector>


namespace AudioEngine {
  class AudioEngineController;
  class CozmoPlugIn;
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
namespace Audio {
  
class RobotAudioBuffer;
  
class AudioController : public Util::noncopyable
{
  
public:
  
  AudioController( Util::Data::DataPlatform* dataPlatfrom );
  // TODO: Need to add Language Local, pathToZipFile, zipBasePath, assetManager & assetManagerBasePath for RAMS & Android
  
  ~AudioController();

  // Note: Transfer's callback context ownership to Audio Controller
  AudioEngine::AudioPlayingID PostAudioEvent( const std::string& eventName,
                                              AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject,
                                              AudioEngine::AudioCallbackContext* callbackContext = nullptr );
  
  AudioEngine::AudioPlayingID PostAudioEvent( AudioEngine::AudioEventID eventId,
                                              AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject,
                                              AudioEngine::AudioCallbackContext* callbackContext = nullptr );
  
  bool SetState( AudioEngine::AudioStateGroupId stateGroupId,
                 AudioEngine::AudioStateId stateId ) const;
  
  bool SetSwitchState( AudioEngine::AudioSwitchGroupId switchGroupId,
                       AudioEngine::AudioSwitchStateId switchStateId,
                       AudioEngine::AudioGameObject gameObject ) const;
  
  bool SetParameter( AudioEngine::AudioParameterId parameterId,
                     AudioEngine::AudioRTPCValue rtpcValue,
                     AudioEngine::AudioGameObject gameObject,
                     AudioEngine::AudioTimeMs valueChangeDuration = 0,
                     AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear ) const;
  
  // Get Audio Buffer Obj for robot
  RobotAudioBuffer* GetRobotAudioBuffer() { return _robotAudioBuffer; }
  
  // Return true if successful
//  bool SetAudioCallbackFunc( AudioEngine::AudioCallbackFunc callbackFunc );

private:
  
  AudioEngine::AudioEngineController* _audioEngine      = nullptr;  // Audio Engine Lib
  AudioEngine::CozmoPlugIn*           _cozmoPlugIn      = nullptr;  // Plugin Instance
  RobotAudioBuffer*                   _robotAudioBuffer = nullptr;  // Audio Buffer for Robot Audio Clinet
  
  Util::Dispatch::Queue*              _dispatchQueue    = nullptr;  // The dispatch queue we're ticking on
  Anki::Util::TaskHandle              _taskHandle       = nullptr;  // Handle to our tick callback task
  
  bool _isInitialized = false;
  
  using CallbackContextMap = std::unordered_map< AudioEngine::AudioPlayingID, AudioEngine::AudioCallbackContext* >;
  CallbackContextMap _eventCallbackContexts;
  
  std::vector< AudioEngine::AudioCallbackContext* > _callbackGarbageCollector;

  
  // Tick Audio Engine
  void Update();
  
  void MoveCallbackContextToGarbageCollector( const AudioEngine::AudioCallbackContext* callbackContext );
  
};


} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_AudioController_H__ */
