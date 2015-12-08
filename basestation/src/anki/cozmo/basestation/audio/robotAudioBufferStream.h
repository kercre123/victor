/*
 * File: robotAudioBufferStream.h
 *
 * Author: Jordan Rivas
 * Created: 12/06/2015
 *
 * Description: A buffer stream is a continuous stream of audio key frames provided by the RobotAudioBuffer. The stream
 *              is thread safe to allow key frames to be pushed and popped from different threads. The stream takes
 *              responsibility for the key frame’s memory when they are pushed into the queue and relinquished ownership
 *              when it is popped.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioBufferStream_H__
#define __Basestation_Audio_RobotAudioBufferStream_H__

#include <util/helpers/noncopyable.h>
#include <stdio.h>
#include <queue>
#include <mutex>


namespace Anki {
namespace Cozmo {

namespace AnimKeyFrame {
  struct AudioSample;
}

namespace Audio {
  
class RobotAudioBufferStream : Util::noncopyable  {
  
public:
  
  // Delete KeyFrames left in queue
  ~RobotAudioBufferStream();
  
  //  Push Audio Key Frames into buffer stream, this will take ownership of key frame's memory
  void PushKeyFrame( AnimKeyFrame::AudioSample* audioKeyFrame );
  
    //  Pop Audio Key Frames out of buffer stream, this will release relinquished of key frame's memory to caller.
  AnimKeyFrame::AudioSample* PopKeyFrame();
  
  // Check if key frames are available
  bool HasKeyFrames() const { return ( _keyFrameQueue.size() > 0 ); }
  
  // Get key frame count
  size_t KeyFrameCount() const { return _keyFrameQueue.size(); }
  
  // Check if the stream has received all it's audio key frames
  bool IsComplete() const { return _isComplete; }
  
  // This is called by the RobotAudioBuffer after all the key frames for this stream have been pushed
  void SetIsComplete() { _isComplete = true; }
  
private:
  
  bool _isComplete = false;
  using KeyFrameQueueType = std::queue< AnimKeyFrame::AudioSample* >;
  KeyFrameQueueType _keyFrameQueue;
  std::mutex _lock;
  
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioBufferStream_H__ */
