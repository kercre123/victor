/**
 * File: cozmoAudioController.cpp
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


#include "anki/cozmo/basestation/audio/cozmoAudioController.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "audioEngine/audioScene.h"
#include "audioEngine/musicConductor.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
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
#include "audioEngine/plugins/hijackAudioPlugIn.h"
#include "audioEngine/plugins/wavePortalPlugIn.h"
#else
// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0
#endif



namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine;
using namespace AudioMetaData;
  
#if USE_AUDIO_ENGINE
// Resolve audio asset file path
static bool ResolvePathToAudioFile( const std::string&, const char*, char*, const size_t );

// Setup Ak Logging callback
static void AudioEngineLogCallback( uint32_t, const char*, ErrorLevel, AudioPlayingId, AudioGameObject );
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::CozmoAudioController( const CozmoContext* context )
{
  // Setup Music Conductor
  MusicConductorConfig musicConfig{};
  musicConfig.musicGameObject = static_cast<AudioGameObject>( GameObjectType::Default );
  musicConfig.musicGroupId = Util::numeric_cast<AudioStateGroupId>( GameState::StateGroupType::Music );
  musicConfig.startEventId = Util::numeric_cast<AudioEventId>( GameEvent::Music::Play );
  musicConfig.stopEventId = Util::numeric_cast<AudioEventId>( GameEvent::Music::Stop );
  InitializeMusicConductor( musicConfig );
  
#if USE_AUDIO_ENGINE
  {
    DEV_ASSERT(nullptr != context, "CozmoAudioController.CozmoAudioController.CozmocContex.IsNull");
    
    const Util::Data::DataPlatform* dataPlatfrom = context->GetDataPlatform();
    const std::string assetPath = dataPlatfrom->pathToResource(Util::Data::Scope::Resources, "sound/" );
    
    // If assets don't exist don't init the Audio engine
#ifndef ANDROID
    const bool assetsExist = Util::FileUtils::DirectoryExists( assetPath );
    
    if ( !assetsExist ) {
      PRINT_NAMED_ERROR("CozmoAudioController.CozmoAudioController", "Audio Assets do NOT exists - Ignore if Unit Test");
      return;
    }
#endif
    
    // Config Engine
    SetupConfig config{};
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
      Util::JObjectHandle activity{Util::JNIUtils::getCurrentActivity(env), env};
      
      // Link Java VM
      config.javaVm = Util::JNIUtils::GetJvm();
      config.javaActivity = env->NewLocalRef(activity.get());
      
      // Get a handle to the object representing the OBB folder
      jmethodID obbDirMethodID = env->GetMethodID(contextClass.get(), "getObbDir", "()Ljava/io/File;");
      Util::JObjectHandle obbDir{env->CallObjectMethod(activity.get(), obbDirMethodID), env};
      
      // Get the obb folder path as a string
      Util::JClassHandle fileClass{env->FindClass("java/io/File"), env};
      std::string obbDirPath = Util::JNIUtils::getStringFromObjectMethod(env, fileClass.get(), obbDir.get(), "getAbsolutePath", "()Ljava/lang/String;");
      if (!obbDirPath.empty())
      {
        PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.CozmoAudioController", "obb Dir path: %s", obbDirPath.c_str());
        
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
            PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.CozmoAudioController", "OBB file: %s", obbZipPath.c_str());
          }
          else
          {
            PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.CozmoAudioController", "No OBB file in the OBB folder");
          }
        }
        else
        {
          PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.CozmoAudioController", "No OBB folder");
        }
      }
      
      std::string apkPath = Util::JNIUtils::getStringFromObjectMethod(env, contextClass.get(), activity.get(), "getPackageCodePath", "()Ljava/lang/String;");
      std::string apkZipPath = apkPath + "?" + "assets/cozmo_resources/sound/AudioAssets.zip";
      PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.CozmoAudioController", "APK file: %s", apkZipPath.c_str());
      config.pathToZipFiles.push_back(std::move(apkZipPath));
    }
#else
    // iOS & Mac Platfroms
    // Note: We only have 1 file at the moment this will change when we brake up assets for RAMS
    std::string zipAssets = assetPath + "AudioAssets.zip";
    if (Util::FileUtils::FileExists(zipAssets)) {
      config.pathToZipFiles.push_back(std::move(zipAssets));
    }
    else {
      PRINT_NAMED_ERROR("CozmoAudioController.CozmoAudioController", "Audio Assets not found: '%s'", zipAssets.c_str());
    }
#endif
    
    // Set Local
    config.audioLocal = AudioLocaleType::EnglishUS;
    // Engine Memory
    config.defaultMemoryPoolSize      = ( 2 * 1024 * 1024 );      // 2 MB
    config.defaultLEMemoryPoolSize    = ( 1024 * 1024 );          // 1 MB
    config.ioMemorySize               = ( (1024 + 512) * 1024 );  // 1.5 MB
    config.defaultMaxNumPools         = 30;
    config.enableGameSyncPreparation  = true;
    config.enableStreamCache          = true;
    
    
    // Start your Engines!!!
    InitializeAudioEngine( config );
    
    // Setup Engine Logging callback
    SetLogOutput( ErrorLevel::All, &AudioEngineLogCallback );
    
    // If we're using the audio engine, assert that it was successfully initialized.
    DEV_ASSERT(IsInitialized(), "CozmoAudioController.Initialize Audio Engine fail");
  }
#endif // USE_AUDIO_ENGINE
  
  // The audio engine was initialized correctly, so now let's setup everything else
  if ( IsInitialized() )
  {
#if USE_AUDIO_ENGINE
    
    // Register and Prepare Plug-Ins
    SetupPlugins();
    
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
    
    RegisterAudioScene( std::move(initScene) );
    
    LoadAudioScene( sceneTitle );
    
#endif
    
    // Register Game Objects in defined in CLAD
    RegisterCladGameObjectsWithAudioController();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::~CozmoAudioController()
{
#if USE_AUDIO_ENGINE
  {
  }
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* CozmoAudioController::RegisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObjectId,
                                                                  AudioEngine::AudioPluginId pluginId )
{
  RobotAudioBuffer* buffer = new RobotAudioBuffer();
  const auto it = _robotAudioBufferIdMap.emplace( pluginId, buffer );
  _gameObjectPluginIdMap.emplace( gameObjectId, pluginId );
  
  if ( !it.second ) {
    // If buffer already exists
    delete buffer;
    PRINT_NAMED_ERROR( "CozmoAudioController.RegisterRobotAudioBuffer",
                      "Robot buffer already exists! PluginId: %d GameObject: %u",
                      pluginId, static_cast<uint32_t>( gameObjectId ) );
  }
  
  return it.first->second;
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::UnregisterRobotAudioBuffer( AudioEngine::AudioGameObject gameObject,
                                                       AudioEngine::AudioPluginId pluginId)
{
  const auto it = _robotAudioBufferIdMap.find( pluginId );
  if (it != _robotAudioBufferIdMap.end()) {
    Util::SafeDelete(it->second);
    _robotAudioBufferIdMap.erase(it);
  } else {
    PRINT_NAMED_ERROR( "CozmoAudioController.UnregisterRobotAudioBuffer",
                      "Robot buffer doesn't exist! PluginId: %d GameObject: %u",
                      pluginId, static_cast<uint32_t>( gameObject ) );
  }
  
  const auto it2 = _gameObjectPluginIdMap.find( gameObject );
  if (it2 != _gameObjectPluginIdMap.end()) {
    _gameObjectPluginIdMap.erase(it2);
  } else {
    PRINT_NAMED_ERROR( "CozmoAudioController.UnregisterRobotAudioBuffer",
                      "Robot buffer doesn't exist! PluginId: %d GameObject: %u",
                      pluginId, static_cast<uint32_t>( gameObject ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* CozmoAudioController::GetRobotAudioBufferWithGameObject( AudioEngine::AudioGameObject gameObjectId ) const
{
  const auto it = _gameObjectPluginIdMap.find( gameObjectId );
  if ( it != _gameObjectPluginIdMap.end() ) {
    return GetRobotAudioBufferWithPluginId( it->second );
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBuffer* CozmoAudioController::GetRobotAudioBufferWithPluginId( AudioEngine::AudioPluginId pluginId ) const
{
  const auto it = _robotAudioBufferIdMap.find( pluginId );
  if ( it != _robotAudioBufferIdMap.end() ) {
    return it->second;
  }
  return nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::AppIsInFocus( const bool inFocus )
{
  PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.AppIsInFocus", "inFocus %c", inFocus ? 'Y' : 'N');
  // Post App State Event
  const GameEvent::Coz_App event = inFocus ? GameEvent::Coz_App::Enter_Foreground : GameEvent::Coz_App::Enter_Background;
  PostAudioEvent(static_cast<const AudioEventId>(event));
}

  
// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::SetupPlugins()
{
//#define USE_AUDIO_ENGINE 1
#if USE_AUDIO_ENGINE
  // Setup CozmoPlugIn & RobotAudioBuffer
  using namespace AudioEngine::PlugIns;
  InitializePluginInterface();
  GetPluginInterface()->SetupWavePortalPlugIn();
  GetPluginInterface()->SetupHijackAudioPlugInAndRobotAudioBuffers( static_cast<uint32_t>( AnimConstants::AUDIO_SAMPLE_RATE ),
                                                                    static_cast<uint16_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) );
  
  HijackAudioPlugIn* hijackAudioPlugIn = GetPluginInterface()->GetHijackAudioPlugIn();
  
  
  hijackAudioPlugIn = new HijackAudioPlugIn( static_cast<uint32_t>( AnimConstants::AUDIO_SAMPLE_RATE ),
                                             static_cast<uint16_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) );
  
  // Setup Callbacks
  hijackAudioPlugIn->SetCreatePlugInCallback( [this] ( const uint32_t plugInId )
                                              {
                                                PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.HijackAudioPlugin", "Create Plugin id: %d callback", plugInId);
                                                RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
                                                
                                                if ( buffer != nullptr ) {
                                                  buffer->PrepareAudioBuffer();
                                                }
                                                else {
                                                  // Buffer doesn't exist
                                                  PRINT_CH_INFO(CozmoAudioController::kLogChannelName,
                                                                "CozmoAudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.CreatePlugInCallback",
                                                                "PluginId: %u NotFound", plugInId);
                                                }
                                                
                                                
#if HijackAudioPlugInDebugLogs
                                                _plugInLog.emplace_back( TimeLog( LogEnumType::CreatePlugIn, "",
                                                                                 Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
                                              } );
  
  hijackAudioPlugIn->SetDestroyPluginCallback( [this] ( const uint32_t plugInId )
                                               {
                                                 PRINT_CH_INFO(kLogChannelName, "CozmoAudioController.HijackAudioPlugin", "Destroy Plugin id: %d callback", plugInId);
                                                 RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
                                                 
                                                 // Done with voice clear audio buffer
                                                 if ( buffer != nullptr ) {
                                                   buffer->CloseAudioBuffer();
                                                 }
                                                 else {
                                                   // Buffer doesn't exist
                                                   PRINT_CH_INFO(CozmoAudioController::kLogChannelName,
                                                                 "CozmoAudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.DestroyPluginCallback",
                                                                 "PluginId: %u NotFound", plugInId);
                                                 }
                                                 
                                                 
#if HijackAudioPlugInDebugLogs
                                                 _plugInLog.emplace_back( TimeLog( LogEnumType::DestroyPlugIn, "",
                                                                                  Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
                                                 PrintPlugInLog();
#endif
                                               } );
  
  hijackAudioPlugIn->SetProcessCallback( [this] ( const uint32_t plugInId, const AudioReal32* samples, const uint32_t sampleCount )
                                         {
                                           RobotAudioBuffer* buffer = GetRobotAudioBufferWithPluginId( plugInId );
                                           if ( buffer != nullptr ) {
                                             buffer->UpdateBuffer( samples, sampleCount );
                                           }
                                           else {
                                             // Buffer doesn't exist
                                             PRINT_CH_INFO(CozmoAudioController::kLogChannelName,
                                                           "CozmoAudioController.SetupHijackAudioPlugInAndRobotAudioBuffers.ProcessCallback",
                                                           "PluginId: %u NotFound", plugInId);
                                           }
                                           
#if HijackAudioPlugInDebugLogs
                                           _plugInLog.emplace_back( TimeLog( LogEnumType::Update, "SampleCount: " + std::to_string(sampleCount),
                                                                            Util::Time::UniversalTime::GetCurrentTimeInNanoseconds() ));
#endif
                                         } );

#endif // USE_AUDIO_ENGINE
}
  
  
#if USE_AUDIO_ENGINE
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
// Setup Ak Logging callback
void AudioEngineLogCallback( uint32_t akErrorCode,
                            const char* errorMessage,
                            AudioEngine::ErrorLevel errorLevel,
                            AudioEngine::AudioPlayingId playingId,
                            AudioEngine::AudioGameObject gameObjectId )
{
  std::ostringstream logStream;
  logStream << "ErrorCode: " << akErrorCode << " Message: '" << ((nullptr != errorMessage) ? errorMessage : "")
  << "' LevelBitFlag: " << (uint32_t)errorLevel << " PlayingId: " << playingId << " GameObjId: " << gameObjectId;
  
  if (((uint32_t)errorLevel & (uint32_t)ErrorLevel::Message) == (uint32_t)ErrorLevel::Message) {
    PRINT_CH_INFO(CozmoAudioController::kLogChannelName,
                  "CozmoAudioController.AudioEngineLog",
                  "%s", logStream.str().c_str());
  }
  
  if (((uint32_t)errorLevel & (uint32_t)ErrorLevel::Error) == (uint32_t)ErrorLevel::Error) {
    PRINT_NAMED_WARNING("CozmoAudioController.AudioEngineError", "%s", logStream.str().c_str());
  }
}
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::RegisterCladGameObjectsWithAudioController()
{
  // Enumerate through GameObjectType Enums
  for ( uint32_t aGameObj = static_cast<AudioGameObject>(GameObjectType::Default);
       aGameObj < static_cast<uint32_t>(GameObjectType::End);
       ++aGameObj) {
    // Register GameObjectType
    bool success = RegisterGameObject( static_cast<AudioGameObject>( aGameObj ),
                                       std::string(EnumToString(static_cast<GameObjectType>( aGameObj ))) );
    if (!success) {
      PRINT_NAMED_ERROR( "CozmoAudioController.RegisterCladGameObjectsWithAudioController",
                        "Registering GameObjectId: %ul - %s was unsuccessful",
                        aGameObj, EnumToString(static_cast<GameObjectType>(aGameObj)) );
    }
  }
}


// Debug Cozmo PlugIn Logs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if HijackAudioPlugInDebugLogs

double ConvertToMilliSec(unsigned long long int milliSeconds) {
  return (double)milliSeconds / 1000000.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::PrintPlugInLog() {
  
  unsigned long long int postTime = 0, createTime = 0, updateTime = 0;
  bool isFirstUpdateLog = false;
  for (auto& aLog : _plugInLog) {
    switch (aLog.LogType) {
      case LogEnumType::Post:
      {
        postTime = aLog.TimeInNanoSec;
        PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                            "----------------------------------------------\n \
                            Post Event %s - time: %f ms", aLog.Msg.c_str(), ConvertToMilliSec( aLog.TimeInNanoSec ));
      }
        break;
        
      case LogEnumType::CreatePlugIn:
      {
        createTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = true;
        
        PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                            "Create PlugIn %s - time: %f ms\n - Post -> Create time delta = %f ms\n",
                            aLog.Msg.c_str(), ConvertToMilliSec( aLog.TimeInNanoSec ),
                            ConvertToMilliSec( createTime - postTime ));
      }
        break;
        
      case LogEnumType::Update:
      {
        PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                            "Update %s - time: %f ms\n", aLog.Msg.c_str(), ConvertToMilliSec( aLog.TimeInNanoSec ));
        
        
        if ( isFirstUpdateLog ) {
          PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                              "- Post -> Update time delta = %f ms\n - Create -> Update time delta = %f ms\n",
                              ConvertToMilliSec( aLog.TimeInNanoSec - postTime ),
                              ConvertToMilliSec( aLog.TimeInNanoSec - createTime ));
        }
        else {
          PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                              "- Previous Update -> Update time delta = %f ms\n",
                              ConvertToMilliSec( aLog.TimeInNanoSec - updateTime ));
          
        }
        
        updateTime = aLog.TimeInNanoSec;
        isFirstUpdateLog = false;
      }
        break;
        
      case LogEnumType::DestroyPlugIn:
      {
        PRINT_NAMED_WARNING("CozmoAudioController.PrintPlugInLog",
                            "Destroy Plugin %s - time: %f ms\n ----------------------------------------------",
                            aLog.Msg.c_str(),
                            ConvertToMilliSec( aLog.TimeInNanoSec ));
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
