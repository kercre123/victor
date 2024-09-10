/*
 * File: akAlsaSinkPlugIn.h
 *
 * Author: Matt Michini
 * Created: 6/19/2018
 *
 * Description: This wraps the AkAlsaSink plug-in instance. This is intended to be interacted with at app level.
 *
 *
 * Copyright: Anki, Inc. 2018
 */


#ifndef __AnkiAudio_PlugIns_AkAlsaSinkPlugIn_H__
#define __AnkiAudio_PlugIns_AkAlsaSinkPlugIn_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include "audioEngine/plugins/akAlsaSinkPluginTypes.h"
#include "util/container/fixedCircularBuffer.h"
#include <cstdint>
#include <mutex>

class AkAlsaSink;

namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
class AUDIOENGINE_EXPORT AkAlsaSinkPlugIn
{
public:
  
  AkAlsaSinkPlugIn();
  
  ~AkAlsaSinkPlugIn();
  
  // Non-Copyable Class
  AkAlsaSinkPlugIn( const AkAlsaSinkPlugIn& ) = delete;
  AkAlsaSinkPlugIn& operator=( const AkAlsaSinkPlugIn& ) = delete;
  
  // Register plugin with Audio Engine
  bool RegisterPlugIn();
  
  // Set Alsa Audio callback thread affinity mask
  // Note: Need to set this when registering the plugin, before plugin Fx is created by audio engine
  void SetAudioThreadAffinityMask( AudioUInt32 affinityMask ) { _audioThreadAffinityMask = affinityMask; }
  
  // Returns true if we are currently playing sound through the speaker
  bool IsUsingSpeaker() const;
  
  // Get the maximum speaker 'latency', which is the max delay between when we
  // command audio to be played and it actually gets played on the speaker
  uint32_t GetSpeakerLatency_ms() const;
  
  uint32_t GetSampleRate() const;
  uint8_t  GetNumberOfChannels() const;
  
  // NOTE: This callback happens on the Audio Thread, DO NOT BLOCK!!! Copy the data and get OUT!!
  void SetPlaybackBufferCallback(const AkAlsaSinkPluginTypes::WriteBufferToAlsaCallbackFunc callback);

  
private:

#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  AkAlsaSink* _pluginInstance;
#endif
  
  AudioEngine::AudioUInt32 _audioThreadAffinityMask;
  AkAlsaSinkPluginTypes::WriteBufferToAlsaCallbackFunc _writeToAlsaCallback;

};

} // PlugIns
} // AudioEngine
} // Anki

#endif // __AnkiAudio_PlugIns_AkAlsaSinkPlugIn_H__
