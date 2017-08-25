/**
* File: robotAudioTestBuffer
*
* Author: damjan stulic
* Created: 6/9/16
*
* Description: This consists of a circular buffer to cache the audio samples from the Cozmo Plugin update. When there
*              is enough cached the data packed into a EngineToRobot audio sample message and is pushed into a
*              RobotAudioMessageStream. The RobotAudioMessageStreams are stored in a FIFO queue until they are ready
*              to be sent to the robot.
*
* Copyright: Anki, inc. 2016
*
*/


#include "helpers/audio/robotAudioTestBuffer.h"

namespace Anki {
namespace Cozmo {
namespace Audio {

void RobotAudioTestBuffer::PrepareAudioBuffer( double creationTime_ms )
{
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace( creationTime_ms );
}

} // Audio
} // Cozmo
} // Anki
