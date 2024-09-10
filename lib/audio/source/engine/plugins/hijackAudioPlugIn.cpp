/*
 * File: HijackAudioPlugIn.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: This wraps and holds the HijackFx plug-in instance to provide callbacks for Create, Destroy & Process
 *              events. This is intended to be interacted with at app level.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "audioEngine/plugins/hijackAudioPlugIn.h"
#include "AK/SoundEngine/Common/AkTypes.h"
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "audioEngine/audioDefines.h"
#include "hijackFxFactory.h"
#include "hijackFx.h"
#include <assert.h>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HijackAudioPlugIn::HijackAudioPlugIn( uint32_t resampleRate, uint16_t samplesPerFrame ) :
  _resampleRate( resampleRate ),
  _samplesPerFrame( samplesPerFrame )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HijackAudioPlugIn::~HijackAudioPlugIn()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HijackAudioPlugIn::RegisterPlugin()
{
  // Hijack Audio Streaming
  HijackFx::PostCreateFxFunc = [this](HijackFx* plugin) { SetupEnginePlugInFx(plugin); };

  return true;
}
  
bool HijackAudioPlugIn::SetCreatePlugInCallback( PlugInEventFunc callback ) {
  std::lock_guard<std::mutex> lock(_setCallbackMutex);
  _createCallback = callback;
  return RegisterPlugin();
}

bool HijackAudioPlugIn::SetDestroyPluginCallback( PlugInEventFunc callback ) {
  std::lock_guard<std::mutex> lock(_setCallbackMutex);
  _destroyCallback = callback;
  return RegisterPlugin();
}

bool HijackAudioPlugIn::SetProcessCallback( AudioBufferFunc callback ) {
  std::lock_guard<std::mutex> lock(_setCallbackMutex);
  _processCallback = callback;
  return RegisterPlugin();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackAudioPlugIn::SetupEnginePlugInFx( HijackFx* plugIn )
{
  // Must set _resampleRate & _samplesPerFrame before creating plug-in
  assert( _resampleRate != 0 && _samplesPerFrame != 0 );
  plugIn->SetDspSettings( _resampleRate, _samplesPerFrame );
  
  plugIn->SetDidInitCallback( [this] ( const HijackFx* pluginInstance )
  {
    if ( nullptr != _createCallback ) {
      _createCallback( pluginInstance->GetParamsPlugInId() );
    }
  });
  
  plugIn->SetAudioBufferCallback( [this] ( const HijackFx* pluginInstance, const AudioReal32* samples, const uint32_t sampleCount )
  {
    if ( nullptr != _processCallback ) {
      _processCallback( pluginInstance->GetParamsPlugInId(), samples, sampleCount );
    }
  } );
  
  plugIn->SetTerminateCallback( [this] ( const HijackFx* pluginInstance )
  {
    if (nullptr != _destroyCallback ) {
      _destroyCallback( pluginInstance->GetParamsPlugInId() );
    }
  });
}

} // PlugIns
} // AudioEngine
} // Anki
