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

#include <util/helpers/templateHelpers.h>
#include <util/container/circularBuffer.h>
#include <util/dispatchQueue/dispatchQueue.h>
#include <stdint.h>
#include <stdio.h>
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
  
  
  RobotAudioBuffer();
  ~RobotAudioBuffer();
  
  // Write samples to buffer
  void UpdateBuffer( const uint8_t* samples, size_t sampleCount );
  
  // True if Plug-In is streaming audio data
  bool IsPlugInActive() const { return _plugInIsActive; }
  
  // True if key frame audio sample messages are available
  bool HasKeyFrameAudioSample() const { return (_KeyFrameAudioSampleBuffer.size() > 0); }
  
  // Pop the front key frame audio sample message
  // Note: Audio Sample pointer memory needs to be manage or it will leak memory.
  AnimKeyFrame::AudioSample* PopKeyFrameAudioSample();
  
  // This is called when the plug-in is terminated. It will flush the remaining audio samples out of the cache
  void ClearCache();
  
  
private:
  
  static constexpr size_t kAudioSampleBufferSize = 2400;
  static constexpr size_t kKeyFrameAudioSampleBufferSize = 100;
  
  Util::CircularBuffer< uint8_t > _audioSampleCache;
  Util::CircularBuffer< AnimKeyFrame::AudioSample* > _KeyFrameAudioSampleBuffer;
    
  std::mutex _KeyFrameAudioSampleBufferLock;
  
  bool _plugInIsActive  = false;      // Plug-In is updating data
  bool _clearCache      = false;      // Plug-In has been terminated
  
  
  size_t CopyAudioSampleCachToKeyFrameAudioSample( size_t size );
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBuffer_H__ */
