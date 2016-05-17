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
    ASSERT_NAMED( sourceSize <= sampleCount, "AudioFrameData.CopySamples.InvalidSourceSize");
    memcpy( samples, source, sourceSize * sizeof(AudioSample) );
    // Pad the end of the frame with zeros
    if ( sourceSize < sampleCount) {
      memset( samples + (sourceSize * sizeof(AudioSample)) , 0, (sampleCount-sourceSize) * sizeof(AudioSample));
    }
  }
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioDataTypes_H__ */
