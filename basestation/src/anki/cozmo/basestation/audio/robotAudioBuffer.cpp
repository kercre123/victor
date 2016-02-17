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
#include "clad/robotInterface/messageEngineToRobot.h"
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::PrepareAudioBuffer()
{
  // Prep new Continuous Stream Buffer
  _streamQueue.emplace();
  _currentStream = &_streamQueue.back();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::UpdateBuffer( const uint8_t* samples, size_t sampleCount )
{
  // Create Robot AnkiKey Frame AudioSample struct
  ASSERT_NAMED( sampleCount <= static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ), ("RobotAudioBuffer.UpdateBuffer buffer is too big!"+ std::to_string(sampleCount) + " > " + std::to_string(static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ))).c_str() );
  
  // Create Audio Frame
  AnimKeyFrame::AudioSample audioSample = AnimKeyFrame::AudioSample();
  ASSERT_NAMED(static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) <= audioSample.Size(), "Block size must be less or equal to audioSameple size");
  // Copy samples into audioSample
  memcpy(audioSample.sample.data(), samples, sampleCount * sizeof(uint8_t));
  
  
  // Pad the back of the buffer with 0s
  // This should only apply to the last frame
  if (sampleCount < static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE )) {
    PRINT_NAMED_DEBUG("RobotAudioBuffer.UpdateBuffer", "Pad audioSample remaining bytes - current size %zu of %d", sampleCount,AnimConstants::AUDIO_SAMPLE_SIZE);
    std::fill( audioSample.sample.begin() + sampleCount, audioSample.sample.end(), 0 );
  }
  
  ASSERT_NAMED( nullptr!= _currentStream, "Must pass a Robot Audio Buffer Stream object" );
  RobotInterface::EngineToRobot* audioMsg = new RobotInterface::EngineToRobot( std::move( audioSample ) );
  
  PRINT_NAMED_DEBUG("RobotAudioBuffer.UpdateBuffer", "PushRobotAudioMessage - AudioSample size: %zu", audioSample.Size());
  _currentStream->PushRobotAudioMessage( audioMsg );
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
  while (!_streamQueue.empty()) {
    _streamQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioBuffer::ClearCache()
{
  // No more samples to cache, create final Audio Message
  _currentStream->SetIsComplete();
  _currentStream = nullptr;
}

  
} // Audio
} // Cozmo
} // Anki
