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

#define HijackAudioPlugInDebugLogs 0

namespace AudioEngine {
  class AudioEngineController;

namespace PlugIns {
  class HijackAudioPlugIn;
  class WavePortalPlugIn;
}
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
class AudioWaveFileReader;
  
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

  // Stops playing all sounds on the specified game object
  // + If AudioGameObject::Invalid is specified, then ALL audio will be stopped
  void StopAllAudioEvents( AudioEngine::AudioGameObject gameObject = AudioEngine::kInvalidAudioGameObject );
  
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
  
  // Set out_buffer
  // Return coresponding game object for out_buffer
  AudioEngine::AudioGameObject GetAvailableRobotAudioBuffer( RobotAudioBuffer*& out_buffer );
  
  // Register Game Objects
  // Note Game Object Ids must be unique
  bool RegisterGameObject( AudioEngine::AudioGameObject gameObjectId,  std::string gameObjectName );

  // TODO: Add RemoveRegisterGameObject()
  
  // TEMP: Set Cozmo Speaker Volumes
  void StartUpSetDefaults();
  

private:
  
  AudioEngine::AudioEngineController*               _audioEngine            = nullptr;  // Audio Engine Lib
  AudioEngine::PlugIns::HijackAudioPlugIn*          _hijackAudioPlugIn      = nullptr;  // Plugin Instance
  std::unordered_map< int32_t, RobotAudioBuffer* >  _robotAudioBufferPool; // Store all Audio Buffers
  
  AudioWaveFileReader*           _reader                      = nullptr;
  AudioEngine::PlugIns::WavePortalPlugIn* _wavePortalPlugIn   = nullptr;
  
  
  Util::Dispatch::Queue*    _dispatchQueue  = nullptr;  // The dispatch queue we're ticking on
  Anki::Util::TaskHandle    _taskHandle     = nullptr;  // Handle to our tick callback task
  
  bool _isInitialized = false;
  
  using CallbackContextMap = std::unordered_map< AudioEngine::AudioPlayingID, AudioEngine::AudioCallbackContext* >;
  CallbackContextMap _eventCallbackContexts;
  
  std::vector< AudioEngine::AudioCallbackContext* > _callbackGarbageCollector;

  // Setup HijackAudio plug-in & robot buffers
  void SetupHijackAudioPlugInAndRobotAudioBuffers();
  
  // Setup WavePortal plug-in
  void SetupWavePortalPlugIn();
  
  RobotAudioBuffer* GetAudioBuffer( int32_t plugInId ) const;
  
  // Tick Audio Engine
  void Update();
  
  // Clean up call back messages
  void MoveCallbackContextToGarbageCollector( const AudioEngine::AudioCallbackContext* callbackContext );
  void ClearGarbageCollector();
  
  
  // Debug Cozmo PlugIn Logs
#if HijackAudioPlugInDebugLogs
  enum class LogEnumType {
    Post,
    CreatePlugIn,
    DestoryPlugIn,
    Update,
  };
  
  struct TimeLog {
    LogEnumType LogType;
    std::string Msg;
    unsigned long long int TimeInNanoSec;
    
    TimeLog(LogEnumType logType, std::string msg, unsigned long long int timeInNanoSec) :
    LogType(logType),
    Msg(msg),
    TimeInNanoSec(timeInNanoSec)
    {
    }
  };
  std::vector<TimeLog> _plugInLog;
  void PrintPlugInLog();
#endif
  
};


} // Audio
} // Cozmo
} // Anki



#endif /* __Basestation_Audio_AudioController_H__ */
