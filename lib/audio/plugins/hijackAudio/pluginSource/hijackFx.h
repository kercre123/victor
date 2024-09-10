/*
 * File: hijackFx.h
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

#ifndef __AnkiAudio_PlugIns_HijackFx_H__
#define __AnkiAudio_PlugIns_HijackFx_H__

#include "hijackFxDsp.h"
#include "hijackFxParams.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <functional>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {


class HijackFx : public AK::IAkInPlaceEffectPlugin
{
public:
  
  // Function will be called after allocating plugin instance in CreateHijackFx()
  static std::function<void(HijackFx*)> PostCreateFxFunc;
  
  HijackFx();
  
  // Param:
  // in_pAllocator - Interface to memory allocator to be used by the effect
  // in_pEffectPluginContext - Interface to effect plug-in's context
  // in_pParams - Interface to plug-in parameters
  // io_rFormat - Audio data format of the input/output signal. Only an out-of-place plugin is allowed to change the
  //              channel configuration.
  AKRESULT Init( AK::IAkPluginMemAlloc* in_pAllocator,
                 AK::IAkEffectPluginContext*	in_pEffectPluginContext,
                 AK::IAkPluginParam* in_pParams,
                 AkAudioFormat& in_rFormat ) override;
  
  // Param:
  // in_pAllocator - Interface to memory allocator to be used by the plug-in
  AKRESULT Term( AK::IAkPluginMemAlloc* in_pAllocator ) override;
  
  
  AKRESULT Reset() override;
  
  // Param:
  // out_rPluginInfo - Reference to the plug-in information structure to be retrieved
  AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo ) override;
  
  // Param:
  // io_pBuffer - In/Out audio buffer data structure (in-place processing)
  void Execute( AkAudioBuffer* io_pBuffer ) override;
  
  // Execution processing when the voice is virtual. Nothing special to do for this effect.
  AKRESULT TimeSkip(AkUInt32 in_uFrames) override { return AK_DataReady; }

  
  using DidInitPluginFunc = std::function<void( const HijackFx* pluginInstance )>;
  void SetDidInitCallback( DidInitPluginFunc callback ) { _initFunc = callback; }

  void SetAudioBufferCallback( HijackFxDsp::AudioBufferCallback bufferCallback ) { _dsp.SetAudioBufferCallback( bufferCallback ); }
  
  using WillTerminatePluginFunc = std::function<void( const HijackFx* pluginInstance )>;
  void SetTerminateCallback( WillTerminatePluginFunc callback ) { _termFunc = callback; }
  
  void SetDspSettings( const uint32_t resampleRate, const uint16_t samplesPerFrame ) { _dsp.SetResampleRate(resampleRate); _dsp.SetSamplesPerFrame(samplesPerFrame); }
  
  AkInt32 GetParamsPlugInId() const { return _params->GetPlugInId(); }
  
  
private:
  
  HijackFxDsp             _dsp;                     // Internal effect
  HijackFxParams*         _params     = nullptr;    // Effect parameter
  DidInitPluginFunc       _initFunc   = nullptr;    // Callback when the plugin's init method is completed
  WillTerminatePluginFunc _termFunc   = nullptr;    // Callback when plugin is going to be destroyed

};


} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_HijackFx_H__ */
