/*
 * File: streamingWavePortalPlugIn.h
 *
 * Author: Jordan Rivas
 * Created: 7/11/2018
 *
 * Description: This wraps the StreamingWavePortalFx plug-in instance to provide Audio Data source for playback. This
 *              is intended to be interacted with at app level.
 *
 * Note:        This Plug-in will assert if you don't set the Audio Data Stream before the plug-in is called by audio
 *              engine (Wwise)
 *
 * Copyright: Anki, Inc. 2018
 */


#ifndef __AnkiAudio_PlugIns_StreamingWavePortalPlugIn_H__
#define __AnkiAudio_PlugIns_StreamingWavePortalPlugIn_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTools/streamingWaveDataInstance.h"
#include <string>
#include <unordered_map>
#include <map>
#include <memory>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
class StreamingWavePortalFx;


class AUDIOENGINE_EXPORT StreamingWavePortalPlugIn
{
public:

  using PluginId_t = uint8_t;

  StreamingWavePortalPlugIn();

  ~StreamingWavePortalPlugIn();

  // Non-Copyable Class
  StreamingWavePortalPlugIn( const StreamingWavePortalPlugIn& ) = delete;
  StreamingWavePortalPlugIn& operator=( const StreamingWavePortalPlugIn& ) = delete;

  // Register plugin with Audio Engine
  bool RegisterPlugIn();

  // Audio Data Methods
  // Create a StreamingWaveDataInstance
  static std::shared_ptr<StreamingWaveDataInstance> CreateDataInstance()
    { return std::make_shared<StreamingWaveDataInstance>(); }

  // Before playing audio stream add data instance to plugin with Id
  // Return nullPtr if StreamingWaveDataInstance already exist for pluginId
  bool AddDataInstance( const std::shared_ptr<StreamingWaveDataInstance>& dataInstance, PluginId_t pluginId );

  // Return null if can't find plugin
  std::shared_ptr<StreamingWaveDataInstance> GetDataInstance( PluginId_t pluginId ) const;

  // Clear the Audio Data from the plugin
  // Note: This is used to delete audio data that was set but was never used. It is safe to call even if the audio
  //       data has been transferred
  void ClearAudioData( PluginId_t pluginId );

  // Check if the plugin has Audio Data
  bool HasAudioDataInfo( PluginId_t pluginId ) const;

  // Check if plugin is in uses
  bool IsPluginActive( PluginId_t pluginId ) const;

  // Set plugin life cycle callback functions
  using PluginCallbackFunc = std::function<void( const StreamingWavePortalPlugIn* pluginInstance )>;
  void SetInitCallback( PluginCallbackFunc callback ) { _initFunc = callback; }
  void SetTerminateCallback( PluginCallbackFunc callback ) { _termFunc = callback; }


  // This is for private use only
  // Wwise SDK's expects a c function when registering a plug-in. This method allows that callback function to
  // integrate the plug-in Fx with this class.
  void SetupEnginePlugInFx( StreamingWavePortalFx* plugIn );

private:

  // Track data in use by each plugin ID
  std::map<uint8_t, std::shared_ptr<StreamingWaveDataInstance>> _dataInstanceMap;

  // Mutex for thread-safe access to std::map
  mutable std::mutex _dataInstanceMutex;

  PluginCallbackFunc    _initFunc   = nullptr;    // Callback when the plugin's init method is completed
  PluginCallbackFunc    _termFunc   = nullptr;    // Callback when plugin is going to be destroyed

};

} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_StreamingWavePortalPlugIn_H__ */
