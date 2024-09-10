/*
 * File: akAlsaSinkPluginTypes.h
 *
 * Author: Jordan Rivas
 * Created: 02/01/19
 *
 * Description: Constants for Ak ALSA Sink Plugin
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __AnkiAudio_PlugIns_AkAlsaSinkPluginTypes_H__
#define __AnkiAudio_PlugIns_AkAlsaSinkPluginTypes_H__

#include <cstdint>
#include <functional>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  class AkAlsaSinkPluginTypes {
  public:
    // NOTE: These are all expected in Vector Project
    static constexpr size_t kAkAlsaSinkSampleRate   = 32000;
    static constexpr size_t kAkAlsaSinkBufferSize   = 1024;
    static constexpr size_t kAkAlsaSinkBufferCount  = 3;
    static constexpr size_t kAkAlsaSinkChannelCount = 1;
    
    
    using AkSinkAudioSample_t = int16_t;
    using AudioChunk = AkSinkAudioSample_t[ kAkAlsaSinkBufferSize ];
    
    // Callback for Audio Engine end of frame
    // param: period is reference to periods data
    // param: hasData is true when the frame has audio data, not silence
    using WriteBufferToAlsaCallbackFunc = std::function<void(const AudioChunk& chunk, bool hasData)>;
  };

} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_AkAlsaSinkPluginTypes_H__ */
