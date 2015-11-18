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
  // Allways get callbacks for Cozmo events
  const AudioCallbackFlag flags = (AudioCallbackFlag) ((uint8_t)callbackFlag | (uint8_t)AudioCallbackFlag::EventComplete);
  const CallbackIdType callbackId = PostEvent( event, 0, flags);
  
  _currentEvents.emplace(callbackId, event);
  return callbackId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t RobotAudioClient::GetSoundSample( uint8_t* outArray, uint32_t requestedSize )
{

  if ( requestedSize > _audioBuffer.GetBufferSize() ) {
    if ( 0 == _audioBuffer.GetEmptyUpdateCount() ) {
      PRINT_NAMED_INFO("RobotAudioClient::GetSoundSample", "Not Enough bytes %zu", _audioBuffer.GetBufferSize() );
      return 0;
    }
    PRINT_NAMED_INFO("RobotAudioClient::GetSoundSample", " End of event bytes left %zu -- EmptyUpdateCount: %d", _audioBuffer.GetBufferSize(), _audioBuffer.GetEmptyUpdateCount() );
    _isStreaming = false;
  }
  
  
  uint32_t returnSize = (uint32_t)_audioBuffer.GetAudioSamples(outArray, requestedSize);
  
//  PRINT_NAMED_INFO("RobotAudioClient::GetSoundSample", "returnSize %d  - Post bufferSize %zu", returnSize, _audioBuffer.GetBufferSize());
  
  return returnSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RobotAudioClient::IsReadyToStartAudioStream()
{
  if (_audioBuffer.GetBufferSize() >= _preBufferSize) {
    _isStreaming = true;
  }
  
  return _isStreaming;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t RobotAudioClient::GetBufferSize() const
{
  return (uint32_t)_audioBuffer.GetBufferSize();
}


// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RobotAudioClient::HandleEvents(const AnkiEvent<MessageAudioClient>& event)
{
  PRINT_NAMED_INFO("RobotAudioClient.HandleEvents",
                   "Handle game event of type %s !",
                   MessageAudioClientTagToString(event.GetData().GetTag()) );
  
  switch ( event.GetData().GetTag() ) {
      
    case MessageAudioClientTag::AudioCallbackDuration:
    {
      // Handle Duration Callback
      const AudioCallbackDuration& audioMsg = event.GetData().Get_AudioCallbackDuration();
      printf("\n\nCallbackId: %d\n\n", audioMsg.callbackId);
    }
      break;
      
    case MessageAudioClientTag::AudioCallbackMarker:
    {
      // Handle Marker Callback
      const AudioCallbackMarker& audioMsg = event.GetData().Get_AudioCallbackMarker();
      printf("\n\nCallbackId: %d\n\n", audioMsg.callbackId);
    }
      break;
      
    case MessageAudioClientTag::AudioCallbackComplete:
    {
      // Handle Complete Callback
      const AudioCallbackComplete& audioMsg = event.GetData().Get_AudioCallbackComplete();
      printf("\n\nCallbackId: %d\n\n", audioMsg.callbackId);
    }
      break;
      
    default:
    {
      PRINT_NAMED_ERROR( "HandleEvents.HandleEvents",
                         "Subscribed to unhandled event of type %s !",
                         MessageAudioClientTagToString(event.GetData().GetTag()) );
    }
  }
}

} // Audio
} // Cozmo
} // Anki
