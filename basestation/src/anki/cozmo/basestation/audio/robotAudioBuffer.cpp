/*
 * File: robotAudioBuffer.cpp
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

#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"


namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace();
  _currentStream = &_streamQueue.back();
  _currentStream->SetCreatedTime_ms( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() );
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
  audioFrame->CopySamples( samples, sampleCount );
  _currentStream->PushRobotAudioFrame( audioFrame );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::CloseAudioBuffer()
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
RobotAudioFrameStream* RobotAudioBuffer::GetFrontAudioBufferStream()
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

  
} // Audio
} // Cozmo
} // Anki
