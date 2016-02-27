/*
 * File: robotAudioMessageStream.cpp
 *
 * Author: Jordan Rivas
 * Created: 12/06/2015
 *
 * Description: A stream is a continuous stream of EngineToRobot AudioSamples provided by the RobotAudioBuffer. The
 *              stream is thread safe to allow messages to be pushed and popped from different threads. The stream takes
 *              responsibility for the messages’s memory when they are pushed into the queue and relinquished ownership
 *              when it is popped.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioMessageStream.h"
#include "clad/types/animationKeyFrames.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioMessageStream::~RobotAudioMessageStream()
{
  // Delete all key frames
  while ( !_messageQueue.empty() ) {
    Util::SafeDelete( _messageQueue.front() );
    _messageQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioMessageStream::PushRobotAudioMessage( RobotInterface::EngineToRobot* audioMsg )
{
  std::lock_guard<std::mutex> lock( _lock );
  ASSERT_NAMED( !_isComplete, "Do NOT add key frames after the stream is set to isComplete" );
  _messageQueue.push( audioMsg );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotInterface::EngineToRobot* RobotAudioMessageStream::PopRobotAudioMessage()
{
  std::lock_guard<std::mutex> lock( _lock );
  ASSERT_NAMED( !_messageQueue.empty(), "Do Not call this methods if Key Frame Queue is empty" );
  RobotInterface::EngineToRobot* audioMsg = _messageQueue.front();
  _messageQueue.pop();
  
  return audioMsg;
}


} // Audio
} // Cozmo
} // Anki
