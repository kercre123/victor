/***********************************************************************************************************************
 *
 *  AudioEngineController
 *  Audio
 *
 *  Created by Jarrod Hatfield on 1/29/15.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 ***********************************************************************************************************************/

#include "audioEngine/audioDefines.h"
#include "audioEngine/audioEngineController.h"
#include "audioEngine/audioScene.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/musicConductor.h"
#include "engine/audioFilePackageLowLevelIOBlocking.h"
#include "engine/wwiseComponent.h" // TODO: Confirm this can be compiled out
#include "json/json.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/string/stringUtils.h"
#include <assert.h>
#include <vector>
#include <algorithm>

// Debug logs are off by default
#ifndef HijackAudioPlugInDebugLogs
#define HijackAudioPlugInDebugLogs 0
#endif

// Allow the build to include/exclude the audio libs
//#define EXCLUDE_ANKI_AUDIO_LIBS 0

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#else
// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0
#endif

#define AUDIO_DEBUG_LOG_LEVEL 2
// Log Levels
// 0 - No logs
// 1 - Events
// 2 - States, Switches & Stop All Events
// 3 - Parameters

namespace Anki {
namespace AudioEngine {

  
const char* AudioEngineController::kLogChannelName = "Audio";

static std::mutex sCallbackQueueMutex;
static std::vector<std::pair<AudioCallbackContext*, std::unique_ptr<const AudioCallbackInfo>>> sCallbackQueue;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineController::AudioEngineController()
{
#if USE_AUDIO_ENGINE
  _engineComponent.reset(new WwiseComponent());
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineController::~AudioEngineController()
{
  ClearGarbageCollector();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::InitializeAudioEngine( const SetupConfig& config )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  _isInitialized = _engineComponent->Initialize( config );
  success = _isInitialized;
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::InitializeMusicConductor( const MusicConductorConfig& config )
{
  _musicConductor.reset( new MusicConductor( *this, config ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::InitializePluginInterface()
{
  _pluginInterface.reset( new PlugIns::AnkiPluginInterface() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetLogOutput( ErrorLevel errorLevel, LogCallbackFunc logCallback )
{
#if USE_AUDIO_ENGINE
  return _engineComponent->SetLogOutput( errorLevel, logCallback );
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::WriteProfilerCapture( bool write, const std::string& fileName )
{
#if USE_AUDIO_ENGINE
  return _engineComponent->WriteProfilerCapture( write, fileName );
#endif // USE_AUDIO_ENGINE
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::WriteAudioOutputCapture( bool write, const std::string& fileName )
{
#if USE_AUDIO_ENGINE
  return _engineComponent->WriteAudioOutputCapture( write, fileName );
#endif // USE_AUDIO_ENGINE
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::AddZipFiles( const std::vector<std::string>& pathsToZipFiles )
{
#if USE_AUDIO_ENGINE
  _engineComponent->AddZipFiles( pathsToZipFiles );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::CheckFileExists( const std::string& fileName )
{
  bool exists = false;
#if USE_AUDIO_ENGINE
  exists = _engineComponent->CheckFileExists( fileName );
#endif  
  return exists;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineCallbackId AudioEngineController::RegisterGlobalCallback( AudioEngineCallbackFlag callbackFlag,
                                                                     AudioEngineCallbackFunc&& callbackFunc )
{
  AudioEngineCallbackId callbackId = kInvalidAudioEngineCallbackId;
#if USE_AUDIO_ENGINE
  callbackId = _engineComponent->RegisterGlobalCallback( callbackFlag, std::move(callbackFunc) );
#endif
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::UnregisterGlobalCallback( AudioEngineCallbackId callbackId )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  success = _engineComponent->UnregisterGlobalCallback( callbackId );
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlayingId AudioEngineController::PostAudioEvent( const std::string& eventName,
                                                      AudioGameObject gameObjectId,
                                                      AudioCallbackContext* callbackContext )
{
  AudioPlayingId playingId = kInvalidAudioPlayingId;
  const AudioCallbackFlag flags = ((nullptr != callbackContext) ?
                                   callbackContext->GetCallbackFlags() : AudioCallbackFlag::NoCallbacks);
  (void) flags;

#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    const AudioEventId eventId = GetAudioIdFromString( eventName );
    playingId = PostAudioEvent( eventId, gameObjectId, callbackContext );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 1) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.PostAudioEvent",
                   "Event: '%s' GameObj: %u CallbackFlag: %d PlayId: %d Initialized: %c",
                   eventName.c_str(), static_cast<uint32_t>(gameObjectId),
                   flags, playingId, _isInitialized ? 'Y' : 'N');
  }  
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlayingId AudioEngineController::PostAudioEvent( AudioEventId eventId,
                                                      AudioGameObject gameObjectId,
                                                      AudioCallbackContext* callbackContext )
{
  AudioPlayingId playingId = kInvalidAudioPlayingId;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    playingId = _engineComponent->PostEvent( eventId, gameObjectId, callbackContext );
    if ( kInvalidAudioPlayingId != playingId &&
        nullptr != callbackContext ) {
      callbackContext->SetPlayId( playingId );
      callbackContext->SetDestroyCallbackFunc( [this] ( const AudioCallbackContext* thisContext )
                                              {
                                                MoveCallbackContextToGarbageCollector( thisContext );
                                              } );
      std::lock_guard<std::mutex> lock( _callbackContextMutex );
      _callbackContextMap.emplace( playingId, callbackContext );
    }
    else if ( kInvalidAudioPlayingId == playingId &&
             nullptr != callbackContext ) {
      // Event Failed and there is a callback
      // Perform Error callback for Event Failed and delete callbackContext
      callbackContext->HandleCallback( AudioErrorCallbackInfo( gameObjectId,
                                                               kInvalidAudioPlayingId,
                                                               eventId,
                                                               AudioCallbackErrorType::EventFailed ) );
      Anki::Util::SafeDelete( callbackContext );
    }
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::Post, "EventId: " + std::to_string(eventId),
                                      Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 1) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.PostAudioEvent",
                   "EventId: %u GameObj: %u CallbackFlag: %d PlayId: %d Initialized: %c",
                   eventId, static_cast<uint32_t>(gameObjectId),
                   (nullptr != callbackContext) ? callbackContext->GetCallbackFlags() : 0,
                   playingId, _isInitialized ? 'Y' : 'N');
  }
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::StopAllAudioEvents( AudioGameObject gameObjectId )
{
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    _engineComponent->StopAllAudioEvents( gameObjectId );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 2) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.StopAllAudioEvents",
                   "GameObj: %u Initialized: %c",
                   static_cast<uint32_t>(gameObjectId),
                   _isInitialized ? 'Y' : 'N');
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetState( AudioStateGroupId stateGroupId,
                                      AudioStateId stateId ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetState( stateGroupId, stateId );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 2) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.SetState",
                   "StateGroupId: %u StateId: %u Success: %c Initialized: %c",
                   stateGroupId, stateId,
                   success ? 'Y' : 'N',
                   _isInitialized ? 'Y' : 'N');
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetSwitchState( AudioSwitchGroupId switchGroupId,
                                            AudioSwitchStateId switchStateId,
                                            AudioGameObject gameObjectId ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetSwitch( switchGroupId, switchStateId, gameObjectId );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 2) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.SetSwitchState",
                   "SwitchGroupId: %u SwitchStateId: %u GameObj: %u Success: %c Initialized: %c",
                   switchGroupId, switchStateId,
                   static_cast<uint32_t>(gameObjectId),
                   success ? 'Y' : 'N',
                   _isInitialized ? 'Y' : 'N');
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetParameter( AudioParameterId parameterId,
                                          AudioRTPCValue rtpcValue,
                                          AudioGameObject gameObjectId,
                                          AudioTimeMs valueChangeDuration,
                                          AudioCurveType curve ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetRTPCValue( parameterId, rtpcValue, gameObjectId, valueChangeDuration, curve );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 3) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.SetParameter",
                   "ParameterId: %u Value: %f GameObj: %u Duration: %d Curve: %hhu Success: %c Initialized: %c",
                   parameterId, rtpcValue,
                   static_cast<uint32_t>(gameObjectId),
                   valueChangeDuration, curve,
                   success ? 'Y' : 'N',
                   _isInitialized ? 'Y' : 'N');
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetParameterWithPlayingId( AudioParameterId parameterId,
                                                       AudioRTPCValue rtpcValue,
                                                       AudioPlayingId playingId,
                                                       AudioTimeMs valueChangeDuration,
                                                       AudioCurveType curve ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetRTPCValueWithPlayingId( parameterId, rtpcValue, playingId, valueChangeDuration, curve );
  }
#endif
  
  if (AUDIO_DEBUG_LOG_LEVEL >= 3) {
    PRINT_CH_DEBUG(kLogChannelName,
                   "AudioEngineController.SetParameterWithPlayingId",
                   "ParameterId: %u Value: %f PlayingId: %u Duration: %d Curve: %hhu Success: %c Initialized: %c",
                   parameterId, rtpcValue, playingId,
                   valueChangeDuration, curve,
                   success ? 'Y' : 'N',
                   _isInitialized ? 'Y' : 'N');
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::GetParameterValue( AudioParameterId parameterId,
                                               AudioGameObject gameObject,
                                               AudioPlayingId playingId,
                                               AudioRTPCValue& out_rtpcValue,
                                               AudioRTPCValueType& inOut_rtpcValueType )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->GetRTPCValue( parameterId, gameObject, playingId, out_rtpcValue, inOut_rtpcValueType );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::RegisterGameObject( AudioGameObject gameObjectId, const std::string& optionalName )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized )
  {
    // Check if Id has already been registered
    const auto registeredGameObjIt = _registeredGameObjects.find( gameObjectId );
    if ( registeredGameObjIt != _registeredGameObjects.end() ) {
      AUDIO_LOG( "AudioEngineController.RegisterGameObject: Can NOT register gameObjectId: %lu - %s GameObject %s \
                is already registered with that gameObjectId" ,
                (unsigned long)gameObjectId, optionalName.c_str(), registeredGameObjIt->second.c_str() );
      assert(false);
    }
    
    // Register Game Object
    success = _engineComponent->RegisterAudioGameObject( gameObjectId, optionalName );
    if ( success ) {
      _registeredGameObjects.emplace( gameObjectId, optionalName );
      success =  true;
    }
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::UnregisterGameObject( AudioGameObject gameObjectId )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized )
  {
    success = _engineComponent->UnregisterAudioGameObject( gameObjectId );
    if ( success ) {
      _registeredGameObjects.erase( gameObjectId );
      success = true;
    }
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::SetDefaultListeners( const std::vector<AudioGameObject>& listeners)
{
#if USE_AUDIO_ENGINE
  if (_isInitialized) {
    _engineComponent->SetDefaultListeners( listeners );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetGameObjectAuxSendValues( AudioGameObject gameObjectId,
                                                        const AuxSendList& auxSendValues )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetGameObjectAuxSendValues( gameObjectId, auxSendValues );
  }
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::SetGameObjectOutputBusVolume( AudioGameObject emitterGameObj,
                                                          AudioGameObject listenerGameObj,
                                                          AudioReal32 controlVolume )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _engineComponent->SetGameObjectOutputBusVolume( emitterGameObj, listenerGameObj, controlVolume );
  }
#endif
  return success;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::Update()
{
#if USE_AUDIO_ENGINE
  if (_musicConductor != nullptr) {
    _musicConductor->UpdateTick();
  }
  ProcessAudioQueue();
  FlushCallbackQueue();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::ProcessAudioQueue() const
{
#if USE_AUDIO_ENGINE
  // NOTE: Don't need time delta
  _engineComponent->Update( 0 );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::FlushCallbackQueue()
{
#if USE_AUDIO_ENGINE
  _engineComponent->FlushCallbackQueue();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static
uint32_t AudioEngineController::GetAudioIdFromString( const std::string& name )
{
  uint32_t audioId = kInvalidAudioEventId;
#if USE_AUDIO_ENGINE
  audioId = WwiseComponent::GetAudioIdFromString( name );
#endif
  return audioId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::RegisterAudioSceneWithJsonFile( const std::string& resourcePath, std::string& out_sceneName )
{
  // Check Path
  if ( !Util::FileUtils::FileExists( resourcePath ) ) {
    PRINT_NAMED_ERROR("AudioEngineController.RegisterAudioSceneWithJsonFile", "resourcePath.DoesNotExist");
    return false;
  }
  
  // Check if file is empty
  const std::string& contents = Util::StringFromContentsOfFile( resourcePath );
  if ( contents.empty() ) {
    PRINT_NAMED_ERROR("AudioEngineController.RegisterAudioSceneWithJsonFile", "contents.IsEmpty");
    return false;
  }
  
  Json::Reader reader;
  Json::Value jsonVal;
  if ( !reader.parse(contents, jsonVal, false) ) {
    PRINT_NAMED_ERROR("AudioEngineController.RegisterAudioSceneWithJsonFile", "ParseJsonFailed");
    return false;
  }
  
  // Load Scene with Json value and register
  AudioScene scene = AudioScene(jsonVal);
  out_sceneName = scene.SceneName;
  RegisterAudioScene( std::move(scene) );
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::RegisterAudioScene( const AudioScene&& scene )
{
  _audioScenes[scene.SceneName] = std::move( scene );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::UnregisterAudioScene( const std::string& sceneName )
{
  const auto& it = _audioScenes.find( sceneName );
  if ( it != _audioScenes.end() ) {
    _audioScenes.erase( it );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::LoadAudioScene( const std::string& sceneName )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  // Check if the Scene has been loaded
  const auto& sceneIt = _audioScenes.find( sceneName );
  if ( sceneIt == _audioScenes.end() ) {
    AUDIO_LOG( "AudioEngineController.LoadAudioScene Audio Secene %s is NOT registered", sceneName.c_str() );
    return false;
  }

  // Add Zip files to search path
  // NOTE: Must be full path to zip file
  // TODO: May need to append base path to zip file names to make this more portable.
  if ( !sceneIt->second.ZipFiles.empty() ) {
    AddZipFiles( sceneIt->second.ZipFiles );
  }
  
  // Load Banks
  for ( auto& bankName : sceneIt->second.Banks ) {
    const bool success = LoadSoundbank( bankName );
    if (!success) {
      return false;
    }
  }

  // Prepare all events & default States
  if ( sceneIt->second.Events.size() > 0) {
    AudioNameList eventNames;
    // TODO: Load default game group events
    for ( auto& event : sceneIt->second.Events ) {
      eventNames.push_back( event.EventName );
    }
    _engineComponent->PrepareEvents( eventNames );
  }

  _activeSceneNames.emplace_back( sceneName );
  success = true;
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::UnloadAudioScene( const std::string& sceneName )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  const auto& audioSceneIt = _audioScenes.find( sceneName );
  if ( audioSceneIt == _audioScenes.end() ) {
    AUDIO_LOG( "AudioEngineController.UnloadAudioScene Audio Secene %s is NOT registered", sceneName.c_str() );
    return false;
  }

  // Unload events
  if ( audioSceneIt->second.Events.size() > 0 ) {
    AudioNameList eventNames;
    for ( auto& event : audioSceneIt->second.Events ) {
      eventNames.push_back( event.EventName );
    }
    _engineComponent->PrepareEvents( eventNames, false);
  }

  // Unload Banks
  for ( auto& bankName : audioSceneIt->second.Banks ) {
    UnloadSoundbank( bankName );
  }

  auto activeSceneIt = std::find( _activeSceneNames.begin(), _activeSceneNames.end(), sceneName );
  if ( activeSceneIt == _activeSceneNames.end() ) {
    AUDIO_LOG( "AudioEngineController.UnloadAudioScene Audio Secene %s is NOT in Active list", sceneName.c_str() );
  }
  _activeSceneNames.erase( activeSceneIt );
  success = true;
#endif
  return success;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AudioScene* AudioEngineController::GetAudioScene( const std::string& sceneName ) const
{
  const AudioScene* scene = nullptr;
#if USE_AUDIO_ENGINE
  // Check if the Scene has been loaded
  const auto& sceneIt = _audioScenes.find( sceneName );
  if ( sceneIt == _audioScenes.end() ) {
    AUDIO_LOG( "AudioEngineController.GetAudioScene Audio Secene %s is NOT registered", sceneName.c_str() );
    return nullptr;
  }
  scene = &sceneIt->second;
#endif
  return scene;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::LoadSoundbank( const std::string& bankName )
{
  bool success = false;
#if USE_AUDIO_ENGINE

  // Check if bank is already loaded
  const auto& it = std::find( _loadedBankNames.begin(), _loadedBankNames.end(), bankName );
  if ( it == _loadedBankNames.end() ) {
    // Didn't find bank, load it
    if ( _engineComponent->LoadBank( bankName ) ) {
      _loadedBankNames.push_back( bankName );
      success = true;
      PRINT_CH_DEBUG(kLogChannelName, "AudioEngineController.LoadSoundbank",
                     "Successfully loaded sound bank '%s'", bankName.c_str() );
    }
    else {
      PRINT_NAMED_WARNING("AudioEngineController.LoadSoundbank",
                          "Failed to load soundbank '%s'", bankName.c_str() );
    }
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineController.LoadSoundbank",
                        "Soundbank '%s' is already loaded", bankName.c_str() );
  }
  
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioEngineController::UnloadSoundbank( const std::string& bankName )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  
  // Check if bank is loaded
  const auto& it = std::find( _loadedBankNames.begin(), _loadedBankNames.end(), bankName );
  if ( it != _loadedBankNames.end() ) {
    // Found Bank, unload it
    if ( _engineComponent->UnloadBank( bankName ) ) {
      _loadedBankNames.erase( it );
      success = true;
      PRINT_CH_DEBUG(kLogChannelName, "AudioEngineController.UnloadSoundbank",
                     "Successfully unloaded soundbank '%s'", bankName.c_str() );
    }
    else {
      PRINT_NAMED_WARNING("AudioEngineController.UnloadSoundbank",
                          "Failed to unload soundbank '%s'", bankName.c_str() );
    }
  }
  else {
    PRINT_NAMED_WARNING("AudioEngineController.UnloadSoundBank",
                        "Soundbank '%s' is NOT loaded", bankName.c_str() );
  }
  
#endif
  return success;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Event Callback Metods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::MoveCallbackContextToGarbageCollector( const AudioCallbackContext* callbackContext )
{
  DEV_ASSERT(nullptr != callbackContext, "AudioEngineController.MoveCallbackContextToGarbageCollector.CallbackContextIsNull");
  
  // FIXME: Is there a better way of doing this?
  ClearGarbageCollector();
  
  // Move context from EventCallbackMap to CallbackGarbageCollector
  std::lock_guard<std::mutex> lock(_callbackContextMutex);
  const auto it = _callbackContextMap.find( callbackContext->GetPlayId() );
  if ( it != _callbackContextMap.end() ) {
    DEV_ASSERT(it->second == callbackContext,
               "AudioEngineController.MoveCallbackContextToGarbageCollector.PlayIdDoesNotMatchCallbackContext");
    // Move to GarbageCollector
    it->second->ClearCallbacks();
    _callbackGarbageCollector.emplace_back( it->second );
    _callbackContextMap.erase( it );
  }
  else {
    DEV_ASSERT(it != _callbackContextMap.end(),
               ("AudioEngineController.MoveCallbackContextToGarbageCollector.CanNotFindPlayId: "
                + std::to_string(callbackContext->GetPlayId() )).c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioEngineController::ClearGarbageCollector()
{
  std::for_each(_callbackGarbageCollector.begin(),
                _callbackGarbageCollector.end(),
                [](AudioCallbackContext* aContext){ Anki::Util::SafeDelete( aContext ); } );
  _callbackGarbageCollector.clear();
}
  

} // AudioEngine
} // Anki
