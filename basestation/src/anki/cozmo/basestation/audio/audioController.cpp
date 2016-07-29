/*
 * File: audioController.cpp
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

#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/musicConductor.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/audio/audioBusses.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include "clad/audio/audioStateTypes.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"

#include <unordered_map>

#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "anki/cozmo/basestation/audio/audioWaveFileReader.h"

// Allow the build to include/exclude the audio libs
//#define EXCLUDE_ANKI_AUDIO_LIBS 0

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#include "DriveAudioEngine/audioEngineController.h"
#include "DriveAudioEngine/PlugIns/hijackAudioPlugIn.h"
#include "DriveAudioEngine/PlugIns/wavePortalPlugIn.h"
#else

// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0

#endif
#define ENABLE_AC_SLEEP_TIME_DIAGNOSTICS 1
// RUN_TIME requires SLEEP_TIME
#define ENABLE_AC_RUN_TIME_DIAGNOSTICS 1
#define UPDATE_LOOP_SLEEP_DURATION_MS 10
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


namespace Anki {
namespace Cozmo {
namespace Audio {
  
using namespace AudioEngine;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::AudioController( Util::Data::DataPlatform* dataPlatfrom )
: _pluginInterface( new AudioControllerPluginInterface( *this ) )
{
#if USE_AUDIO_ENGINE
  {
    _audioEngine = new AudioEngineController();
    const std::string assetPath = dataPlatfrom->pathToResource(Util::Data::Scope::Resources, "sound/" );
    const bool assetsExist = Util::FileUtils::DirectoryExists( assetPath );
    
    // If assets don't exist don't init the Audio engine
    if ( !assetsExist ) {
      PRINT_NAMED_ERROR("AudioController.AudioController", "Audio Assets do NOT exists - Ignore if Unit Test");
      return;
    }
    
    // Config engine
    AudioEngine::SetupConfig config{};
    // Assets
    config.assetFilePath = assetPath;
    config.audioLocal = AudioLocaleType::EnglishUS;
    // Memory
    config.defaultMemoryPoolSize      = ( 4 * 1024 * 1024 );
    config.defaultLEMemoryPoolSize    = ( 4 * 1024 * 1024 );
    config.defaultPoolBlockSize       = 1024;
    config.defaultMaxNumPools         = 30;
    config.enableGameSyncPreparation  = true;
    
    _isInitialized = _audioEngine->Initialize( config );
    
    // If we're using the audio engine, assert that it was successfully initialized.
    ASSERT_NAMED(_isInitialized, "AudioController.Initialize Audio Engine fail");
  }
#endif // USE_AUDIO_ENGINE
  
  // The audio engine was initialized correctly, so now let's setup everything else
  if ( _isInitialized )
  {
    ASSERT_NAMED( !_taskHandle, "AudioController.Initialize Invalid Task Handle" );
#if USE_AUDIO_ENGINE
    
    // Register and Prepare Plug-Ins
    SetupHijackAudioPlugInAndRobotAudioBuffers();
    SetupWavePortalPlugIn();
    
    
    // FIXME: Temp fix to load audio banks
    AudioBankList bankList = {
      "Init.bnk",
      "Music.bnk",
      "UI.bnk",
      "SFX.bnk",
      "Cozmo.bnk",
      "Dev_Debug.bnk",
    };
    const std::string sceneTitle = "InitScene";
    AudioScene initScene = AudioScene( sceneTitle, AudioEventList(), bankList );
    
    _audioEngine->RegisterAudioScene( std::move(initScene) );
    
    _audioEngine->LoadAudioScene( sceneTitle );
    
    StartUpSetDefaults();

#endif
  
    // Setup Music Conductor
    _musicConductor = new MusicConductor( *this,
                                          static_cast<AudioGameObject>( GameObjectType::Default ),
                                          Util::numeric_cast<AudioStateGroupId>( GameState::StateGroupType::Music ),
                                          Util::numeric_cast<AudioEventId>( GameEvent::Music::Play ),
                                          Util::numeric_cast<AudioEventId>( GameEvent::Music::Stop ) );
    
    // Setup our update method to be called periodically
    _dispatchQueue = Util::Dispatch::Create( "AudioController" );
    const std::chrono::milliseconds sleepDuration = std::chrono::milliseconds(UPDATE_LOOP_SLEEP_DURATION_MS);
    _taskHandle = Util::Dispatch::ScheduleCallback( _dispatchQueue, sleepDuration, std::bind( &AudioController::Update, this ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::~AudioController()
{
  if ( _taskHandle && _taskHandle->IsValid() ) {
    _taskHandle->Invalidate();
  }
  
  if ( _dispatchQueue != nullptr ) {
    Util::Dispatch::Stop( _dispatchQueue );
    Util::Dispatch::Release( _dispatchQueue );
  }
  
  Util::SafeDelete( _musicConductor );
  Util::SafeDelete( _pluginInterface );
  
  ClearGarbageCollector();
  
#if USE_AUDIO_ENGINE
  {
    Util::SafeDelete( _audioEngine );
    Util::SafeDelete( _hijackAudioPlugIn );
    Util::SafeDelete( _wavePortalPlugIn );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioPlayingId AudioController::PostAudioEvent( const std::string& eventName,
                                                             AudioEngine::AudioGameObject gameObjectId,
                                                             AudioEngine::AudioCallbackContext* callbackContext )
{
  AudioPlayingId playingId = kInvalidAudioPlayingId;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    playingId = _audioEngine->PostEvent( eventName, gameObjectId, callbackContext );
    if ( kInvalidAudioPlayingId != playingId &&
        nullptr != callbackContext ) {
      callbackContext->SetPlayId( playingId );
      callbackContext->SetDestroyCallbackFunc( [this] ( const AudioCallbackContext* thisContext )
                                              {
                                                MoveCallbackContextToGarbageCollector( thisContext );
                                              } );
      _eventCallbackContexts.emplace( playingId, callbackContext );
    }
    else if ( kInvalidAudioPlayingId == playingId &&
             nullptr != callbackContext ) {
      // Event Failed and there is a callback
      // Perform Error callback for Event Failed and delete callbackContext
      const AudioEventId eventId = _audioEngine->GetAudioHashFromString( eventName );
      callbackContext->HandleCallback( AudioErrorCallbackInfo( gameObjectId,
                                                               kInvalidAudioPlayingId,
                                                               eventId,
                                                               AudioEngine::AudioCallbackErrorType::EventFailed ) );
      Util::SafeDelete( callbackContext );
    }
  }
#endif
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioPlayingId AudioController::PostAudioEvent( AudioEngine::AudioEventId eventId,
                                                             AudioEngine::AudioGameObject gameObjectId,
                                                             AudioEngine::AudioCallbackContext* callbackContext )
{
  AudioPlayingId playingId = kInvalidAudioPlayingId;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    playingId = _audioEngine->PostEvent( eventId, gameObjectId, callbackContext );
    if ( kInvalidAudioPlayingId != playingId &&
         nullptr != callbackContext ) {
      callbackContext->SetPlayId( playingId );
      callbackContext->SetDestroyCallbackFunc( [this] ( const AudioCallbackContext* thisContext )
      {
        MoveCallbackContextToGarbageCollector( thisContext );
      } );
      _eventCallbackContexts.emplace( playingId, callbackContext );
    }
    else if ( kInvalidAudioPlayingId == playingId &&
              nullptr != callbackContext ) {
      // Event Failed and there is a callback
      // Perform Error callback for Event Failed and delete callbackContext
      callbackContext->HandleCallback( AudioErrorCallbackInfo( gameObjectId,
                                                               kInvalidAudioPlayingId,
                                                               eventId,
                                                               AudioEngine::AudioCallbackErrorType::EventFailed ) );
      Util::SafeDelete( callbackContext );
    }
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::Post, "EventId: " + std::to_string(eventId) , Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  }
#endif
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::StopAllAudioEvents( AudioEngine::AudioGameObject gameObject )
{
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    _audioEngine->StopAllAudioEvents( gameObject );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetState( AudioEngine::AudioStateGroupId stateGroupId,
                                AudioEngine::AudioStateId stateId ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetState( stateGroupId, stateId );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetSwitchState( AudioEngine::AudioSwitchGroupId switchGroupId,
                                      AudioEngine::AudioSwitchStateId switchStateId,
                                      AudioEngine::AudioGameObject gameObject ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetSwitch( switchGroupId, switchStateId, gameObject );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetParameter( AudioEngine::AudioParameterId parameterId,
                                    AudioEngine::AudioRTPCValue rtpcValue,
                                    AudioEngine::AudioGameObject gameObject,
                                    AudioEngine::AudioTimeMs valueChangeDuration,
                                    AudioEngine::AudioCurveType curve ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetRTPCValue( parameterId, rtpcValue, gameObject, valueChangeDuration, curve );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetParameterWithPlayingId( AudioEngine::AudioParameterId parameterId,
                                                 AudioEngine::AudioRTPCValue rtpcValue,
                                                 AudioEngine::AudioPlayingId playingId,
                                                 AudioEngine::AudioTimeMs valueChangeDuration,
                                                 AudioEngine::AudioCurveType curve ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetRTPCValueWithPlayingId( parameterId, rtpcValue, playingId, valueChangeDuration, curve );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* AudioController::RegisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObject,
                                                             AudioEngine::AudioPluginId pluginId )
{
  RobotAudioBuffer* buffer = new RobotAudioBuffer();
  const auto it = _robotAudioBufferIdMap.emplace( pluginId, buffer );
  _gameObjectPluginIdMap.emplace( gameObject, pluginId );
  
  if ( !it.second ) {
    // If buffer already exist
    delete buffer;
    PRINT_NAMED_ERROR( "AudioController.RegisterRobotAudioBuffer",
                       "Robot buffer already exist! PluginId: %d GameObject: %u",
                       pluginId, static_cast<uint32_t>( gameObject ) );
  }
  
  return it.first->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* AudioController::GetRobotAudioBufferWithGameObject( AudioEngine::AudioGameObject gameObject ) const
{
  const auto it = _gameObjectPluginIdMap.find( gameObject );
  if ( it != _gameObjectPluginIdMap.end() ) {
    return GetRobotAudioBufferWithPluginId( it->second );
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* AudioController::GetRobotAudioBufferWithPluginId( AudioEngine::AudioPluginId pluginId ) const
{
  const auto it = _robotAudioBufferIdMap.find( pluginId );
  if ( it != _robotAudioBufferIdMap.end() ) {
    return it->second;
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::RegisterGameObject( AudioEngine::AudioGameObject gameObjectId, std::string gameObjectName )
{
  bool success = false;
  
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->RegisterAudioGameObject( gameObjectId, gameObjectName );
  }
#endif
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetGameObjectAuxSendValues( AudioEngine::AudioGameObject gameObject,
                                                  const AuxSendList& auxSendValues )
{
  bool success = false;
  
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetGameObjectAuxSendValues( gameObject, auxSendValues );
  }
#endif
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetGameObjectOutputBusVolume( AudioEngine::AudioGameObject gameObject,
                                                    AudioEngine::AudioReal32 controlVolume )
{
  bool success = false;
  
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetGameObjectOutputBusVolume( gameObject, controlVolume );
  }
#endif
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// THIS IS TEMP
void AudioController::StartUpSetDefaults()
{
  SetParameter( static_cast<AudioEngine::AudioParameterId>( GameParameter::ParameterType::Robot_Volume ), 0.15, kInvalidAudioGameObject );
}


// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::SetupHijackAudioPlugInAndRobotAudioBuffers()
{
#if USE_AUDIO_ENGINE
  using namespace PlugIns;
  // Setup CozmoPlugIn & RobotAudioBuffer
  _hijackAudioPlugIn = new HijackAudioPlugIn( static_cast<uint32_t>( AnimConstants::AUDIO_SAMPLE_RATE ), static_cast<uint16_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) );

  // Setup Callbacks
  _hijackAudioPlugIn->SetCreatePlugInCallback( [this] ( const uint32_t plugInId )
  {
    PRINT_NAMED_INFO( "AudioController.Initialize", "Create PlugIn Callback! PluginId: %d", plugInId );
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
    // Catch mistakes with wwise project
    ASSERT_NAMED( buffer != nullptr,
                  "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.SetCreatePlugInCallback.RobotAudioBufferNull");
    if ( buffer != nullptr ) {
      buffer->PrepareAudioBuffer();
    }
    
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::CreatePlugIn, "", Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  } );
  
  _hijackAudioPlugIn->SetDestroyPluginCallback( [this] ( const uint32_t plugInId )
  {
    PRINT_NAMED_INFO( "AudioController.Initialize", "Destroy PlugIn Callback! PluginId: %d", plugInId );
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
    // Catch mistakes with wwise project
    ASSERT_NAMED( buffer != nullptr,
                  "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.SetDestroyPluginCallback.RobotAudioBufferNull");
    // Done with voice clear audio buffer
    if ( buffer != nullptr ) {
      buffer->CloseAudioBuffer();
    }
    
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::DestoryPlugIn, "", Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
    PrintPlugInLog();
#endif
  } );
  
  _hijackAudioPlugIn->SetProcessCallback( [this] ( const uint32_t plugInId, const AudioReal32* samples, const uint32_t sampleCount )
  {
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
    // Catch mistakes with wwise project
    ASSERT_NAMED( buffer != nullptr,
                  "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.GetRobotAudioBufferWithPluginId.RobotAudioBufferNull");
    if ( buffer != nullptr ) {
      buffer->UpdateBuffer( samples, sampleCount );
    }
     
#if HijackAudioPlugInDebugLogs
     _plugInLog.emplace_back( TimeLog( LogEnumType::Update, "FrameCount: " + std::to_string(frameCount) , Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  } );
  
  const bool success = _hijackAudioPlugIn->RegisterPlugin();
  if ( ! success ) {
    PRINT_NAMED_ERROR( "AudioController.Initialize", "Failed to Register Cozmo PlugIn");
  }
#endif // USE_AUDIO_ENGINE
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController:: SetupWavePortalPlugIn()
{
#if USE_AUDIO_ENGINE
  using namespace PlugIns;
  // Register Wave file
  _wavePortalPlugIn = new WavePortalPlugIn();
  _wavePortalPlugIn->RegisterPlugIn();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::Update()
{
#if USE_AUDIO_ENGINE
  
#if ENABLE_AC_SLEEP_TIME_DIAGNOSTICS
  const double startUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
  {
    static bool firstUpdate = true;
    static double lastUpdateTimeMs = 0.0;
    if (! firstUpdate)
    {
      const double timeSinceLastUpdate = startUpdateTimeMs - lastUpdateTimeMs;
      const double maxLatency = UPDATE_LOOP_SLEEP_DURATION_MS + 15.;
      if (timeSinceLastUpdate > maxLatency)
      {
        Anki::Util::sEventF("audio_controller.update.sleep.slow", {{DDATA,STR(UPDATE_LOOP_SLEEP_DURATION_MS)}}, "%.2f", timeSinceLastUpdate);
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_AC_SLEEP_TIME_DIAGNOSTICS
  
  // Tick Music Conductor & Audio Engine
  _musicConductor->UpdateTick();
  
  // NOTE: Don't need time delta
  _audioEngine->Update( 0.0 );


#if ENABLE_AC_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = UPDATE_LOOP_SLEEP_DURATION_MS;
    if (updateLengthMs > maxUpdateDuration)
    {
      Anki::Util::sEventF("audio_controller.update.run.slow", {{DDATA,STR(UPDATE_LOOP_SLEEP_DURATION_MS)}}, "%.2f", updateLengthMs);
    }
  }
#endif // ENABLE_AC_RUN_TIME_DIAGNOSTICS


#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::MoveCallbackContextToGarbageCollector( const AudioEngine::AudioCallbackContext* callbackContext )
{
  ASSERT_NAMED( nullptr != callbackContext, "AudioController.MoveCallbackContextToGarbageCollector Callback Context is \
                NULL");
  PRINT_NAMED_INFO( "AudioController.MoveCallbackContextToGarbageCollector", "Add PlayId: %d Callback Context to \
                    garbagecollector", callbackContext->GetPlayId() );
  // FIXME: Is there a better way of doing this?
  ClearGarbageCollector();
  
  // Move context from EventCallbackMap to CallbackGarbageCollector
  const auto it = _eventCallbackContexts.find( callbackContext->GetPlayId() );
  if ( it != _eventCallbackContexts.end() ) {
    ASSERT_NAMED( it->second == callbackContext, "AudioController.MoveCallbackContextToGarbageCollector PlayId dose \
                  NOT match Callback Context" );
    // Move to GarbageCollector
    it->second->ClearCallbacks();
    _callbackGarbageCollector.emplace_back( it->second );
    _eventCallbackContexts.erase( it );
  }
  else {
    ASSERT_NAMED( it != _eventCallbackContexts.end(), ( "AudioController.MoveCallbackContextToGarbageCollector Can NOT \
                  find PlayId: " + std::to_string( callbackContext->GetPlayId() )).c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::ClearGarbageCollector()
{
  std::for_each(_callbackGarbageCollector.begin(),
                _callbackGarbageCollector.end(),
                [](AudioEngine::AudioCallbackContext* aContext){ Util::SafeDelete( aContext ); } );
  _callbackGarbageCollector.clear();
}


// Debug Cozmo PlugIn Logs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if HijackAudioPlugInDebugLogs

double ConvertToMiliSec(unsigned long long int miliSeconds) {
  return (double)miliSeconds / 1000000.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::PrintPlugInLog() {
  
  unsigned long long int postTime = 0, createTime = 0, updateTime = 0;
  bool isFirstUpdateLog = false;
  for (auto& aLog : _plugInLog) {
    switch (aLog.LogType) {
      case LogEnumType::Post:
      {
        postTime = aLog.TimeInNanoSec;
        
        printf("----------------------------------------------\n \
               Post Event %s - time: %f ms\n", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ));
      }
        break;
        
      case LogEnumType::CreatePlugIn:
      {
        createTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = true;
        
        printf("Create PlugIn %s - time: %f ms\n \
               - Post -> Create time delta = %f ms\n", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ), ConvertToMiliSec( createTime - postTime ));
      }
        break;
        
      case LogEnumType::Update:
      {
        printf("Update %s - time: %f ms\n", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ));
        
        
        if ( isFirstUpdateLog ) {
          printf("- Post -> Update time delta = %f ms\n \
                  - Create -> Update time delta = %f ms\n", ConvertToMiliSec( aLog.TimeInNanoSec - postTime ), ConvertToMiliSec( aLog.TimeInNanoSec - createTime ));
        }
        else {
          printf("- Previous Update -> Update time delta = %f ms\n", ConvertToMiliSec( aLog.TimeInNanoSec - updateTime ));
          
        }
        
        updateTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = false;
      }
        break;
        
      case LogEnumType::DestoryPlugIn:
      {
        printf("Destory Plugin %s - time: %f ms\n \
               ----------------------------------------------\n", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ));
      }
        break;
        
      default:
        break;
    }
  }
  _plugInLog.clear();
}
#endif

  
} // Audio
} // Cozmo
} // Anki
