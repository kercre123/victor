/*
 * File: AnkiPluginInterface.h
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

#ifndef __AnkiAudio_AnkiPluginInterface_H__
#define __AnkiAudio_AnkiPluginInterface_H__


#include "audioEngine/audioExport.h"
#include "audioEngine/audioEngineConfigDefines.h"
#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "util/helpers/noncopyable.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace Anki {
namespace AudioEngine {

class AudioEngineController;

namespace PlugIns {
class AkAlsaSinkPlugIn;
class HijackAudioPlugIn;
class StreamingWavePortalPlugIn;
class WavePortalPlugIn;


class AUDIOENGINE_EXPORT AnkiPluginInterface : private Anki::Util::noncopyable {
  
public:
  // WavePortal Interface
  // TODO: At some point there will be more then 1 instance of the WavePortal Plug-in they will be tracked by their Id
  AnkiPluginInterface();
  
  ~AnkiPluginInterface();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Wave Portal Plugin -- DEPRECATED -- Use StreamingWavePortal
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SetupWavePortalPlugIn();
  
  WavePortalPlugIn* const GetWavePortalPlugIn() const { return _wavePortalPlugIn.get(); }
  
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
  using PluginCallbackFunc = std::function<void( const AnkiPluginInterface* pluginInstance )>;
  void SetWavePortalInitCallback( PluginCallbackFunc callback );
  void SetWavePortalTerminateCallback( PluginCallbackFunc callback );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Streaming Wave Portal Plugin
  // Interface directly with StreamingWavePortalPlugIn
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SetupStreamingWavePortalPlugIn();
  
  StreamingWavePortalPlugIn* const GetStreamingWavePortalPlugIn() const { return _streamingWavePortalPlugIn.get(); }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Hijack Audio Plugin
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SetupHijackAudioPlugInAndRobotAudioBuffers( uint32_t resampleRate, uint16_t samplesPerFrame );
  
  HijackAudioPlugIn* const GetHijackAudioPlugIn() const { return _hijackAudioPlugIn.get(); }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // AK ALSA Sink Plugin
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SetupAkAlsaSinkPlugIn( AudioUInt32 affinityMask = SetupConfig::ThreadProperties::kInvalidAffinityMask );
  
  AkAlsaSinkPlugIn* const GetAlsaSinkPlugIn() const { return _akAlsaSinkPlugIn.get(); }
  
  // Returns true if we are currently playing sound through the speaker
  bool AkAlsaSinkIsUsingSpeaker() const;
  
  // Get the maximum speaker 'latency', which is the max delay between when we
  // command audio to be played and it actually gets played on the speaker
  uint32_t AkAlsaSinkGetSpeakerLatency_ms() const;

private:
  
//  AudioEngineController& _parentAudioController;
  
  // Custom Plugins Instance
  std::unique_ptr<AkAlsaSinkPlugIn>           _akAlsaSinkPlugIn;
  std::unique_ptr<HijackAudioPlugIn>          _hijackAudioPlugIn;
  std::unique_ptr<WavePortalPlugIn>           _wavePortalPlugIn;
  std::unique_ptr<StreamingWavePortalPlugIn>  _streamingWavePortalPlugIn;
  
  // WavePortal
  PluginCallbackFunc    _wavePortalInitFunc   = nullptr;    // Callback when the plugin's init method is completed
  PluginCallbackFunc    _wavePortalTermFunc   = nullptr;    // Callback when plugin is going to be destroyed
  
};

} // PlugIns
} // AudioEngine
} // Anki

#endif /* __Basestation_Audio_AnkiPluginInterface_H__ */
