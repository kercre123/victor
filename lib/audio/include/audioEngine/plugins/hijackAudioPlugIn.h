/*
 * File: hijackAudioPlugIn.h
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: This wraps and holds the HijackFx plug-in instance to provide callbacks for Create, Destroy & Process
 *              events. This is intended to be interacted with at app level.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __AnkiAudio_PlugIns_HijackAudioPlugIn_H__
#define __AnkiAudio_PlugIns_HijackAudioPlugIn_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include <mutex>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

class HijackFx;

class AUDIOENGINE_EXPORT HijackAudioPlugIn
{
public:
  
  using PlugInEventFunc = std::function<void( const uint32_t plugInId )>;
  using AudioBufferFunc = std::function<void( const uint32_t plugInId, const AudioReal32* samples, const uint32_t sampleCount )>;
  
  
  HijackAudioPlugIn( uint32_t resampleRate, uint16_t samplesPerFrame );
  
  ~HijackAudioPlugIn();
  
  bool RegisterPlugin();
  
  bool SetCreatePlugInCallback( PlugInEventFunc callback );
  
  bool SetDestroyPluginCallback( PlugInEventFunc callback );
  
  bool SetProcessCallback( AudioBufferFunc callback );
  
  
  // This is for private use only
  // Wwise SDK's expects a c function when registering a plug-in. This method allows that callback function to
  // intergrate the plug-in Fx with this class.
  void SetupEnginePlugInFx( HijackFx* plugIn );
  
  
private:
      
  PlugInEventFunc _createCallback     = nullptr;
  PlugInEventFunc _destroyCallback    = nullptr;
  AudioBufferFunc _processCallback    = nullptr;
  
  const uint32_t _resampleRate;
  const uint32_t _samplesPerFrame;
  
  std::mutex _setCallbackMutex;
};

  
} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_HijackAudioPlugIn_H__ */
