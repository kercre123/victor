/*
 * File: streamingWavePortalFx.h
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

#ifndef __AnkiAudio_Plugins_StreamingWavePortalFx_H__
#define __AnkiAudio_Plugins_StreamingWavePortalFx_H__


#include "audioEngine/audioTools/streamingWaveDataInstance.h"
#include "streamingWavePortalParams.h"
#include <AK/Plugin/PluginServices/AkFXDurationHandler.h>
#include <memory>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {


class StreamingWavePortalFx : public AK::IAkSourcePlugin
{
public:

  // Function will be called after allocating plugin instance in CreateWavePortalFx()
  static std::function<void(StreamingWavePortalFx*)> PostCreateFxFunc;

  // Constructor.
  StreamingWavePortalFx();

  // Destructor.
  virtual ~StreamingWavePortalFx() override;

  // Source plug-in initialization.
  virtual AKRESULT Init( AK::IAkPluginMemAlloc*			  in_pAllocator,      // Memory allocator interface.
                         AK::IAkSourcePluginContext*  in_pSourceFXContext,// Sound engine plug-in execution context.
                         AK::IAkPluginParam*			    in_pParams,         // Associated effect parameters node.
                         AkAudioFormat&					      io_rFormat          // Output audio format.
  ) override;

  // Source plug-in termination.
  virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator ) override;

  // Reset effect.
  virtual AKRESULT Reset( ) override;

  // Plug-in info query.
  virtual AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo ) override;

  // Effect plug-in DSP processing.
  // Output audio buffer structure.
  virtual void Execute( AkAudioBuffer *	io_pBuffer ) override;

  // Get the total duration of the source in milliseconds.
  virtual AkReal32 GetDuration( ) const override;

  // Get Wwise Parameter Plugin Id
  // Note: This must be called after Init() otherwise will assert
  uint8_t GetPluginId() const;

  // Set a shared pointer to data instance
  using StreamingWaveDataInstance = Anki::AudioEngine::StreamingWaveDataInstance;
  void SetDataInstance( const std::shared_ptr<StreamingWaveDataInstance>& instance ) { _data = instance; }

  // Set plugin life cycle callback functions
  using PluginCallbackFunc = std::function<void( StreamingWavePortalFx* pluginInstance )>;
  void SetInitCallback( PluginCallbackFunc callback ) { _initFunc = callback; }
  void SetTerminateCallback( PluginCallbackFunc callback ) { _termFunc = callback; }


private:

  // Parameters node
  StreamingWavePortalParams * m_pParams;
  // Shared data instance
  std::shared_ptr<StreamingWaveDataInstance> _data;

  // Callback when the plugin's init method is completed
  PluginCallbackFunc    _initFunc   = nullptr;
  // Callback when plugin is going to be destroyed
  PluginCallbackFunc    _termFunc   = nullptr;

};
  
} // Plugins
} // AudioEngine
} // Anki

#endif // __AnkiAudio_Plugins_StreamingWavePortalFx_H__
