/**
 * File: cozmoAudioController.h
 *
 * Author: Jordan Rivas
 * Created: 02/17/2017
 *
 * Description: Cozmo interface to Audio Engine
 *              - Subclass of AudioEngine::AudioEngineController
 *              - Implement Cozmo specific audio functionality
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Basestation_Audio_CozmoAudioController_H__
#define __Basestation_Audio_CozmoAudioController_H__

#include "audioEngine/audioEngineController.h"

#define HijackAudioPlugInDebugLogs 0


namespace Anki {
namespace Cozmo {
class CozmoContext;

namespace Audio {
  
  class AudioControllerPluginInterface;
  class RobotAudioBuffer;
  
class CozmoAudioController : public AudioEngine::AudioEngineController
{
  
public:
  
  CozmoAudioController( const CozmoContext* context );
  
  virtual ~CozmoAudioController();
  
  // Create and store a Robot Audio Buffer with the corresponding GameObject and PluginId
  RobotAudioBuffer* RegisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObjectId,
                                              AudioEngine::AudioPluginId pluginId );
  
  void UnregisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObject,
                                   AudioEngine::AudioPluginId pluginId );
  
  // Get Robot Audio Buffer
  RobotAudioBuffer* GetRobotAudioBufferWithGameObject( AudioEngine::AudioGameObject gameObjectId ) const;
  RobotAudioBuffer* GetRobotAudioBufferWithPluginId( AudioEngine::AudioPluginId pluginId ) const;
  
  // Handle when the app is goes in and out of Focus
  void AppIsInFocus( const bool inFocus );
  
  
private:
  // Configure plugins & robot buffers
  void SetupPlugins();

  // Register CLAD Game Objects
  void RegisterCladGameObjectsWithAudioController();

private:
  
  // Store Robot Audio Buffer and Look up tables
  // Once a buffer is created it is not intended to be destroyed
  std::unordered_map< AudioEngine::AudioPluginId, RobotAudioBuffer* > _robotAudioBufferIdMap;
  std::unordered_map< AudioEngine::AudioGameObject, AudioEngine::AudioPluginId > _gameObjectPluginIdMap;
  
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
  
}
}
}


#endif /* __Basestation_Audio_CozmoAudioController_H__ */
