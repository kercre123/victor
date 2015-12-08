/*
 * File: robotAudioBufferStream.cpp
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

#include "anki/cozmo/basestation/audio/robotAudioBufferStream.h"
#include "clad/types/animationKeyFrames.h"
#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioBufferStream::~RobotAudioBufferStream()
{
  // Delete all key frames
  while ( !_keyFrameQueue.empty() ) {
    Util::SafeDelete( _keyFrameQueue.front() );
    _keyFrameQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBufferStream::PushKeyFrame( AnimKeyFrame::AudioSample* audioKeyFrame )
{
  _lock.lock();
  ASSERT_NAMED( !_isComplete, "Do NOT add key frames after the stream is set to isComplete" );
  _keyFrameQueue.emplace( audioKeyFrame );
  _lock.unlock();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimKeyFrame::AudioSample* RobotAudioBufferStream::PopKeyFrame()
{
  _lock.lock();
  ASSERT_NAMED( !_keyFrameQueue.empty(), "Do Not call this methods if Key Frame Queue is empty" );
  AnimKeyFrame::AudioSample* keyFrame = _keyFrameQueue.front();
  _keyFrameQueue.pop();
  _lock.unlock();
  
  return keyFrame;
}


} // Audio
} // Cozmo
} // Anki
