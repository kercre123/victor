/*
 * File: audioControllerPluginInterface.h
 *
 * Author: Jordan Rivas
 * Created: 4/05/2016
 *
 * Description: This class wraps Audio Engine custom plugins to allow the app’s objects to interface with them. The
 *              interface is intended to remove the complexities of compile issues since the Audio Engine can be
 *              complied out of the project.
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#ifndef __Basestation_Audio_AudioControllerPluginInterface_H__
#define __Basestation_Audio_AudioControllerPluginInterface_H__


#include "anki/cozmo/basestation/audio/standardWaveDataContainer.h"
#include "util/helpers/noncopyable.h"
#include <cstdint>
#include <functional>


namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioController;

class AudioControllerPluginInterface: public Util::noncopyable {
  
public:
  AudioControllerPluginInterface( AudioController& parentAudioController );
  
  // WavePortal Interface
  // TODO: At some point there will be more then 1 instance of the WavePortal Plug-in they will be tracked by their Id
  
  // Give Audio Data ownership to plugin for playback
  void GiveWavePortalAudioDataOwnership(const StandardWaveDataContainer* audioData);
  
  // Clear the Audio Data from the plugin
  // Note: This is used to release audio data that is not going to be used
  void ClearWavePortalAudioData();
  
  // Check if the plugin has Audio Data
  bool WavePortalHasAudioDataInfo() const;
  
  // Check if plugin is in uses
  bool WavePortalIsActive() const;
  
  // Set plugin life cycle callback functions
  // Note: If the plugin is compiled out the callback functions will not be called
  using PluginCallbackFunc = std::function<void( const AudioControllerPluginInterface* pluginInstance )>;
  void SetWavePortalInitCallback( PluginCallbackFunc callback );
  void SetWavePortalTerminateCallback( PluginCallbackFunc callback );


private:
  
  AudioController& _parentAudioController;
  
  // WavePortal
  PluginCallbackFunc    _wavePortalInitFunc   = nullptr;    // Callback when the plugin's init method is completed
  PluginCallbackFunc    _wavePortalTermFunc   = nullptr;    // Callback when plugin is going to be destroyed
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioControllerPluginInterface_H__ */
