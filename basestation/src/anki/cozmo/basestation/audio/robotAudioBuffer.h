/*
 * File: robotAudioBuffer.h
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This consists of a circular buffer to cache the audio samples from the Cozmo Plugin update. When there
 *              is enough cached the data packed into a EngineToRobot audio sample message and is pushed into a
 *              RobotAudioMessageStream. The RobotAudioMessageStreams are stored in a FIFO queue until they are ready
 *              to be sent to the robot.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioBuffer_H__
#define __Basestation_Audio_RobotAudioBuffer_H__

#include "anki/cozmo/basestation/audio/robotAudioMessageStream.h"
#include <util/helpers/templateHelpers.h>
#include <util/container/circularBuffer.h>
#include <util/dispatchQueue/dispatchQueue.h>
#include <stdint.h>
#include <stdio.h>
#include <queue>
#include <mutex>

#define DEBUG_ROBOT_ANIMATION_AUDIO 0
#define DEBUG_ROBOT_ANIMATION_AUDIO_LEVEL 0

namespace Anki {
namespace Cozmo {
  
namespace AnimKeyFrame {
  struct AudioSample;
}
  
namespace Audio {
  

class RobotAudioBuffer
{
  
public:
  
  /*****************************************
   * Plug-in callback methods
   */
  
  // This called when the plug-in is created
  void PrepareAudioBuffer();
  
  // Write samples to buffer
  void UpdateBuffer( const uint8_t* samples, size_t sampleCount );
  
  // This is called when the plug-in is terminated. It will flush the remaining audio samples out of the cache
  void ClearCache();
  

  /*****************************************
   * Audio Client methods
   */

  bool IsActive() const { return _isActive; }
  
  bool HasAudioBufferStream() const { return _streamQueue.size() > 0; }
  
  // Get the front / top Audio Buffer stream in the queue
  RobotAudioMessageStream* GetFrontAudioBufferStream();
  
  // Pop the front  / top Audio buffer stream in the queue
  void PopAudioBufferStream() { _streamQueue.pop(); }
  
  // Clear the Audio buffer stream queue
  void ClearBufferStreams();
  
  // Begin reseting the audio buffer. The buffer will ignore update buffer calls and wait for the audio controller
  // to clear cache
  void ResetAudioBuffer();
  
  // Check if the buffer is in the reset audio buffer state
  bool IsWaitingForReset() const { return _isWaitingForReset; }
  
private:
  
  // A queue of robot audio messages (continuous audio data)
  std::queue< RobotAudioMessageStream > _streamQueue;
  
  // Track what stream is in uses
  RobotAudioMessageStream* _currentStream = nullptr;
  
  bool _isActive = false;
  
  // Flag to identify we are waiting for current update buffer session to complete
  bool _isWaitingForReset = false;
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
