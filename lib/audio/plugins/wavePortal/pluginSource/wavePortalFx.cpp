/*
 * File: wavePortalFx.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: This is the main class for WavePortal Plug-in. It acts as the primary interface to set an AudioDataProxy
 *              struct as the playback source when this plug-in is used by Wwise audio engine.
 *
 * Copyright: Anki, Inc. 2015
 */


#include "wavePortalFx.h"
#include "wavePortalPluginId.h"
#include <algorithm>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
// Static Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Plugin mechanism. Instanciation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateWavePortalFx( AK::IAkPluginMemAlloc * in_pAllocator )
{
  AK::IAkPlugin* plugin = AK_PLUGIN_NEW( in_pAllocator, WavePortalFx() );
  if (WavePortalFx::PostCreateFxFunc != nullptr) {
    WavePortalFx::PostCreateFxFunc(static_cast<WavePortalFx*>(plugin));
  }
  return plugin;
}

// Plugin mechanism. Instantiation method that must be registered to the plug-in manager.
AK::IAkPluginParam* CreateWavePortalFxParams(AK::IAkPluginMemAlloc * in_pAllocator)
{
	return AK_PLUGIN_NEW(in_pAllocator, WavePortalFxParams());
}

//Static initializer object to register automatically the plugin into the sound engine
AK::PluginRegistration WavePortalFxRegistration(AkPluginTypeSource, AKCOMPANYID_ANKI, AKSOURCEID_WAVEPORTAL, CreateWavePortalFx, CreateWavePortalFxParams);

// Static function for post CreateWavePortalFx() work
std::function<void(WavePortalFx*)> WavePortalFx::PostCreateFxFunc = nullptr;

  
// Instance Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFx::Init(AK::IAkPluginMemAlloc* in_pAllocator,
                            AK::IAkSourcePluginContext*	in_pEffectPluginContext,
                            AK::IAkPluginParam* in_pParams,
                            AkAudioFormat& io_rFormat )
{
  // Setup Params
  _params = dynamic_cast<WavePortalFxParams*>(in_pParams);
  if ( _params == nullptr ) {
    return AK_Fail;
  }
  _sourceIdx = 0;
  
  // Setup data
  if ( _initFunc != nullptr ) {
    _initFunc( this );
  }
  if (_audioDataStream == nullptr ) {
    return AK_Fail;
  }
  
  // Set formate to source's properties
  io_rFormat.uSampleRate = _audioDataStream->sampleRate;

  // Profile plugin
  AK_PERF_RECORDING_RESET();
  
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFx::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
  if ( _termFunc != nullptr ) {
    _termFunc( this );
  }

  delete _audioDataStream;
  _audioDataStream = nullptr;
  AK_PLUGIN_DELETE( in_pAllocator, this );
  
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFx::Reset()
{
  _sourceIdx = 0;
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFx::GetPluginInfo(AkPluginInfo & out_rPluginInfo)
{
  out_rPluginInfo.eType = AkPluginTypeSource;
  out_rPluginInfo.bIsInPlace = true;
  out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;

  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WavePortalFx::Execute(AkAudioBuffer* io_pBufferOut)
{
  if ( _audioDataStream == nullptr ) {
    io_pBufferOut->eState = AK_NoMoreData;
    return;
  }
  
  // Profile plugin
  AK_PERF_RECORDING_START( "WavePortal", 25, 30 );
  
  // Copy Data to Wwise buffer
  uint32_t framesRemaining = _audioDataStream->bufferSize - _sourceIdx;
  // Set Min Frame Count
  uint32_t frameCount = io_pBufferOut->MaxFrames() > framesRemaining ? framesRemaining : io_pBufferOut->MaxFrames();
  io_pBufferOut->uValidFrames = frameCount;
  
  AkSampleType* pfBufOut = io_pBufferOut->GetChannel(0);
  AkSampleType* pfBufEnd = pfBufOut + io_pBufferOut->uValidFrames;
  
  while (pfBufOut != pfBufEnd) {
    *pfBufOut++ = _audioDataStream->audioBuffer[ _sourceIdx++ ];
  }
  
  io_pBufferOut->eState = AK_DataReady;
  
  // Catch the last frame
  if (io_pBufferOut->uValidFrames < io_pBufferOut->MaxFrames()) {
    // Pad end of buffer with zeros
    io_pBufferOut->ZeroPadToMaxFrames();
    io_pBufferOut->eState = AK_NoMoreData;
  }
  
  AK_PERF_RECORDING_STOP( "WavePortal", 25, 30 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AkReal32 WavePortalFx::GetDuration() const
{
  if ( _audioDataStream != nullptr ) {
    return _audioDataStream->duration_ms;
  }
  
  return 0.0f;
}


} // PlugIns
} // AudioEngine
} // Anki
