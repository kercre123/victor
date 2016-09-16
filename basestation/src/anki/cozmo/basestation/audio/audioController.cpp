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
#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "anki/cozmo/basestation/audio/musicConductor.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/audio/audioBusses.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include "clad/audio/audioStateTypes.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"
#include <sstream>
#include <unordered_map>

#ifdef ANDROID
#include "util/jni/jniUtils.h"
#endif

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

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


namespace Anki {
namespace Cozmo {
namespace Audio {
  
using namespace AudioEngine;
  
const char* AudioController::kAudioLogChannelName = "Audio";

// Resolve audio asset file path
static bool ResolvePathToAudioFile( const std::string&, const char*, char*, const size_t );
#if USE_AUDIO_ENGINE
  // Setup Ak Logging callback
  static void AudioEngineLogCallback( uint32_t, const char*, ErrorLevel, AudioPlayingId, AudioGameObject );
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::AudioController( const CozmoContext* context )
: _pluginInterface( new AudioControllerPluginInterface( *this ) )
{
  // Setup Music Conductor
  _musicConductor = new MusicConductor( *this,
                                        static_cast<AudioGameObject>( GameObjectType::Default ),
                                        Util::numeric_cast<AudioStateGroupId>( GameState::StateGroupType::Music ),
                                        Util::numeric_cast<AudioEventId>( GameEvent::Music::Play ),
                                        Util::numeric_cast<AudioEventId>( GameEvent::Music::Stop ) );

#if USE_AUDIO_ENGINE
  {
    ASSERT_NAMED(nullptr != context, "AudioController.AudioController.CozmocContex.IsNull");
    
    const Util::Data::DataPlatform* dataPlatfrom = context->GetDataPlatform();
    const std::string assetPath = dataPlatfrom->pathToResource(Util::Data::Scope::Resources, "sound/" );

    // If assets don't exist don't init the Audio engine
#ifndef ANDROID
    const bool assetsExist = Util::FileUtils::DirectoryExists( assetPath );

    if ( !assetsExist ) {
      PRINT_NAMED_ERROR("AudioController.AudioController", "Audio Assets do NOT exists - Ignore if Unit Test");
      return;
    }
#endif
    
    // Config Engine
    AudioEngine::SetupConfig config{};
    // Assets
    config.assetFilePath = assetPath;
    // Path resolver function
    config.pathResolver = std::bind(&ResolvePathToAudioFile, assetPath,
                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    
    // Add Assets Zips to list.

#ifdef ANDROID
    // Add to the APK file and OBB file (if it exists) to the list of valid zip files where the audio assets can be
    {
      auto envWrapper = Util::JNIUtils::getJNIEnvWrapper();
      JNIEnv* env = envWrapper->GetEnv();
      JNI_CHECK(env);

      // Get a handle to the activity instance
      Util::JClassHandle contextClass{env->FindClass("android/content/ContextWrapper"), env};
      Util::JObjectHandle activity{Util::JNIUtils::getUnityActivity(env), env};

      // Get a handle to the object representing the OBB folder
      jmethodID obbDirMethodID = env->GetMethodID(contextClass.get(), "getObbDir", "()Ljava/io/File;");
      Util::JObjectHandle obbDir{env->CallObjectMethod(activity.get(), obbDirMethodID), env};

      // Get the obb folder path as a string
      Util::JClassHandle fileClass{env->FindClass("java/io/File"), env};
      std::string obbDirPath = Util::JNIUtils::getStringFromObjectMethod(env, fileClass.get(), obbDir.get(), "getAbsolutePath", "()Ljava/lang/String;");
      if (!obbDirPath.empty())
      {
        PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AudioController", "obb Dir path: %s", obbDirPath.c_str());
        
        // Get the list of files in the folder
        jmethodID listMethodID = env->GetMethodID(fileClass.get(), "list", "()[Ljava/lang/String;");
        Util::JObjectArrayHandle filesInOBBDir{(jobjectArray)env->CallObjectMethod(obbDir.get(), listMethodID), env};
        if (filesInOBBDir != nullptr)
        {
          // Get the first file in the folder and add it to to the list of paths
          if (env->GetArrayLength(filesInOBBDir.get()) > 0)
          {
            Util::JObjectHandle obbFileObject{env->GetObjectArrayElement(filesInOBBDir.get(), 0), env};
            std::string obbFile(env->GetStringUTFChars((jstring)obbFileObject.get(), 0));

            std::string obbZipPath = obbDirPath + "/" + obbFile + "?" + "assets/cozmo_resources/sound/AudioAssets.zip";
            config.pathToZipFiles.push_back(std::move(obbZipPath));
            PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AudioController", "OBB file: %s", obbZipPath.c_str());
          }
          else 
          {
            PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AudioController", "No OBB file in the OBB folder");
          }
        }
        else 
        {
          PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AudioController", "No OBB folder");
        }
      }

      std::string apkPath = Util::JNIUtils::getStringFromObjectMethod(env, contextClass.get(), activity.get(), "getPackageCodePath", "()Ljava/lang/String;");
      std::string apkZipPath = apkPath + "?" + "assets/cozmo_resources/sound/AudioAssets.zip";
      config.pathToZipFiles.push_back(std::move(apkZipPath));
      PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AudioController", "APK file: %s", apkZipPath.c_str());
    }
#else
    // iOS & Mac Platfroms
    // Note: We only have 1 file at the moment this will change when we brake up assets for RAMS
    std::string zipAssets = assetPath + "AudioAssets.zip";
    if (Util::FileUtils::FileExists(zipAssets)) {
      config.pathToZipFiles.push_back(std::move(zipAssets));
    }
    else {
      PRINT_NAMED_ERROR("AudioController.AudioController", "Audio Assets not found: '%s'", zipAssets.c_str());
    }
#endif

    // Set Local
    config.audioLocal = AudioLocaleType::EnglishUS;
    // Engine Memory
    config.defaultMemoryPoolSize      = ( 4 * 1024 * 1024 );
    config.defaultLEMemoryPoolSize    = ( 4 * 1024 * 1024 );
    config.defaultPoolBlockSize       = 1024;
    config.defaultMaxNumPools         = 30;
    config.enableGameSyncPreparation  = true;
    
    // Create Engine
    _audioEngine = new AudioEngineController();
    
    // Start your Engines!!!
    _isInitialized = _audioEngine->Initialize( config );
    
    // Setup Engine Logging callback
    _audioEngine->SetLogOutput( AudioEngine::ErrorLevel::All, &AudioEngineLogCallback );
    
    // If we're using the audio engine, assert that it was successfully initialized.
    ASSERT_NAMED(_isInitialized, "AudioController.Initialize Audio Engine fail");
  }
#endif // USE_AUDIO_ENGINE
  
  // The audio engine was initialized correctly, so now let's setup everything else
  if ( _isInitialized )
  {
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
    
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::~AudioController()
{
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
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.PostAudioEvent",
                "Event: '%s' GameObj: %u CallbackFlag: %d PlayId: %d Initalized: %c",
                eventName.c_str(), static_cast<uint32_t>(gameObjectId),
                (nullptr != callbackContext) ? callbackContext->GetCallbackFlags() : 0,
                playingId, _isInitialized ? 'Y' : 'N');
  
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
    _plugInLog.emplace_back( TimeLog( LogEnumType::Post, "EventId: " + std::to_string(eventId),
                                      Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  }
#endif
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.PostAudioEvent",
                "EventId: %u GameObj: %u CallbackFlag: %d PlayId: %d Initalized: %c",
                eventId, static_cast<uint32_t>(gameObjectId),
                (nullptr != callbackContext) ? callbackContext->GetCallbackFlags() : 0,
                playingId, _isInitialized ? 'Y' : 'N');
  
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::StopAllAudioEvents( AudioEngine::AudioGameObject gameObjectId )
{
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    _audioEngine->StopAllAudioEvents( gameObjectId );
  }
#endif
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.StopAllAudioEvents",
                "GameObj: %u Initalized: %c",
                static_cast<uint32_t>(gameObjectId),
                _isInitialized ? 'Y' : 'N');
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
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.SetState",
                "StateGroupId: %u StateId: %u Success: %c Initalized: %c",
                stateGroupId, stateId,
                success ? 'Y' : 'N',
                _isInitialized ? 'Y' : 'N');
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetSwitchState( AudioEngine::AudioSwitchGroupId switchGroupId,
                                      AudioEngine::AudioSwitchStateId switchStateId,
                                      AudioEngine::AudioGameObject gameObjectId ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetSwitch( switchGroupId, switchStateId, gameObjectId );
  }
#endif
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.SetSwitchState",
                "SwitchGroupId: %u SwitchStateId: %u GameObj: %u Success: %c Initalized: %c",
                switchGroupId, switchStateId,
                static_cast<uint32_t>(gameObjectId),
                success ? 'Y' : 'N',
                _isInitialized ? 'Y' : 'N');
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetParameter( AudioEngine::AudioParameterId parameterId,
                                    AudioEngine::AudioRTPCValue rtpcValue,
                                    AudioEngine::AudioGameObject gameObjectId,
                                    AudioEngine::AudioTimeMs valueChangeDuration,
                                    AudioEngine::AudioCurveType curve ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetRTPCValue( parameterId, rtpcValue, gameObjectId, valueChangeDuration, curve );
  }
#endif
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.SetParameter",
                "ParameterId: %u Value: %f GameObj: %u Duration: %d Curve: %hhu Success: %c Initalized: %c",
                parameterId, rtpcValue,
                static_cast<uint32_t>(gameObjectId),
                valueChangeDuration, curve,
                success ? 'Y' : 'N',
                _isInitialized ? 'Y' : 'N');
  
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
  
