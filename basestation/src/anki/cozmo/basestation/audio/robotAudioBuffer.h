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


namespace Anki {
namespace Cozmo {
  
namespace AnimKeyFrame {
  struct AudioSample;
}
  
namespace Audio {
  

class RobotAudioBuffer
{
  
public:
  
  // Default constructor
  RobotAudioBuffer();
  
  // This called when the plug-in is created
  void PrepareAudioBuffer();
  
  // Write samples to buffer
  void UpdateBuffer( const uint8_t* samples, size_t sampleCount );
  
  // This is called when the plug-in is terminated. It will flush the remaining audio samples out of the cache
  void ClearCache();
  
  // Audio Client Methods
  bool HasAudioBufferStream() { return _streamQueue.size() > 0; }
  
  // Get the front / top Audio Buffer stream in the queue
  RobotAudioMessageStream* GetFrontAudioBufferStream();
  
  // Pop the front  / top Audio buffer stream in the queue
  void PopAudioBufferStream() { _streamQueue.pop(); }
  
  // Clear the Audio buffer stream queue
  void ClearBufferStreams();
  
private:
  
  // There should never be more then 800 samples in the buffer
  static constexpr size_t kAudioSampleBufferSize = 2000;
  
  // Cache Audio samples from PlugIn
  Util::CircularBuffer< uint8_t > _audioSampleCache;
  
  // A queue of robot audio messages (continuous audio data)
  std::queue< RobotAudioMessageStream > _streamQueue;
  
  // Track what stream is in uses
  RobotAudioMessageStream* _currentStream = nullptr;
  
  // Copy Audio Cache samples into Robot Audio Message and store in stream
  size_t CopyAudioSampleCacheToRobotAudioMessage( size_t size, RobotAudioMessageStream* stream );
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
