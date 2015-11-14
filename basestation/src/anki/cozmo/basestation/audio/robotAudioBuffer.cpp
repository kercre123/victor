//
//  RobotAudioBuffer.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/13/15.
//
//


#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"

namespace Anki {
namespace Cozmo {
namespace Audio {
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
void RobotAudioBuffer::UpdateBuffer( uint8_t* samples, uint32_t sampleCount )
{
  
  printf("\n\nRobotAudioBuffer::UpdateBuffer  -- sample count %d\n\n", sampleCount );
}
  
} // Audio
} // Cozmo
} // Anki
