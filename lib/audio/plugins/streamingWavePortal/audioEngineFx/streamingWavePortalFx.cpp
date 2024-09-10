/*
 * File: streamingWavePortalFx.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/17/2018
 *
 * Description: This is the main class for StreamingWavePortal Plug-in. It acts as the primary interface to set an
 *              StreamingWaveDataInstance as the playback source when this plug-in is used by Wwise audio engine.
 *
 * Note: This currently only supports single channel data
 *
 * Copyright: Anki, Inc. 2018
 */


#include "streamingWavePortalFx.h"
#include "streamingWavePortalId.h"
#include <AK/AkWwiseSDKVersion.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkAssert.h>

// Plugin mechanism. Instanciation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateStreamingWavePortal( AK::IAkPluginMemAlloc * in_pAllocator )
{
  using namespace Anki::AudioEngine::PlugIns;
  AK::IAkPlugin* plugin = AK_PLUGIN_NEW( in_pAllocator, StreamingWavePortalFx() );
  if (StreamingWavePortalFx::PostCreateFxFunc != nullptr) {
    StreamingWavePortalFx::PostCreateFxFunc( static_cast<StreamingWavePortalFx*>(plugin) );
  }
  return plugin;
}

// Plugin mechanism. Parameter node create function to be registered to the FX manager.
AK::IAkPluginParam * CreateStreamingWavePortalParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
  using namespace Anki::AudioEngine::PlugIns;
  return AK_PLUGIN_NEW( in_pAllocator, StreamingWavePortalParams() );
}


AK_IMPLEMENT_PLUGIN_FACTORY(StreamingWavePortal, AkPluginTypeSource, AKCOMPANYID_ANKI, AKSOURCEID_STREAMINGWAVEPORTAL)


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// Static function for post CreateWavePortalFx() work
std::function<void(StreamingWavePortalFx*)> StreamingWavePortalFx::PostCreateFxFunc = nullptr;

// Constructor.
StreamingWavePortalFx::StreamingWavePortalFx()
: m_pParams(NULL)
{
}

// Destructor.
StreamingWavePortalFx::~StreamingWavePortalFx()
{
}

// Initializes the effect.
AKRESULT StreamingWavePortalFx::Init( AK::IAkPluginMemAlloc*      in_pAllocator,
                                      AK::IAkSourcePluginContext* in_pSourceFXContext,
                                      AK::IAkPluginParam*         in_pParams,
                                      AkAudioFormat&              io_rFormat )
{
  // Setup Params
  m_pParams = ( StreamingWavePortalParams* )in_pParams;
  if ( m_pParams == nullptr ) {
    return AK_Fail;
  }
  m_pParams->m_ParamChangeHandler.ResetAllParamChanges(); // All parameter changes have been applied
  
  // Setup audio data
  if ( _initFunc != nullptr ) {
    _initFunc( this );
  }
  
  if ( !_data || !_data->HasAudioData() ) {
    return AK_Fail;
  }
  
  // Setup plugin
  io_rFormat.channelConfig.SetStandard( AK_SPEAKER_SETUP_MONO );
  io_rFormat.uSampleRate = _data->GetSampleRate();
  
  AK_PERF_RECORDING_RESET();
  
  return AK_Success;
}

// The effect must destroy itself herein.
AKRESULT StreamingWavePortalFx::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
  if ( _termFunc != nullptr ) {
    _termFunc( this );
  }
  
  _data.reset(); // Release data
  AK_PLUGIN_DELETE( in_pAllocator, this );
  return AK_Success;
}

// Reinitialize processing state.
AKRESULT StreamingWavePortalFx::Reset()
{
  return AK_Success;
}

// Effect info query.
AKRESULT StreamingWavePortalFx::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
  out_rPluginInfo.eType = AkPluginTypeSource;
  out_rPluginInfo.bIsInPlace = true;
  out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
  return AK_Success;
}

//Effect processing.
void StreamingWavePortalFx::Execute( AkAudioBuffer* io_pBufferOut )
{
  AK_PERF_RECORDING_START( "StreamingWavePortal", 25, 30 );
  _data->WriteToPluginBuffer( io_pBufferOut );
  AK_PERF_RECORDING_STOP( "StreamingWavePortal", 25, 30 );
}

// Get the duration of the source in mSec.
AkReal32 StreamingWavePortalFx::GetDuration() const
{
  // Unknown duration
  return 0.0f;
}

uint8_t StreamingWavePortalFx::GetPluginId() const
{
  AKASSERT( m_pParams != nullptr );
  return m_pParams->instanceId;
}


} // Plugins
} // AudioEngine
} // Anki
