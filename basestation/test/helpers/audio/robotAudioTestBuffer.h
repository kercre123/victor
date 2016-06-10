/**
* File: robotAudioTestBuffer
*
* Author: damjan stulic
* Created: 6/9/16
*
* Description: This consists of a circular buffer to cache the audio samples from the Cozmo Plugin update. When there
*              is enough cached, the data is packed into a EngineToRobot audio sample message and is pushed into a
*              RobotAudioMessageStream. The RobotAudioMessageStreams are stored in a FIFO queue until they are ready
*              to be sent to the robot.
*
* Copyright: Anki, inc. 2016
*
*/


#ifndef __Test_Helpers_Audio_RobotAudioTestBuffer_H__
#define __Test_Helpers_Audio_RobotAudioTestBuffer_H__

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"

namespace Anki {
namespace Cozmo {
namespace Audio {


class RobotAudioTestBuffer: public RobotAudioBuffer
{

};


} // Audio
} // Cozmo
} // Anki

#endif /* __Test_Helpers_Audio_RobotAudioTestBuffer_H__ */
