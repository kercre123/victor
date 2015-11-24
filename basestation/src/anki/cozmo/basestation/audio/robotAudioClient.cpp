/*
 * File: robotAudioClient.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Client handles the Robot’s specific audio needs. It is a sub-class of AudioEngineClient.
 *
 * Copyright: Anki, Inc. 2015
 */

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "clad/audio/messageAudioClient.h"

#include <util/helpers/templateHelpers.h>
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotAudioClient::RobotAudioClient( AudioEngineMessageHandler& messageHandler, RobotAudioBuffer& audioBuffer ) :
  AudioEngineClient( messageHandler ),
  _audioBuffer( audioBuffer )
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineClient::CallbackIdType RobotAudioClient::PostCozmoEvent( EventType event, AudioCallbackFlag callbackFlag )
{
  // Track Event
  // Always get callbacks for Cozmo events
  
  // FIXME: Don't think I need this any more??  - JMR
  const AudioCallbackFlag flags = (AudioCallbackFlag) ((uint8_t)callbackFlag | (uint8_t)AudioCallbackFlag::EventComplete);
  const CallbackIdType callbackId = PostEvent( event, 0, flags);
  
  _currentEvents.emplace(callbackId, event);
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::IsPlugInActive() const
{
  return _audioBuffer.IsPlugInActive();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::HasKeyFrameAudioSample() const
{
  return _audioBuffer.HasKeyFrameAudioSample();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimKeyFrame::AudioSample&& RobotAudioClient::PopAudioSample() const
{
  return _audioBuffer.PopKeyFrameAudioSample();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackDuration& callbackMsg )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackMarker& callbackMsg )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleCallbackEvent( const AudioCallbackComplete& callbackMsg )
{

}

} // Audio
} // Cozmo
} // Anki
