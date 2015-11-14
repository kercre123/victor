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
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include <util/dispatchQueue/dispatchQueue.h>
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>

// Allow the build to include/exclude the audio libs
//#define EXCLUDE_ANKI_AUDIO_LIBS 0

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#include <DriveAudioEngine/audioEngineController.h>
#include <DriveAudioEngine/PlugIns/cozmoPlugIn.h>
#else

// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0

#endif


namespace Anki {
namespace Cozmo {
namespace Audio {
  
using namespace AudioEngine;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::AudioController( Util::Data::DataPlatform* dataPlatfrom )
{
#if USE_AUDIO_ENGINE
  {
    _audioEngine = new AudioEngineController();
    
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
    ASSERT_NAMED( !_taskHandle, "AudioController.Initialize Invalid Task Handle" );
#if USE_AUDIO_ENGINE
    
    // Setup CozmoPlugIn & RobotAudioBuffer
    _cozmoPlugIn = new CozmoPlugIn();
    _robotAudioBuffer = new RobotAudioBuffer();
    
    // Setup Callbacks
    _cozmoPlugIn->SetCreatePlugInCallback( [this] () {
      PRINT_NAMED_INFO( "AudioController.Initialize", "Create PlugIn Callback!" );
    } );
    // Not yet working
    _cozmoPlugIn->SetDestroyPluginCallback( [this] () {
      PRINT_NAMED_INFO( "AudioController.Initialize", "Create Destroy Callback!" );
    } );
    
    _cozmoPlugIn->SetProcessCallback( [this] (AudioEngine::CozmoPlugIn::CozmoAudioBuffer& buffer )
    {
      if ( nullptr != _robotAudioBuffer ) {
       _robotAudioBuffer->UpdateBuffer( buffer.frames, buffer.frameCount );
      }
    });
    
    const bool success = _cozmoPlugIn->RegisterPlugin();
    if ( ! success ) {
      PRINT_NAMED_ERROR( "AudioController.Initialize", "Fail to Regist Cozmo PlugIn");
    }
    
    // FIXME: Temp fix to load audio banks
    AudioBankList bankList = {
      "Init.bnk",
      "Music.bnk",
      "UI.bnk",
      "VO.bnk",
      "Cozmo_Movement.bnk",
    };
    const std::string sceneTitle = "InitScene";
    AudioScene initScene = AudioScene( sceneTitle, AudioEventList(), bankList );
    
    _audioEngine->RegisterAudioScene( std::move(initScene) );
    
    _audioEngine->LoadAudioScene( sceneTitle );

#endif
  
    // Setup our update method to be called periodically
    _dispatchQueue = Util::Dispatch::Create();
    const std::chrono::milliseconds sleepDuration = std::chrono::milliseconds(10);
    _taskHandle = Util::Dispatch::ScheduleCallback( _dispatchQueue, sleepDuration, std::bind( &AudioController::Update, this ) );
    
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioController::~AudioController()
{
  if ( _taskHandle && _taskHandle->IsValid() )
  {
    _taskHandle->Invalidate();
  }
  Util::Dispatch::Release( _dispatchQueue );
  
#if USE_AUDIO_ENGINE
  {
    Util::SafeDelete( _audioEngine );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioPlayingID AudioController::PostAudioEvent( const std::string& eventName,
                                                             AudioEngine::AudioGameObject gameObjectId) const
{
  AudioPlayingID playingId = kInvalidAudioPlayingID;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    playingId = _audioEngine->PostEvent( eventName, gameObjectId );
  }
#endif
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::AudioPlayingID AudioController::PostAudioEvent( AudioEngine::AudioEventID eventId,
                                                             AudioEngine::AudioGameObject gameObjectId ) const
{
  AudioPlayingID playingId = kInvalidAudioPlayingID;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    playingId = _audioEngine->PostEvent( eventId, gameObjectId );
  }
#endif
  return playingId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioController::SetState( AudioEngine::AudioStateGroupId stateGroupId,
                                AudioEngine::AudioStateId stateId ) const
{
  bool success = false;
#if USE_AUDIO_ENGINE
  if ( _isInitialized ) {
    _audioEngine->SetState( stateGroupId, stateId );
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
    _audioEngine->SetSwitch( switchGroupId, switchStateId, gameObject );
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
    _audioEngine->SetRTPCValue( parameterId, rtpcValue, gameObject, valueChangeDuration, curve );
  }
#endif
  return success;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private
  
void AudioController::Update()
{
#if USE_AUDIO_ENGINE
  // NOTE: Don't need time delta
  _audioEngine->Update( 0.0 );
#endif
}
  
} // Audio
} // Cozmo
} // Anki
