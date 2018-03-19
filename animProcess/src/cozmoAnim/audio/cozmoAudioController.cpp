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

#include "audioEngine/audioTypeTranslator.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/animContext.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "audioEngine/audioScene.h"
#include "audioEngine/soundbankLoader.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"
#include <sstream>


// Allow the build to include/exclude the audio libs
//#define EXCLUDE_ANKI_AUDIO_LIBS 0

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioEngine/plugins/hijackAudioPlugIn.h"
#include "audioEngine/plugins/wavePortalPlugIn.h"
#else
// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0
#endif

namespace {
static Anki::Cozmo::Audio::CozmoAudioController* sThis = nullptr;
const std::string kProfilerCaptureFileName    = "VictorProfilerSession.prof";
const std::string kAudioOutputCaptureFileName = "VictorOutputSession.wav";
}

namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioEngine;
using Language = Anki::Util::Locale::Language;


#if USE_AUDIO_ENGINE
// Setup Ak Logging callback
static void AudioEngineLogCallback( uint32_t, const char*, ErrorLevel, AudioPlayingId, AudioGameObject );
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Console Vars
CONSOLE_VAR( bool, kWriteAudioProfilerCapture, "CozmoAudioController", false );
CONSOLE_VAR( bool, kWriteAudioOutputCapture, "CozmoAudioController", false );

// Console Functions
// Session Logs
void SetWriteAudioProfilerCapture( ConsoleFunctionContextRef context )
{
  kWriteAudioProfilerCapture = ConsoleArg_Get_Bool( context, "writeProfiler" );
  if ( sThis != nullptr ) {
    sThis->WriteProfilerCapture( kWriteAudioProfilerCapture );
  }
}

void SetWriteAudioOutputCapture( ConsoleFunctionContextRef context )
{
  kWriteAudioOutputCapture = ConsoleArg_Get_Bool( context, "writeOutput" );
  if ( sThis != nullptr ) {
    sThis->WriteAudioOutputCapture( kWriteAudioOutputCapture );
  }
}

// Robot helpers
void SetRobotMasterVolume( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const float vol = ConsoleArg_Get_Float( context, "robotMasterVolume");
    sThis->SetRobotMasterVolume( vol );
  }
}

void SetProceduralAudioVolume( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const float vol = ConsoleArg_Get_Float( context, "proceduralAudioVolume");
    sThis->SetProceduralAudioVolume( vol );
  }
}

// Generic Audio Interface
void PostAudioEvent( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const char* event = ConsoleArg_Get_String( context, "event" );
    const uint64_t gameObjectId = ConsoleArg_GetOptional_UInt64( context,
                                                                 "gameObjectId",
                                                                 static_cast<uint64_t>(AudioMetaData::GameObjectType::Default) );
    sThis->PostAudioEvent( event, gameObjectId );
  }
}
  
void SetAudioState( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const char* stateGroup = ConsoleArg_Get_String( context, "stateGroup" );
    const char* state = ConsoleArg_Get_String( context, "state" );
    sThis->SetState( AudioEngineController::GetAudioIdFromString( stateGroup ),
                     AudioEngineController::GetAudioIdFromString( state ) );
  }
}

void SetAudioSwitchState( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const char* switchGroup = ConsoleArg_Get_String( context, "switchGroup" );
    const char* state = ConsoleArg_Get_String( context, "state" );
    const uint64_t gameObjectId = ConsoleArg_Get_UInt64( context, "gameObjectId" );
    sThis->SetSwitchState( AudioEngineController::GetAudioIdFromString( switchGroup ),
                           AudioEngineController::GetAudioIdFromString( state ),
                           gameObjectId );
  }
}

void SetAudioParameter( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    const char* parameter = ConsoleArg_Get_String( context, "parameter" );
    const float value = ConsoleArg_Get_Float( context, "value" );
    uint64_t gameObjectId = ConsoleArg_GetOptional_UInt64( context, "gameObjectId", kInvalidAudioGameObject );
    sThis->SetParameter( AudioEngineController::GetAudioIdFromString( parameter ), value, gameObjectId );
  }
}

