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


#include "DriveAudioEngine/audioTypes.h"
#include "DriveAudioEngine/audioCallback.h"
#include "util/helpers/noncopyable.h"
#include "util/dispatchQueue/iTaskHandle.h"
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
namespace Cozmo {
class CozmoContext;
  
namespace Audio {

class MusicConductor;
class AudioControllerPluginInterface;
class RobotAudioBuffer;
class AudioWaveFileReader;

  
class AudioController : public Util::noncopyable
{
  friend class AudioControllerPluginInterface;
  
public:
  
  AudioController( const CozmoContext* context );
  
  ~AudioController();
  
  static const char* kAudioLogChannelName;

  // Note: Transfer's callback context ownership to Audio Controller
  AudioEngine::AudioPlayingId PostAudioEvent( const std::string& eventName,
                                              AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject,
                                              AudioEngine::AudioCallbackContext* callbackContext = nullptr );
  
  AudioEngine::AudioPlayingId PostAudioEvent( AudioEngine::AudioEventId eventId,
                                              AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject,
                                              AudioEngine::AudioCallbackContext* callbackContext = nullptr );

  // Stops playing all sounds on the specified game object
  // + If AudioGameObject::Invalid is specified, then ALL audio will be stopped
  void StopAllAudioEvents( AudioEngine::AudioGameObject gameObjectId = AudioEngine::kInvalidAudioGameObject );
  
  bool SetState( AudioEngine::AudioStateGroupId stateGroupId,
                 AudioEngine::AudioStateId stateId ) const;
  
  bool SetSwitchState( AudioEngine::AudioSwitchGroupId switchGroupId,
                       AudioEngine::AudioSwitchStateId switchStateId,
                       AudioEngine::AudioGameObject gameObjectId ) const;
  
  bool SetParameter( AudioEngine::AudioParameterId parameterId,
                     AudioEngine::AudioRTPCValue rtpcValue,
                     AudioEngine::AudioGameObject gameObjectId,
                     AudioEngine::AudioTimeMs valueChangeDuration = 0,
                     AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear ) const;
  
  bool SetParameterWithPlayingId( AudioEngine::AudioParameterId parameterId,
                                  AudioEngine::AudioRTPCValue rtpcValue,
                                  AudioEngine::AudioPlayingId playingId,
                                  AudioEngine::AudioTimeMs valueChangeDuration = 0,
                                  AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear ) const;
  
  // Create and store a Robot Audio Buffer with the corresponding GameObject and PluginId
  RobotAudioBuffer* RegisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObjectId,
                                              AudioEngine::AudioPluginId pluginId );

  void UnregisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObject,
                                   AudioEngine::AudioPluginId pluginId );
  
  // Get Robot Audio Buffer
  RobotAudioBuffer* GetRobotAudioBufferWithGameObject( AudioEngine::AudioGameObject gameObjectId ) const;
  RobotAudioBuffer* GetRobotAudioBufferWithPluginId( AudioEngine::AudioPluginId pluginId ) const;
  
  
  // Register Game Objects
  // Note Game Object Ids must be unique
  bool RegisterGameObject( AudioEngine::AudioGameObject gameObjectId,  std::string gameObjectName );

  // TODO: Add RemoveRegisterGameObject()
  
  
  // Game-Defined Auxiliary Sends
  using AuxSendList = std::vector<AudioEngine::AudioAuxBusValue>;
  bool SetGameObjectAuxSendValues( AudioEngine::AudioGameObject gameObjectId, const AuxSendList& auxSendValues );
  
  bool SetGameObjectOutputBusVolume( AudioEngine::AudioGameObject gameObjectId, AudioEngine::AudioReal32 controlVolume );

  
  // Expose PlugIn functionality
  AudioControllerPluginInterface* GetPluginInterface() const { return _pluginInterface; }
  
  // Music
  MusicConductor* GetMusicConductor() const { return _musicConductor; }
  
  // Tick Audio Engine
  // This will Tick both Music Conductor & call ProcessAudioQueue()
  // Note: This is thread safe
  void Update();
  
  // Process all Audio Engine events
  // This flushes all audio events that have been posted
  void ProcessAudioQueue() const;
  

private:

  // Engine Components
  AudioEngine::AudioEngineController*       _audioEngine            = nullptr;  // Audio Engine Lib
  // Custom Plugins
  AudioEngine::PlugIns::HijackAudioPlugIn*  _hijackAudioPlugIn      = nullptr;  // Plugin Instance
  AudioEngine::PlugIns::WavePortalPlugIn*   _wavePortalPlugIn       = nullptr;

  // Controller Components
  AudioControllerPluginInterface*           _pluginInterface = nullptr;
  
  // Store Robot Audio Buffer and Look up tables
  // Once a buffer is created it is not intended to be destroyed
  std::unordered_map< AudioEngine::AudioPluginId, RobotAudioBuffer* > _robotAudioBufferIdMap;
  std::unordered_map< AudioEngine::AudioGameObject, AudioEngine::AudioPluginId > _gameObjectPluginIdMap;
  
  bool _isInitialized = false;
  
  using CallbackContextMap = std::unordered_map< AudioEngine::AudioPlayingId, AudioEngine::AudioCallbackContext* >;
  CallbackContextMap _eventCallbackContexts;
  
  std::vector< AudioEngine::AudioCallbackContext* > _callbackGarbageCollector;
  
  MusicConductor* _musicConductor = nullptr;
  

  // Setup HijackAudio plug-in & robot buffers
  void SetupHijackAudioPlugInAndRobotAudioBuffers();
  
  // Setup WavePortal plug-in
  void SetupWavePortalPlugIn();
  
  // Clean up call back messages
  void MoveCallbackContextToGarbageCollector( const AudioEngine::AudioCallbackContext* callbackContext );
  void ClearGarbageCollector();

  
  // Debug Cozmo PlugIn Logs
#if HijackAudioPlugInDebugLogs
  enum class LogEnumType {
    Post,
    CreatePlugIn,
    DestroyPlugIn,
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
