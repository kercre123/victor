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
#include <atomic>
#include <memory>
#include <map>


namespace Anki {
namespace AudioEngine {
class SoundbankLoader;
}
namespace AudioMetaData {
namespace GameParameter {
// Forward declare audioParameterTypes.h
enum class ParameterType : uint32_t;
}
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
  
  // Set a specific volume channel
  // Valid Volume channels are:
  // Robot_Vic_Volume_Master, Robot_Vic_Volume_Animation, Robot_Vic_Volume_Behavior & Robot_Vic_Volume_Procedural
  void SetVolume( AudioMetaData::GameParameter::ParameterType volumeChannel,
                  AudioEngine::AudioRTPCValue volume,
                  AudioEngine::AudioTimeMs timeInMilliSeconds = 0,
                  AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear,
                  bool storeVolume = true );

  // Control Robot's master volume
  // Valid Volume values are [0.0 - 1.0]
  void SetRobotMasterVolume( AudioEngine::AudioRTPCValue volume,
                             AudioEngine::AudioTimeMs timeInMilliSeconds = 0,
                             AudioEngine::AudioCurveType curve = AudioEngine::AudioCurveType::Linear );
  
  // Get Volume channel value [0.0 - 1.0]
  // Return ture if found
  bool GetVolume( AudioMetaData::GameParameter::ParameterType volumeChannel,
                  AudioEngine::AudioRTPCValue& out_value,
                  bool defaultValue = false );
  
  // Reset all volume channels to default value
  // store default values to persistent storage
  void SetDefaultVolumes( bool store = true );
  
  // Activate consumable parameters to get updated Audio Engine runtime values
  // See cozmoAudioController.cpp for "consumable parameters" list
  // Return true if successfully activated/deactivated
  bool ActivateParameterValueUpdates( bool activate );
  bool GetActivatedParameterValue( AudioMetaData::GameParameter::ParameterType parameter,
                                   AudioEngine::AudioRTPCValue& out_value );

private:
  
  const AnimContext* _animContext = nullptr;
  std::unique_ptr<AudioEngine::SoundbankLoader> _soundbankLoader;
  // Volume Settings
  std::map<AudioMetaData::GameParameter::ParameterType, AudioEngine::AudioRTPCValue> _volumeMap;
  // Parameter Value Update functionality
  AudioEngine::AudioEngineCallbackId _parameterUpdateCallbackId = AudioEngine::kInvalidAudioEngineCallbackId;
  std::map<AudioMetaData::GameParameter::ParameterType,
           std::atomic<AudioEngine::AudioRTPCValue>> _consumableParameterValues;
  
  // Register CLAD Game Objects
  void RegisterCladGameObjectsWithAudioController();
  
  // Set initial volumes at startup
  void SetInitialVolume();
  
  // Setup the structures of consumable Audio Engine Parameters
  void SetupConsumableAudioParameters();
  
  // Load/Store persistent volume values
  void LoadVolumeSettings();
  void StoreVolumeSettings();
  bool IsValidVolumeChannel( AudioMetaData::GameParameter::ParameterType volumeChannel );
  
  bool ParameterUpdatesIsActive() const
  { return ( _parameterUpdateCallbackId != AudioEngine::kInvalidAudioEngineCallbackId ); }
  
};

}
}
}

#endif /* __Anki_Cozmo_CozmoAudioController_H__ */
