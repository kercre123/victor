/*
 * File: wavePortalPlugIn.cpp
 *
 * Author: Jordan Rivas
 * Created: 3/25/2016
 *
 * Description: This wraps the WavePortalFx plug-in instance to provide Audio Data source for playback. This is intended
 *              to be interacted with at app level.
 *
 * Note:        This Plug-in will assert if you don't set the Audio Data Stream before the plug-in is called by audio
 *              engine (Wwise)
 *
 * TODO:        At some point we want to update this so it can handle multiple instances of the PluginFX. They are
 *              tracked by their Id.
 *
 * Copyright: Anki, Inc. 2016
 */


#include <functional>
#include "audioEngine/plugins/wavePortalPlugIn.h"
#include "audioEngine/plugins/wavePortalFxTypes.h"
#include "AK/SoundEngine/Common/AkTypes.h"
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "audioEngine/audioDefines.h"
#include "wavePortalFxFactory.h"
#include "wavePortalFx.h"
#include <assert.h>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WavePortalPlugIn::WavePortalPlugIn()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WavePortalPlugIn::~WavePortalPlugIn()
{
  delete _audioDataStream;
  _audioDataStream = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WavePortalPlugIn::RegisterPlugIn()
{
  // Hijack Audio Streaming
    WavePortalFx::PostCreateFxFunc = [this](WavePortalFx* plugin) { SetupEnginePlugInFx(plugin); };
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WavePortalPlugIn::SetAudioDataOwnership(const AudioDataStream* audioDataStream)
{
  assert( nullptr == _audioDataStream );
  assert( nullptr != audioDataStream );
  
  _audioDataStream = audioDataStream;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WavePortalPlugIn::ClearAudioData()
{
  delete _audioDataStream;
  _audioDataStream = nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WavePortalPlugIn::SetupEnginePlugInFx( WavePortalFx* plugIn )
{
  // We should always set the source data before calling the event that creates the plug-in
  assert( _audioDataStream != nullptr );
  
  // Give audio data ownership to the running plugin
  plugIn->SetAudioDataStreamOwnership( _audioDataStream );
  // No longer own data stream
  _audioDataStream = nullptr;
  
  // Set Active flag while plug-in is in uses
  plugIn->SetInitCallback( [this] ( const WavePortalFx* pluginInstance )
  {
    _isActive = true;
    
    if ( _initFunc != nullptr ) {
      _initFunc(this);
    }
  });
  
  plugIn->SetTerminateCallback( [this] ( const WavePortalFx* pluginInstance )
  {
    _isActive = false;
    
    if ( _termFunc != nullptr ) {
      _termFunc(this);
    }
  });
}


} // PlugIns
} // AudioEngine
} // Anki
