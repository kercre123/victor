/*
 * File: robotAudioBuffer.cpp
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

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace();
  _currentStream = &_streamQueue.back();
  _isActive = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const AudioSample* samples, const size_t sampleCount )
{
  // Ignore updates if we are waiting for the plug-in to reset
  if ( _isWaitingForReset ) {
    if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
      PRINT_NAMED_WARNING("RobotAudioBuffer.UpdateBuffer", "Ignore buffer update!");
    }
    return;
  }
  
  // Copy audio samples into frame & push it into the queue
  AudioFrameData *audioFrame = new AudioFrameData( sampleCount );
  audioFrame->CopySamples(samples);
  _currentStream->PushRobotAudioFrame( audioFrame );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioMessageStream* RobotAudioBuffer::GetFrontAudioBufferStream()
{
  ASSERT_NAMED( !_streamQueue.empty(), "Must check if a Robot Audio Buffer Stream is in Queue befor calling this method") ;
  
  return &_streamQueue.front();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearBufferStreams()
{
  while ( !_streamQueue.empty() ) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ResetAudioBuffer()
{
  if ( _currentStream != nullptr ) {
    _isWaitingForReset = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearCache()
{
  if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
    PRINT_NAMED_WARNING("RobotAudioBuffer.ClearCache", "CLEAR!");
  }
  
  // No more samples to cache, create final Audio Message
  _currentStream->SetIsComplete();
  _currentStream = nullptr;
  _isActive = false;
  _isWaitingForReset = false;
}

  
} // Audio
} // Cozmo
} // Anki
