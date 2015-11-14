//
//  RobotAudioBuffer.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/13/15.
//
//

#ifndef RobotAudioBuffer_h
#define RobotAudioBuffer_h

#include <stdint.h>
#include <stdio.h>

namespace Anki {
namespace Cozmo {
namespace Audio {

class RobotAudioBuffer
{
  
public:
  
  bool hasBuffer() { return false; }
  
  void UpdateBuffer( uint8_t* samples, uint32_t sampleCount );
  
private:
  
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* RobotAudioBuffer_h */
