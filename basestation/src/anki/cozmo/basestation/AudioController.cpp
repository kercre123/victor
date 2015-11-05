//
//  AudioController.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/3/15.
//
//

#include "anki/cozmo/basestation/AudioController.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

// Allow the build to include/exclude the audio libs
//#define EXCLUDE_ANKI_AUDIO_LIBS 0

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#include <DriveAudioEngine/audioEngineController.h>

#else

// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0

#endif


namespace Anki {
namespace Cozmo {
  
using namespace AudioEngine;

AudioController* AudioController::_singletonInstance = nullptr;

AudioController::AudioController() :
  _audioEngine( nullptr ),
  _isInitialized( false )
{
#if USE_AUDIO_ENGINE
  {
    _audioEngine = new AudioEngineController();
  }
#endif // USE_AUDIO_ENGINE
}


AudioController::~AudioController()
{
#if USE_AUDIO_ENGINE
  {
    Util::SafeDelete( _audioEngine );
  }
#endif
}


void AudioController::removeInstance()
{
  delete _singletonInstance;
  _singletonInstance = nullptr;
}

bool AudioController::Initialize(Util::Data::DataPlatform *dataPlatfrom)
{
  
  if ( !_isInitialized )
  {
#if USE_AUDIO_ENGINE
    {
      const std::string assetPath = dataPlatfrom->pathToResource(Util::Data::Scope::Resources, "assets/wwise/GeneratedSoundBanks/Mac/" );
      
      // Set Language Local
      const AudioLocaleType localeType = AudioLocaleType::EnglishUS;
      
      _audioEngine->SetLanguageLocale( localeType );
      
      _isInitialized = _audioEngine->Initialize( assetPath );
      
      // If we're using the audio engine, assert that it was successfully initialized.
      ASSERT_NAMED(_isInitialized, "AudioController.Initialize Audio Engine fail");
    }
#endif // USE_AUDIO_ENGINE
    
    // The audio engine was initialized correctly, so now let's setup everything else
    if ( _isInitialized )
    {
#if USE_AUDIO_ENGINE
      // FIXME: Temp fix to load audio banks
      AudioBankList bankList = {
        "Music.bnk",
        "UI.bnk",
        "VO.bnk",
      };
      const std::string sceneTitle = "InitScene";
      AudioScene initScene = AudioScene( sceneTitle, AudioEventList(), bankList );
      
      _audioEngine->RegisterAudioScene( std::move(initScene) );
      
      _audioEngine->LoadAudioScene( sceneTitle );
      
#endif
    }
  }
  
  return _isInitialized;
}
 

  
  
} // Cozmo
} // Anki
