/*
 * File: wavePortalFx.h
 *
 * Author: Jordan Rivas
 * Created: 3/24/2016
 *
 * Description: This is the main class for WavePortal Plug-in. It acts as the primary interface to set an
 *              AudioDataStream struct as the playback source when this plug-in is used by Wwise audio engine.
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __AnkiAudio_PlugIns_WavePortalFx_H__
#define __AnkiAudio_PlugIns_WavePortalFx_H__


#include "audioEngine/plugins/wavePortalFxTypes.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "wavePortalFxParams.h"
#include <functional>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  

class WavePortalFx : public AK::IAkSourcePlugin
{
public:
  
  // Function will be called after allocating plugin instance in CreateWavePortalFx()
  static std::function<void(WavePortalFx*)> PostCreateFxFunc;
  
  // Param:
  // in_pAllocator - Interface to memory allocator to be used by the effect
  // in_pEffectPluginContext - Interface to effect plug-in's context
  // in_pParams - Interface to plug-in parameters
  // io_rFormat - Audio data format of the input/output signal. Only an out-of-place plugin is allowed to change the
  //              channel configuration.
  virtual AKRESULT Init(AK::IAkPluginMemAlloc* in_pAllocator,
                        AK::IAkSourcePluginContext*	in_pEffectPluginContext,
                        AK::IAkPluginParam* in_pParams,
                        AkAudioFormat& io_rFormat ) override;
  
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
  
  /// Get the total duration of the source in milliseconds.
  virtual AkReal32 GetDuration() const override;

  // Set the plug-in's source data
  void SetAudioDataStreamOwnership( const AudioDataStream *dataStream ) { _audioDataStream = dataStream; }
  
  // Get Id of Plug-in
  AkInt32 GetParamsPlugInId() const { return _params->GetPlugInId(); }
  
  // Set plugin life cycle callback functions
  using PluginCallbackFunc = std::function<void( const WavePortalFx* pluginInstance )>;
  void SetInitCallback( PluginCallbackFunc callback ) { _initFunc = callback; }
  void SetTerminateCallback( PluginCallbackFunc callback ) { _termFunc = callback; }

private:
  
  // Plugin parameters
  WavePortalFxParams*   _params    = nullptr;
  uint32_t              _sourceIdx = 0;
  
  // Audio source to playback
  const AudioDataStream* _audioDataStream = nullptr;
  // Callback when the plugin's init method is completed
  PluginCallbackFunc    _initFunc   = nullptr;
  // Callback when plugin is going to be destroyed
  PluginCallbackFunc    _termFunc   = nullptr;
};


} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_WavePortalFx_H__ */
