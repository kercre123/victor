/*
 * File: audioDataTypes.h
 *
 * Author: Jordan Rivas
 * Created: 5/16/16
 *
 * Description: Standard types for Audio services
 *              - Define Audio format
 *              - Transport Audio
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __AnkiAudio_AudioDataTypes_H__
#define __AnkiAudio_AudioDataTypes_H__

#include "audioEngine/audioExport.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <stdint.h>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>

namespace Anki {
namespace AudioEngine {

using AudioSample = float;

struct AUDIOENGINE_EXPORT AudioFrameData {
  std::vector<AudioSample>  samples;
  
  void CopySamples( const AudioSample* source, size_t sourceSize )
  {
    samples.resize(sourceSize);
    std::copy(source, source + sourceSize, samples.data());
  }
  
  std::string Description() const {
    std::stringstream strStr;
    strStr << std::fixed << std::setprecision(2);
    strStr << "SampleCount: " << samples.size() << "  Samples:";
    for (size_t idx = 0; idx < samples.size(); ++idx) {
      strStr << " " << samples[idx] << " |";
    }
    return strStr.str();
  }
};

//static constexpr AudioFrameData INVALID_AUDIO_FRAME = nullptr;

} // AudioEngine
} // Anki

#endif /* __AnkiAudio_AudioDataTypes_H__ */