  PRINT_CH_INFO(kAudioLogChannelName,
                "AudioController.SetParameterWithPlayingId",
                "ParameterId: %u Value: %f PlayingId: %u Duration: %d Curve: %hhu Success: %c Initalized: %c",
                parameterId, rtpcValue, playingId,
                valueChangeDuration, curve,
                success ? 'Y' : 'N',
                _isInitialized ? 'Y' : 'N');
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* AudioController::RegisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObjectId,
                                                             AudioEngine::AudioPluginId pluginId )
{
  RobotAudioBuffer* buffer = new RobotAudioBuffer();
  const auto it = _robotAudioBufferIdMap.emplace( pluginId, buffer );
  _gameObjectPluginIdMap.emplace( gameObjectId, pluginId );
  
  if ( !it.second ) {
    // If buffer already exist
    delete buffer;
    PRINT_NAMED_ERROR( "AudioController.RegisterRobotAudioBuffer",
                       "Robot buffer already exist! PluginId: %d GameObject: %u",
                       pluginId, static_cast<uint32_t>( gameObjectId ) );
  }
  
  return it.first->second;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::UnregisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObject,
                                                 	AudioEngine::AudioPluginId pluginId)
{
  const auto it = _robotAudioBufferIdMap.find( pluginId );
  if (it != _robotAudioBufferIdMap.end()) {
    Util::SafeDelete(it->second);
    _robotAudioBufferIdMap.erase(it);
  } else {
    PRINT_NAMED_ERROR( "AudioController.UnregisterRobotAudioBuffer",
                      "Robot buffer doesn't exist! PluginId: %d GameObject: %u",
                      pluginId, static_cast<uint32_t>( gameObject ) );
  }
  
  const auto it2 = _gameObjectPluginIdMap.find( gameObject );
  if (it2 != _gameObjectPluginIdMap.end()) {
    _gameObjectPluginIdMap.erase(it2);
  } else {	
    PRINT_NAMED_ERROR( "AudioController.UnregisterRobotAudioBuffer",
                      "Robot buffer doesn't exist! PluginId: %d GameObject: %u",
                      pluginId, static_cast<uint32_t>( gameObject ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* AudioController::GetRobotAudioBufferWithGameObject( AudioEngine::AudioGameObject gameObjectId ) const
{
  const auto it = _gameObjectPluginIdMap.find( gameObjectId );
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
bool AudioController::SetGameObjectAuxSendValues( AudioEngine::AudioGameObject gameObjectId,
                                                  const AuxSendList& auxSendValues )
{
  bool success = false;
  
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetGameObjectAuxSendValues( gameObjectId, auxSendValues );
  }
#endif
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetGameObjectOutputBusVolume( AudioEngine::AudioGameObject gameObjectId,
                                                    AudioEngine::AudioReal32 controlVolume )
{
  bool success = false;
  
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    success = _audioEngine->SetGameObjectOutputBusVolume( gameObjectId, controlVolume );
  }
#endif
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::ProcessAudioQueue() const
{
#if USE_AUDIO_ENGINE
  // NOTE: Don't need time delta
  _audioEngine->Update( 0 );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::AppIsInFocus( const bool inFocus )
{
  PRINT_CH_INFO(kAudioLogChannelName, "AudioController.AppIsInFocus", "inFocus %c", inFocus ? 'Y' : 'N');
  // Post App State Event
  const GameEvent::Coz_App event = inFocus ? GameEvent::Coz_App::Enter_Foreground : GameEvent::Coz_App::Enter_Background;
  PostAudioEvent(static_cast<const AudioEngine::AudioEventId>(event));
}

// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::SetupHijackAudioPlugInAndRobotAudioBuffers()
{
#if USE_AUDIO_ENGINE
  using namespace PlugIns;
  // Setup CozmoPlugIn & RobotAudioBuffer
  _hijackAudioPlugIn = new HijackAudioPlugIn( static_cast<uint32_t>( AnimConstants::AUDIO_SAMPLE_RATE ),
                                              static_cast<uint16_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) );

  // Setup Callbacks
  _hijackAudioPlugIn->SetCreatePlugInCallback( [this] ( const uint32_t plugInId )
  {
    PRINT_CH_INFO(kAudioLogChannelName, "AudioController.HijackAudioPlugin", "Create Plugin id: %d callback", plugInId);
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );

    if ( buffer != nullptr ) {
      buffer->PrepareAudioBuffer();
    }
    else {
      // Buffer doesn't exist
      PRINT_CH_INFO(AudioController::kAudioLogChannelName,
                    "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.CreatePlugInCallback",
                    "PluginId: %u NotFound", plugInId);
    }
    
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::CreatePlugIn, "",
                                      Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
  } );
  
  _hijackAudioPlugIn->SetDestroyPluginCallback( [this] ( const uint32_t plugInId )
  {
    PRINT_CH_INFO(kAudioLogChannelName, "AudioController.HijackAudioPlugin", "Destroy Plugin id: %d callback", plugInId);
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );

    // Done with voice clear audio buffer
    if ( buffer != nullptr ) {
      buffer->CloseAudioBuffer();
    }
    else {
      // Buffer doesn't exist
      PRINT_CH_INFO(AudioController::kAudioLogChannelName,
                    "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.DestroyPluginCallback",
                    "PluginId: %u NotFound", plugInId);
    }
    
    
#if HijackAudioPlugInDebugLogs
    _plugInLog.emplace_back( TimeLog( LogEnumType::DestroyPlugIn, "",
                                      Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
    PrintPlugInLog();
#endif
  } );
  
  _hijackAudioPlugIn->SetProcessCallback( [this] ( const uint32_t plugInId, const AudioReal32* samples, const uint32_t sampleCount )
  {
    RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
    if ( buffer != nullptr ) {
      buffer->UpdateBuffer( samples, sampleCount );
    }
    else {
      // Buffer doesn't exist
      PRINT_CH_INFO(AudioController::kAudioLogChannelName,
                    "AudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.ProcessCallback",
                    "PluginId: %u NotFound", plugInId);
    }
     
#if HijackAudioPlugInDebugLogs
     _plugInLog.emplace_back( TimeLog( LogEnumType::Update, "SampleCount: " + std::to_string(sampleCount),
                                       Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
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
  // Tick Music Conductor & Audio Engine
  _musicConductor->UpdateTick();
  ProcessAudioQueue();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioController::MoveCallbackContextToGarbageCollector( const AudioEngine::AudioCallbackContext* callbackContext )
{
  ASSERT_NAMED( nullptr != callbackContext, "AudioController.MoveCallbackContextToGarbageCollector Callback Context is \
                NULL");
  PRINT_CH_DEBUG(kAudioLogChannelName,
                 "AudioController.MoveCallbackContextToGarbageCollector",
                 "Add PlayId: %d Callback Context to garbagecollector",
                 callbackContext->GetPlayId());
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ResolvePathToAudioFile( const std::string& dataPlatformResourcePath,
                             const char* inName,
                             char* outPath,
                             const size_t outPathLen )
{
  if (!inName || !outPath || !outPathLen) {
    return false;
  }
  *outPath = '\0';
  if (dataPlatformResourcePath.empty()) {
    return false;
  }
  std::string path = dataPlatformResourcePath + std::string(inName);
  if (path.empty()) {
    return false;
  }
  (void) strncpy(outPath, path.c_str(), outPathLen);
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if USE_AUDIO_ENGINE
// Setup Ak Logging callback
void AudioEngineLogCallback( uint32_t akErrorCode,
                            const char* errorMessage,
                            ErrorLevel errorLevel,
                            AudioPlayingId playingId,
                            AudioGameObject gameObjectId )
{
  std::ostringstream logStream;
  logStream << "ErrorCode: " << akErrorCode << " Message: '" << ((nullptr != errorMessage) ? errorMessage : "")
  << "' LevelBitFlag: " << (uint32_t)errorLevel << " PlayingId: " << playingId << " GameObjId: " << gameObjectId;
  
  if (((uint32_t)errorLevel & (uint32_t)ErrorLevel::Message) == (uint32_t)ErrorLevel::Message) {
    PRINT_CH_INFO(AudioController::kAudioLogChannelName,
                  "AudioEngineLog",
                  "%s", logStream.str().c_str());
  }
  
  if (((uint32_t)errorLevel & (uint32_t)ErrorLevel::Error) == (uint32_t)ErrorLevel::Error) {
    PRINT_NAMED_ERROR("AudioController.WwiseLogError", "%s", logStream.str().c_str());
  }
}
#endif
  

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
        PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                            "----------------------------------------------\n \
                            Post Event %s - time: %f ms", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ));
      }
        break;
        
      case LogEnumType::CreatePlugIn:
      {
        createTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = true;
        
        PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                            "Create PlugIn %s - time: %f ms\n - Post -> Create time delta = %f ms\n",
                            aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ),
                            ConvertToMiliSec( createTime - postTime ));
      }
        break;
        
      case LogEnumType::Update:
      {
        PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                            "Update %s - time: %f ms\n", aLog.Msg.c_str(), ConvertToMiliSec( aLog.TimeInNanoSec ));
        
        
        if ( isFirstUpdateLog ) {
          PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                              "- Post -> Update time delta = %f ms\n - Create -> Update time delta = %f ms\n",
                              ConvertToMiliSec( aLog.TimeInNanoSec - postTime ),
                              ConvertToMiliSec( aLog.TimeInNanoSec - createTime ));
        }
        else {
          PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                              "- Previous Update -> Update time delta = %f ms\n",
                              ConvertToMiliSec( aLog.TimeInNanoSec - updateTime ));
          
        }
        
        updateTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = false;
      }
        break;
        
      case LogEnumType::DestroyPlugIn:
      {
        PRINT_NAMED_WARNING("AudioController.PrintPlugInLog",
                            "Destroy Plugin %s - time: %f ms\n ----------------------------------------------",
                            aLog.Msg.c_str(),
                            ConvertToMiliSec( aLog.TimeInNanoSec ));
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
