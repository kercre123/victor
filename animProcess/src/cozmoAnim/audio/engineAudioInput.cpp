//
//  engineAudioInput.cpp
//  cozmoEngine2
//
//  Created by Jordan Rivas on 9/12/17.
//
//

#include "cozmoAnim/audio/engineAudioInput.h"
#include "cozmoAnim/engineMessages.h"
#include "clad/audio/audioMessageTypes.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineAudioInput::HandleEngineToRobotMsg(RobotInterface::EngineToRobot& msg)
{
  PRINT_CH_DEBUG(AudioMuxInput::kAudioLogChannel,
                 "AudioUnityInput.HandleGameEvents", "Handle game event of type %X",
                 msg.tag);

  switch ( msg.tag ) {
      
    case RobotInterface::EngineToRobot::Tag_postAudioEvent:
      HandleMessage( msg.postAudioEvent );
      break;
      
    case RobotInterface::EngineToRobot::Tag_stopAllAudioEvents:
      HandleMessage( msg.stopAllAudioEvents );
      break;
    
    case RobotInterface::EngineToRobot::Tag_postAudioGameState:
            HandleMessage( msg.postAudioGameState );
      break;
      
    case RobotInterface::EngineToRobot::Tag_postAudioSwitchState:
      HandleMessage( msg.postAudioSwitchState );
      break;
      
    case RobotInterface::EngineToRobot::Tag_postAudioParameter:
      HandleMessage( msg.postAudioParameter );
      break;

    default:
      // Do nothing
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineAudioInput::PostCallback( const AudioEngine::Multiplexer::AudioCallbackDuration&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackDuration");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineAudioInput::PostCallback( const AudioEngine::Multiplexer::AudioCallbackMarker&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackMarker");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineAudioInput::PostCallback( const AudioEngine::Multiplexer::AudioCallbackComplete&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackComplete");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineAudioInput::PostCallback( const AudioEngine::Multiplexer::AudioCallbackError&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackError");
  }
}

  
} // Audio
} // Cozmo
} // Anki
