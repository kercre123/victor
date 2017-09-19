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

#include "cozmoAnim/audio/cozmoAudioController.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "audioEngine/audioScene.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "cozmoAnim/cozmoAnimContext.h"
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
using Language = Anki::Util::Locale::Language;
//using ExternalLanguage = GameState::External_Language;

#if USE_AUDIO_ENGINE
// Resolve audio asset file path
static bool ResolvePathToAudioFile( const std::string&, const std::string&, std::string& );

// Setup Ak Logging callback
static void AudioEngineLogCallback( uint32_t, const char*, ErrorLevel, AudioPlayingId, AudioGameObject );
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::CozmoAudioController( const CozmoAnimContext* context )
{

#if USE_AUDIO_ENGINE
  {
    DEV_ASSERT(nullptr != context, "CozmoAudioController.CozmoAudioController.CozmoAnimContext.IsNull");

    const Util::Data::DataPlatform* dataPlatfrom = context->GetDataPlatform();
    const std::string assetPath = dataPlatfrom->pathToResource(Util::Data::Scope::Resources, "sound/" );
    PRINT_CH_INFO("Audio", "CozmoAudioController.CozmoAudioController", "AssetPath '%s'", assetPath.c_str());

    // If assets don't exist don't init the Audio engine
    const bool assetsExist = Util::FileUtils::DirectoryExists( assetPath );

    if ( !assetsExist ) {
      PRINT_NAMED_ERROR("CozmoAudioController.CozmoAudioController", "Audio Assets do NOT exist - Ignore if Unit Test");
      return;
    }


    // Config Engine
    SetupConfig config{};
    // Assets
    config.assetFilePath = assetPath;
    // Path resolver function
    config.pathResolver = std::bind(&ResolvePathToAudioFile, assetPath,
                                    std::placeholders::_1, std::placeholders::_2);

    // Add Assets Zips to list.
    // Note: We only have 1 file at the moment this will change when we brake up assets for RAMS
    std::string zipAssets = assetPath + "AudioAssets.zip";
    if (Util::FileUtils::FileExists(zipAssets)) {
      config.pathToZipFiles.push_back(std::move(zipAssets));
    }
    else {
      PRINT_NAMED_ERROR("CozmoAudioController.CozmoAudioController", "Audio Assets not found: '%s'", zipAssets.c_str());
    }

    
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

    // FIXME: Temp fix to load audio banks
    AudioBankList bankList = {
      "Init.bnk",
      //      "Music.bnk",
      //      "UI.bnk",
      "SFX.bnk",
      "Cozmo.bnk",
      "Dev_Debug.bnk",
    };
    const std::string sceneTitle = "InitScene";
    AudioScene initScene = AudioScene( sceneTitle, AudioEventList(), bankList );

    RegisterAudioScene( std::move(initScene) );

    LoadAudioScene( sceneTitle );

    RegisterCladGameObjectsWithAudioController();

#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CozmoAudioController::~CozmoAudioController()
{
}

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
      PRINT_NAMED_ERROR( "CozmoAudioController.RegisterCladGameObjectsWithAudioController",
                        "Registering GameObjectId: %ul - %s was unsuccessful",
                        aGameObj, EnumToString(static_cast<GameObjectType>(aGameObj)) );
    }
  }
}


#if USE_AUDIO_ENGINE
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: Android will be the primary platform, need to optimize path resolver
bool ResolvePathToAudioFile( const std::string& dataPlatformResourcePath,
                            const std::string& in_name,
                            std::string& out_path )
{
  out_path.clear();
  if (dataPlatformResourcePath.empty()) {
    return false;
  }

  out_path = dataPlatformResourcePath + in_name;
  if ( out_path.empty() ) {
    return false;
  }
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


} // Audio
} // Cozmo
} // Anki
