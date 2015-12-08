/*
 * File: robotAudioBuffer.h
 *
 * Author: Jordan Rivas
 * Created: 11/13/2015
 *
 * Description: This a circular buffer to hold audio samples from the Cozmo Plugin for the animation stream to pull out.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioBuffer_H__
#define __Basestation_Audio_RobotAudioBuffer_H__

#include "anki/cozmo/basestation/audio/robotAudioBufferStream.h"
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
  RobotAudioBufferStream* GetFrontAudioBufferStream();
  
  // Pop the front  / top Audio buffer stream in the queue
  void PopAudioBufferStream() { _streamQueue.pop(); }
  
  // Clear the Audio buffer stream queue
  void ClearBufferStreams();
  
private:
  
  // There should never be more then 800 samples in the buffer
  static constexpr size_t kAudioSampleBufferSize = 2000;
  
  // Cache Audio samples from PlugIn
  Util::CircularBuffer< uint8_t > _audioSampleCache;
  
  // A queue of audio buffer streams (continuous audio data)
  std::queue< RobotAudioBufferStream > _streamQueue;
  
  // Track what stream audio key frame is being added to
  RobotAudioBufferStream* _currentStream = nullptr;
  
  // Copy Audio Cache samples into Audio Key Frames and store in stream
  size_t CopyAudioSampleCacheToKeyFrameBuffer( size_t size, RobotAudioBufferStream* stream );
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