void StopAllAudioEvents( ConsoleFunctionContextRef context )
{
  if ( sThis != nullptr ) {
    uint64_t gameObjectId = ConsoleArg_GetOptional_UInt64( context, "gameObjectId", kInvalidAudioGameObject );
    sThis->StopAllAudioEvents( gameObjectId );
  }
}

// Register console var func
const char* consolePath = "CozmoAudioController";
CONSOLE_FUNC( SetWriteAudioProfilerCapture, consolePath, bool writeProfiler );
CONSOLE_FUNC( SetWriteAudioOutputCapture, consolePath, bool writeOutput );
CONSOLE_FUNC( SetRobotMasterVolume, consolePath, float robotMasterVolume );
CONSOLE_FUNC( SetProceduralAudioVolume, consolePath, float proceduralAudioVolume );
CONSOLE_FUNC( PostAudioEvent, consolePath, const char* event, optional uint64 gameObjectId );
CONSOLE_FUNC( SetAudioState, consolePath, const char* stateGroup, const char* state );
CONSOLE_FUNC( SetAudioSwitchState, consolePath, const char* switchGroup, const char* state, uint64 gameObjectId );
CONSOLE_FUNC( SetAudioParameter, consolePath, const char* parameter, float value, optional uint64 gameObjectId );
CONSOLE_FUNC( StopAllAudioEvents, consolePath, optional uint64 gameObjectId );


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CozmoAudioController
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::CozmoAudioController( const AnimContext* context )
{
#if USE_AUDIO_ENGINE
  {
    DEV_ASSERT(nullptr != context, "CozmoAudioController.CozmoAudioController.AnimContext.IsNull");

    const Util::Data::DataPlatform* dataPlatform = context->GetDataPlatform();
    const std::string assetPath = dataPlatform->pathToResource(Util::Data::Scope::Resources, "sound" );
    const std::string writePath = dataPlatform->pathToResource(Util::Data::Scope::Cache, "sound");
    PRINT_CH_INFO("Audio", "CozmoAudioController.CozmoAudioController", "AssetPath '%s'", assetPath.c_str());
    PRINT_CH_INFO("Audio", "CozmoAudioController.CozmoAudioController", "WritePath '%s'", writePath.c_str());
    // If assets don't exist don't init the Audio engine
    const bool assetsExist = Util::FileUtils::DirectoryExists( assetPath );
    if ( !assetsExist ) {
      PRINT_NAMED_ERROR("CozmoAudioController.CozmoAudioController", "Audio Assets do NOT exist - Ignore if Unit Test");
      return;
    }
    // Create sound bank loader
    _soundbankLoader.reset(new SoundbankLoader(*this, assetPath));

    // Config Engine
    SetupConfig config{};
    // Read/Write Asset path
    config.assetFilePath = assetPath;
    config.writeFilePath = writePath;

    // Cozmo uses default audio locale regardless of current context.
    // Locale-specific adjustments are made by setting GameState::External_Language
    // below.
    config.audioLocale = AudioLocaleType::EnglishUS;

    // Engine Memory
    config.defaultMemoryPoolSize      = ( 2 * 1024 * 1024 );      // 2 MB
    config.defaultLEMemoryPoolSize    = ( 1024 * 1024 );          // 1 MB
    config.ioMemorySize               = ( (1024 + 512) * 1024 );  // 1.5 MB
    config.defaultMaxNumPools         = 30;
    config.enableGameSyncPreparation  = true;
    config.enableStreamCache          = true;

    // Start your Engines!!!
    InitializeAudioEngine( config );

    // If we're using the audio engine, assert that it was successfully initialized.
    DEV_ASSERT(IsInitialized(), "CozmoAudioController.Initialize Audio Engine fail");
  }

  // The audio engine was initialized correctly, so now let's setup everything else
  if ( IsInitialized() )
  {
    // Setup Engine Logging callback
    SetLogOutput( ErrorLevel::All, &AudioEngineLogCallback );

    InitializePluginInterface();
    GetPluginInterface()->SetupWavePortalPlugIn();

    // Load audio sound bank metadata
    // NOTE: This will slightly change when we implement RAMS
    if (_soundbankLoader.get() != nullptr) {
      _soundbankLoader->LoadDefaultSoundbanks();
    }

    // Use Console vars to control profiling settings
    if ( kWriteAudioProfilerCapture ) {
      WriteProfilerCapture( true );
    }
    if ( kWriteAudioOutputCapture ) {
      WriteAudioOutputCapture( true );
    }
    
    RegisterCladGameObjectsWithAudioController();
    SetDefaultListeners( { ToAudioGameObject( AudioMetaData::GameObjectType::Victor_Listener ) } );
  }
  if (sThis == nullptr) {
    sThis = this;
  }
  else {
    PRINT_NAMED_ERROR("CozmoAudioController", "sThis.NotNull");
  }
#else
  PRINT_NAMED_WARNING("CozmoAudioController", "Audio Engine is disabled - EXCLUDE_ANKI_AUDIO_LIBS flag is set");
#endif // USE_AUDIO_ENGINE
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::~CozmoAudioController()
{
  if (sThis == this) {
    sThis = nullptr;
  }
  else {
    PRINT_NAMED_ERROR("~CozmoAudioController", "sThis.NotEqualToInstance");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CozmoAudioController::WriteProfilerCapture( bool write )
{
  return AudioEngineController::WriteProfilerCapture( write, kProfilerCaptureFileName );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CozmoAudioController::WriteAudioOutputCapture( bool write )
{
  return AudioEngineController::WriteAudioOutputCapture( write, kAudioOutputCaptureFileName );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::SetRobotMasterVolume( AudioEngine::AudioRTPCValue volume,
                                                 AudioEngine::AudioTimeMs timeInMilliSeconds,
                                                 AudioEngine::AudioCurveType curve )
{
  AudioEngine::AudioRTPCValue orgVol = volume;
  volume = Util::Clamp( volume, 0.0f, 1.0f );
  if ( !Util::IsFltNear( volume, orgVol ) ) {
    PRINT_NAMED_WARNING( "CozmoAudioController.SetRobotMasterVolume",
                         "Invalid volume %f - Acceptable volume values are [0.0, 1.0] - Value was Clamped to %f",
                         orgVol, volume );
  }
  SetParameter( ToAudioParameterId( AudioMetaData::GameParameter::ParameterType::Robot_Volume ),
                volume,
                kInvalidAudioGameObject,
                timeInMilliSeconds,
                curve );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::SetProceduralAudioVolume( AudioEngine::AudioRTPCValue volume,
                                                     AudioEngine::AudioTimeMs timeInMilliSeconds,
                                                     AudioEngine::AudioCurveType curve )
{
  AudioEngine::AudioRTPCValue orgVol = volume;
  volume = Util::Clamp( volume, 0.0f, 1.0f );
  if ( !Util::IsFltNear( volume, orgVol ) ) {
    PRINT_NAMED_WARNING( "CozmoAudioController.SetProceduralAudioVolume",
                         "Invalid volume %f - Acceptable volume values are [0.0, 1.0] - Value was Clamped to %f",
                         orgVol, volume );
  }
  SetParameter( ToAudioParameterId( AudioMetaData::GameParameter::ParameterType::Procedural_Audio_Volume ),
                volume,
                kInvalidAudioGameObject,
                timeInMilliSeconds,
                curve );
}

// Private Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CozmoAudioController::RegisterCladGameObjectsWithAudioController()
{
  using GameObjectType = AudioMetaData::GameObjectType;

  // Enumerate through GameObjectType Enums
  for ( uint32_t aGameObj = static_cast<AudioGameObject>(GameObjectType::Default);
       aGameObj < static_cast<uint32_t>(GameObjectType::End);
       ++aGameObj) {
    // Register GameObjectType
    bool success = RegisterGameObject( static_cast<AudioGameObject>( aGameObj ),
                                      std::string(EnumToString(static_cast<GameObjectType>( aGameObj ))) );
    if (!success) {
      PRINT_NAMED_ERROR("CozmoAudioController.RegisterCladGameObjectsWithAudioController",
                        "Registering GameObjectId: %ul - %s was unsuccessful",
                        aGameObj, EnumToString(static_cast<GameObjectType>(aGameObj)));
    }
  }
}


#if USE_AUDIO_ENGINE
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


} // Audio
} // Cozmo
} // Anki
