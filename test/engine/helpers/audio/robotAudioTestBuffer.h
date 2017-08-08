/*
 * File: robotAudioTestBuffer
 *
 * Author: damjan stulic
 * Created: 6/9/16
 *
 * Description: This is a FIFO queue of RobotAudioFrameStreams which contain a continues stream of audio frames. The
 *              RobotAudioAnimation class will pop frames out of the RobotAudioFrameStreams and sync them with the rest
 *              of the animation tracks. Once a RobotAudioFrameStreams is empty it will be popped of the queue. The
 *              Audio Controller passes audio frames provided by the audio engine. First,  PrepareAudioBuffer() is
 *              called by the Audio Controller a new stream is created and pushed onto the back of the _streamQueue.
 *              Next, UpdateBuffer() is called by the Audio Controller to provide audio frames to the _currentStream.
 *              When all audio frames have been added to the stream the Audio Controller will called CloseAudioBuffer()
 *              to complete that stream.
 *
 * Copyright: Anki, inc. 2016
 *
 */


#ifndef __Test_Helpers_Audio_RobotAudioTestBuffer_H__
#define __Test_Helpers_Audio_RobotAudioTestBuffer_H__

#include "engine/audio/robotAudioBuffer.h"

namespace Anki {
namespace Cozmo {
namespace Audio {


class RobotAudioTestBuffer: public RobotAudioBuffer
{
  
public:
  // Set Creation time of buffer for Unit test
  void PrepareAudioBuffer( double creationTime_ms );
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Test_Helpers_Audio_RobotAudioTestBuffer_H__ */
