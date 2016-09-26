/*
 * File: robotAudioBuffer.h
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
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
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioBuffer_H__
#define __Basestation_Audio_RobotAudioBuffer_H__

#include "anki/cozmo/basestation/audio/robotAudioFrameStream.h"
#include "anki/cozmo/basestation/audio/audioDataTypes.h"
#include "util/helpers/templateHelpers.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include <stdint.h>
#include <stdio.h>
#include <queue>
#include <mutex>

#define DEBUG_ROBOT_ANIMATION_AUDIO 0
#define DEBUG_ROBOT_ANIMATION_AUDIO_LEVEL 0

namespace Anki {
namespace Cozmo {
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
  void UpdateBuffer( const AudioSample* samples, const size_t sampleCount );
  
  // This is called when the plug-in is terminated.
  void CloseAudioBuffer();

  /*****************************************
   * Audio Client methods
   */

  // Check if the buffer is currently active
  bool IsActive() const { return _isActive; }

  // Check if the buffer is in the reset audio buffer state
  bool IsWaitingForReset() const { return _isWaitingForReset; }

  // Check if buffer stream has an audio streams
  bool HasAudioBufferStream() const;
  
  // Get the front / top Audio Buffer stream in the queue
  RobotAudioFrameStream* GetFrontAudioBufferStream();
  
  // Pop the front / top Audio buffer stream in the queue
  void PopAudioBufferStream();
  
  // Clear the Audio buffer stream queue
  void ClearBufferStreams();
  
  // Begin reseting the audio buffer. The buffer will ignore update buffer calls and wait for the audio controller
  // to clear cache
  void ResetAudioBufferAnimationCompleted( bool completed );
  

protected:
  
  // FIXME: Think there is a bug by not locking the _streamQueue. Both Audio thread and Engine thread use the queue
  // Maybe we should use producer consumer queue here!?
  
  // A queue of robot audio frames (continuous audio data)
  std::queue< RobotAudioFrameStream > _streamQueue;
  
  // Stream queue mutex
  mutable std::mutex _lock;
  
  // Track if the Audio Engine is providing data to stream
  bool _isActive = false;
  
  // Flag to identify we are waiting for current update buffer session to complete
  bool _isWaitingForReset = false;
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
