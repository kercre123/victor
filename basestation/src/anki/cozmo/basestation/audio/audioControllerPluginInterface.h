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


#include "util/helpers/noncopyable.h"
#include <cstdint>


namespace Anki {
namespace Cozmo {
namespace Audio {

class AudioController;

class AudioControllerPluginInterface: public Util::noncopyable {
  
public:
  AudioControllerPluginInterface( AudioController& parentAudioController );
  
  // WavePortal Interface
  // Set Audio Data to be used when the plugin is used
  void SetWavePortalAudioDataInfo( uint32_t sampleRate,
                                   uint16_t numberOfChannels,
                                   float duration_ms,
                                   float* audioBuffer,
                                   uint32_t bufferSize );
  
  // Clear the Audio Data from the plugin
  void ClearWavePortalAudioDataInfo();
  
  // Check if the plugin has Audio Data
  bool WavePortalHasAudioDataInfo();
  
private:
  
  AudioController& _parentAudioController;
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioControllerPluginInterface_H__ */
