//
//  standardWaveDataContainer.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 4/29/16.
//
//

#ifndef __Basestation_Audio_StandardWaveDataContainer_H__
#define __Basestation_Audio_StandardWaveDataContainer_H__

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include <stdint.h>

namespace Anki {
  namespace Cozmo {
    namespace Audio {
      
      // Define a audio data in a standard form
      struct StandardWaveDataContainer
      {
        uint32_t sampleRate       = 0;
        uint16_t numberOfChannels = 0;
        size_t   bufferSize       = 0;
        float*   audioBuffer      = nullptr;
        
        StandardWaveDataContainer( uint32_t sampleRate,
                                   uint16_t numberOfChannels,
                                   size_t   bufferSize = 0 )
        : sampleRate( sampleRate )
        , numberOfChannels( numberOfChannels )
        {
          if ( bufferSize > 0 ) {
            CreateDataBuffer( bufferSize );
          }
        }
        
        ~StandardWaveDataContainer()
        {
          Util::SafeDeleteArray(audioBuffer);
        }
        
        // Allocate necessary memory for audio buffer
        bool CreateDataBuffer(const size_t size)
        {
          DEV_ASSERT_MSG(audioBuffer == nullptr,
                         "StandardWaveDataContainer.CreateDataBuffer.AudioBufferNotNull",
                         "Can NOT allocate memory, Audio Buffer is not NULL");
          DEV_ASSERT_MSG(size > 0,
                         "StandardWaveDataContainer.CreateDataBuffer.SizeNotZero",
                         "Must set buffer size");
          
          audioBuffer = new (std::nothrow) float[size];
          if ( audioBuffer == nullptr ) {
            return false;
          }
          
          bufferSize = size;
          return true;
        }
        
        // Check if there is an audio buffer
        bool HasBuffer()
        {
          return ( (bufferSize > 0) && (audioBuffer != nullptr) );
        }
        
        float ApproximateDuration_ms() const
        {
          const double sampleCount = bufferSize / numberOfChannels;
          return static_cast<float>( (sampleCount / (double)sampleRate) * 1000.0 );
        }
        
        void ReleaseAudioDataOwnership()
        {
          // Reset vars
          sampleRate = 0;
          numberOfChannels = 0;
          bufferSize = 0;
          audioBuffer = nullptr;
        }
      };
      
    } // Audio
  } // Cozmo
} // Anki

#endif /* __Basestation_Audio_StandardWaveDataContainer_H__ */
