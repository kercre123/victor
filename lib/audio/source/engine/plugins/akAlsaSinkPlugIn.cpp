/*
 * File: akAlsaSinkPlugIn.cpp
 *
 * Author: Matt Michini
 * Created: 6/19/2018
 *
 * Description: This wraps the AkAlsaSink plug-in instance. This is intended to be interacted with at app level.
 *
 *
 * Copyright: Anki, Inc. 2018
 */


#include "audioEngine/plugins/akAlsaSinkPlugIn.h"

#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
#include "AkAlsaSink.h"
#endif
#include "audioEngine/audioEngineConfigDefines.h"

#include "util/logging/logging.h"

namespace Anki {
namespace AudioEngine {
namespace PlugIns {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AkAlsaSinkPlugIn::AkAlsaSinkPlugIn()
: _audioThreadAffinityMask(SetupConfig::ThreadProperties::kInvalidAffinityMask)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AkAlsaSinkPlugIn::~AkAlsaSinkPlugIn()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AkAlsaSinkPlugIn::RegisterPlugIn()
{
  bool success = false;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  AkAlsaSink::PostCreateAlsaFunc = [this](AkAlsaSink* plugin)
    {
      _pluginInstance = plugin;
      // Set affinity after constructor before Init() methd
      if ( _audioThreadAffinityMask != SetupConfig::ThreadProperties::kInvalidAffinityMask ) {
        _pluginInstance->m_uCpuMask = _audioThreadAffinityMask;
      }
      _pluginInstance->m_writeBufferToAlsaCallback = _writeToAlsaCallback;
    };
  success = true;
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AkAlsaSinkPlugIn::IsUsingSpeaker() const
{
  bool usingSpeaker = false;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  usingSpeaker = (_pluginInstance != nullptr) && _pluginInstance->IsUsingSpeaker();
#endif
  return usingSpeaker;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AkAlsaSinkPlugIn::GetSpeakerLatency_ms() const
{
  uint32_t latency = 0;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  if (_pluginInstance != nullptr) {
    latency = _pluginInstance->GetSpeakerLatency_ms();
  }
#endif
  return latency;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AkAlsaSinkPlugIn::GetSampleRate() const
{
  uint32_t sampleRate = 0;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  if (_pluginInstance != nullptr) {
    sampleRate = _pluginInstance->m_uSampleRate;
  }
#endif
  return sampleRate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t  AkAlsaSinkPlugIn::GetNumberOfChannels() const
{
  uint8_t count = 0;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  if (_pluginInstance != nullptr) {
    count = _pluginInstance->m_uOutNumChannels;
  }
#endif
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void AkAlsaSinkPlugIn::SetPlaybackBufferCallback(const AkAlsaSinkPluginTypes::WriteBufferToAlsaCallbackFunc callback)
{
  _writeToAlsaCallback = callback;
#if defined VICOS && not defined EXCLUDE_ANKI_AUDIO_LIBS
  if (_pluginInstance != nullptr) {
    _pluginInstance->m_writeBufferToAlsaCallback = _writeToAlsaCallback;
  }
#endif
}


} // PlugIns
} // AudioEngine
} // Anki
