/*
 * File: robotAudioMessageStream.h
 *
 * Author: Jordan Rivas
 * Created: 12/06/2015
 *
 * Description: A stream is a continuous stream of audio frames provided by the RobotAudioBuffer. The
 *              stream is thread safe to allow messages to be pushed and popped from different threads. The stream takes
 *              responsibility for the messages’s memory when they are pushed into the queue and relinquished ownership
 *              when it is popped.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_RobotAudioMessageStream_H__
#define __Basestation_Audio_RobotAudioMessageStream_H__

#include "anki/cozmo/basestation/audio/audioDataTypes.h"
#include "util/helpers/noncopyable.h"
#include <stdio.h>
#include <queue>
#include <mutex>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
class RobotAudioMessageStream : Util::noncopyable  {
  
public:
  
  // Delete message left in queue
  ~RobotAudioMessageStream();
  
  //  Push Robot Audio Message into buffer stream, this will take ownership of message's memory
  void PushRobotAudioFrame( AudioFrameData* audioFrame );
  
  //  Pop Robot Audio Message out of buffer stream, this will release relinquished of message's memory to caller.
  AudioFrameData* PopRobotAudioFrame();
  
  // Check if Robot Audio Message frames are available
  bool HasAudioFrame() const { return !_audioFrameQueue.empty(); }
  
  // Get Robot Audio Frame count
  size_t AudioFrameCount() const { return _audioFrameQueue.size(); }

  // This is called by the RobotAudioBuffer after all the message for this stream have been pushed
  void SetIsComplete() { _isComplete = true; }

  // Check if the stream has received all it's audio messages
  bool IsComplete() const { return _isComplete; }
  
  // Set created time when the stream begins to get audio frames
  void SetCreatedTime_ms( double time_ms ) { _createdTime_ms = time_ms; }
  
  // Get created time
  double GetCreatedTime_ms() const { return _createdTime_ms; }
  
private:
  
  bool _isComplete = false;
  double _createdTime_ms = 0.0;
  using AudioFrameQueueType = std::queue< AudioFrameData* >;
  AudioFrameQueueType _audioFrameQueue;
  std::mutex _lock;
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_RobotAudioMessageStream_H__ */
