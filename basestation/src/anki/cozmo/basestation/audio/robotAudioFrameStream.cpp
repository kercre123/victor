/*
 * File: robotAudioFrameStream.cpp
 *
 * Author: Jordan Rivas
 * Created: 12/06/2015
 *
 * Description: A stream is a continuous stream of audio frames provided by the RobotAudioBuffer. The stream is thread
 *              safe to allow messages to be pushed and popped from different threads. The stream takes responsibility
 *              for the messages’s memory when they are pushed into the queue and relinquished ownership when it is
 *              popped.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioFrameStream.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioFrameStream::RobotAudioFrameStream( double createdTime_ms )
: _createdTime_ms( createdTime_ms )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioFrameStream::~RobotAudioFrameStream()
{
  // Delete all key frames
  while ( !_audioFrameQueue.empty() ) {
    Util::SafeDelete( _audioFrameQueue.front() );
    _audioFrameQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioFrameStream::PushRobotAudioFrame( AudioFrameData* audioFrame )
{
  std::lock_guard<std::mutex> lock( _lock );
  ASSERT_NAMED( !_isComplete, "Do NOT add key frames after the stream is set to isComplete" );
  _audioFrameQueue.push( audioFrame );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioFrameData* RobotAudioFrameStream::PopRobotAudioFrame()
{
  std::lock_guard<std::mutex> lock( _lock );
  ASSERT_NAMED( !_audioFrameQueue.empty(), "Do Not call this methods if Key Frame Queue is empty" );
  AudioFrameData* audioFrame = nullptr;
  if (!_audioFrameQueue.empty())
  {
    audioFrame = _audioFrameQueue.front();
    _audioFrameQueue.pop();
  }
  
  return audioFrame;
}


} // Audio
} // Cozmo
} // Anki
