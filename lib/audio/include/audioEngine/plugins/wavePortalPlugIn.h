/*
 * File: wavePortalPlugIn.h
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


#ifndef __AnkiAudio_PlugIns_WavePortalPlugIn_H__
#define __AnkiAudio_PlugIns_WavePortalPlugIn_H__

#include "audioEngine/audioExport.h"
#include <string>
#include <unordered_map>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
class WavePortalFx;
struct AudioDataStream;

class AUDIOENGINE_EXPORT WavePortalPlugIn
{
public:
  
  WavePortalPlugIn();
  
  ~WavePortalPlugIn();
  
  // Non-Copyable Class
  WavePortalPlugIn( const WavePortalPlugIn& ) = delete;
  WavePortalPlugIn& operator=( const WavePortalPlugIn& ) = delete;
  
  // Register plugin with Audio Engine
  bool RegisterPlugIn();
  
  // Audio Data Methods
  // Give ownership of audio data to plugin
  // Note: Previous audio data should be cleared before setting.
  //       When the audio engine creates the source plugin WavePortalFX the ownership of the audio data will be
  //       transferred to that plugin and will be cleared form this class' instance
  void SetAudioDataOwnership( const AudioDataStream* audioDataStream );
  
  // Clear the Audio Data from the plugin
  // Note: This is used to delete audio data that was set but was never used. It is safe to call even if the audio
  //       data has been transferred
  void ClearAudioData();
  
  // Check if the plugin has Audio Data
  bool HasAudioDataInfo() const { return _audioDataStream != nullptr; }
  
  // Check if plugin is in uses
  bool IsActive() const { return _isActive; }
  
  // Set plugin life cycle callback functions
  using PluginCallbackFunc = std::function<void( const WavePortalPlugIn* pluginInstance )>;
  void SetInitCallback( PluginCallbackFunc callback ) { _initFunc = callback; }
  void SetTerminateCallback( PluginCallbackFunc callback ) { _termFunc = callback; }
  
  
  // This is for private use only
  // Wwise SDK's expects a c function when registering a plug-in. This method allows that callback function to
  // integrate the plug-in Fx with this class.
  void SetupEnginePlugInFx( WavePortalFx* plugIn );
  
private:
  
  const AudioDataStream* _audioDataStream = nullptr;
  bool _isActive = false;
  PluginCallbackFunc    _initFunc   = nullptr;    // Callback when the plugin's init method is completed
  PluginCallbackFunc    _termFunc   = nullptr;    // Callback when plugin is going to be destroyed
  
};

} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_WavePortalPlugIn_H__ */
