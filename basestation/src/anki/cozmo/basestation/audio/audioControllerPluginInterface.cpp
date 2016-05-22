/*
 * File: audioControllerPluginInterface.cpp
 *
 * Author: Jordan Rivas
 * Created: 4/05/2016
 *
 * Description: This class wraps Audio Engine custom plugins to allow the app’s objects to interface with them. The
 *              interface is intended to remove the complexities of compile issues since the Audio Engine can be
 *              complied out of the project.
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "anki/cozmo/basestation/audio/audioController.h"

#ifndef EXCLUDE_ANKI_AUDIO_LIBS

#define USE_AUDIO_ENGINE 1
#include "DriveAudioEngine/PlugIns/hijackAudioPlugIn.h"
#include "DriveAudioEngine/PlugIns/wavePortalPlugIn.h"
#else

// If we're excluding the audio libs, don't link any of the audio engine
#define USE_AUDIO_ENGINE 0
#endif


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioControllerPluginInterface::AudioControllerPluginInterface( AudioController& parentAudioController )
: _parentAudioController( parentAudioController )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioControllerPluginInterface::SetWavePortalAudioDataInfo( uint32_t sampleRate,
                                                                 uint16_t numberOfChannels,
                                                                 float duration_ms,
                                                                 float* audioBuffer,
                                                                 uint32_t bufferSize )
{
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    _parentAudioController._wavePortalPlugIn->SetAudioDataInfo( sampleRate,
                                                                numberOfChannels,
                                                                duration_ms,
                                                                audioBuffer,
                                                                bufferSize );
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioControllerPluginInterface::ClearWavePortalAudioDataInfo()
{
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    _parentAudioController._wavePortalPlugIn->ClearAudioDataInfo();
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioControllerPluginInterface::WavePortalHasAudioDataInfo() const
{
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    return _parentAudioController._wavePortalPlugIn->HasAudioDataInfo();
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioControllerPluginInterface::WavePortalIsActive() const
{
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    return _parentAudioController._wavePortalPlugIn->IsActive();
  }
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioControllerPluginInterface::SetWavePortalInitCallback( PluginCallbackFunc callback )
{
  _wavePortalInitFunc = callback;
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    // Wrap the Plug-in interface callback inside of the plugin instance's callback
    if ( _wavePortalInitFunc != nullptr ) {
      using namespace AudioEngine::PlugIns;
      _parentAudioController._wavePortalPlugIn->SetInitCallback( [this] (const WavePortalPlugIn* pluginInstance)
      {
        if ( _wavePortalInitFunc != nullptr ) {
          _wavePortalInitFunc( this );
        }
      });
    }
    else {
      _parentAudioController._wavePortalPlugIn->SetInitCallback( nullptr );
    }
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioControllerPluginInterface::SetWavePortalTerminateCallback( PluginCallbackFunc callback )
{
  _wavePortalTermFunc = callback;
#if USE_AUDIO_ENGINE
  if ( _parentAudioController._wavePortalPlugIn != nullptr ) {
    // Wrap the Plug-in interface callback inside of the plugin instance's callback
    if ( _wavePortalTermFunc != nullptr ) {
      using namespace AudioEngine::PlugIns;
      _parentAudioController._wavePortalPlugIn->SetTerminateCallback( [this] (const WavePortalPlugIn* pluginInstance)
      {
        if ( _wavePortalTermFunc != nullptr ) {
          _wavePortalTermFunc( this );
        }
      });
    }
    else {
      _parentAudioController._wavePortalPlugIn->SetTerminateCallback( nullptr );
    }
  }
#endif
}

} // Audio
} // Cozmo
} // Anki
