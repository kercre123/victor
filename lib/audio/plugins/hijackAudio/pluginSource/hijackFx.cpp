/*
 * File: HijackFx.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: This is the main class for Hijack Plug-in. It acts as the primary interface and owns HijackFxDsp &
 *              HijackFxParam classes. It has callbacks methods which allows us to get the audio byte stream from the
 *              plug-in.
 *
 * Copyright: Anki, Inc. 2015
 */


#include "hijackFx.h"
#include "hijackAudioPluginId.h"
#include <AK/AkWwiseSDKVersion.h>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// Static Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Plugin mechanism. Instanciation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateHijackFx( AK::IAkPluginMemAlloc * in_pAllocator )
{
  AK::IAkPlugin* plugin = AK_PLUGIN_NEW( in_pAllocator, HijackFx() );
  if (HijackFx::PostCreateFxFunc != nullptr) {
    HijackFx::PostCreateFxFunc(static_cast<HijackFx*>(plugin));
  }
  return plugin;
}

// Plugin mechanism. Instantiation method that must be registered to the plug-in manager.
AK::IAkPluginParam* CreateHijackFxParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
  return AK_PLUGIN_NEW( in_pAllocator, HijackFxParams() );
}
  
//Static initializer object to register automatically the plugin into the sound engine
AK::PluginRegistration HijackFxRegistration(AkPluginTypeEffect, AKCOMPANYID_ANKI, AKEFFECTID_HIJACK, CreateHijackFx, CreateHijackFxParams);

// Static function for post CreateHijackFx() work
std::function<void(HijackFx*)> HijackFx::PostCreateFxFunc = nullptr;


// Instance Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HijackFx::HijackFx() :
  _dsp( this )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFx::Init( AK::IAkPluginMemAlloc* in_pAllocator,
                         AK::IAkEffectPluginContext*	in_pEffectPluginContext,
                         AK::IAkPluginParam* in_pParams,
                         AkAudioFormat& in_rFormat )
{
  _params = static_cast<HijackFxParams*>( in_pParams );
  _dsp.Setup( in_rFormat );
  AKRESULT result = _dsp.InitHijack( in_pAllocator, in_rFormat );
  
  if ( nullptr != _initFunc ) {
    _initFunc( this );
  }
  
  // Profile plugin
  AK_PERF_RECORDING_RESET();
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFx::Term( AK::IAkPluginMemAlloc* in_pAllocator )
{
  if ( nullptr != _termFunc ) {
    _termFunc( this );
  }
  _dsp.TermHijack( in_pAllocator );
  AK_PLUGIN_DELETE( in_pAllocator, this );
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFx::Reset()
{
  _dsp.ResetHijack();
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFx::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
  out_rPluginInfo.eType = AkPluginTypeEffect;
  out_rPluginInfo.bIsInPlace = true;
  out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;

  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HijackFx::Execute( AkAudioBuffer* io_pBuffer )
{
  // // Profile plugin
  AK_PERF_RECORDING_START( "Hijack", 25, 30 );
  // Execute DSP processing synchronously here
  _dsp.Process( io_pBuffer );
  AK_PERF_RECORDING_STOP( "Hijack", 25, 30 );
}


} // PlugIns
} // AudioEngine
} // Anki
