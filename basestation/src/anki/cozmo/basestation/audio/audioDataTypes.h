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

#ifndef __Basestation_Audio_AudioDataTypes_H__
#define __Basestation_Audio_AudioDataTypes_H__


#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <stdint.h>
#include <memory>
#include <sstream>
#include <iomanip>
//#include <istream>


namespace Anki {
namespace Cozmo {
namespace Audio {

using AudioSample = float;

struct AudioFrameData {
  AudioSample*  samples      = nullptr;
  size_t        sampleCount  = 0;
  
  AudioFrameData( size_t size )
  : sampleCount( size )
  {
    if ( sampleCount > 0) {
      samples = new AudioSample[sampleCount];
    }
  }
  
  ~AudioFrameData()
  {
    Util::SafeDeleteArray( samples );
  }
  
  void CopySamples( const AudioSample* source, size_t sourceSize )
  {
    DEV_ASSERT( sourceSize <= sampleCount, "AudioFrameData.CopySamples.InvalidSourceSize");
    memcpy( samples, source, sourceSize * sizeof(AudioSample) );
    // Pad the end of the frame with zeros
    if ( sourceSize < sampleCount) {
      memset( samples + (sourceSize * sizeof(AudioSample)) , 0.f, (sampleCount-sourceSize) * sizeof(AudioSample));
    }
  }
  
  std::string Description() const {
    std::stringstream strStr;
    strStr << std::fixed << std::setprecision(2);
    strStr << "SampleCount: " << sampleCount << "  Samples:";
    for (size_t idx = 0; idx < sampleCount; ++idx) {
      strStr << " " << samples[idx] << " |";
    }
    return strStr.str();
  }
};

//static constexpr AudioFrameData INVALID_AUDIO_FRAME = nullptr;

} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioDataTypes_H__ */
