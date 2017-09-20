/*
 * File: engineRobotAudioInput.cpp
 *
 * Author: Jordan Rivas
 * Created: 9/12/2017
 *
 * Description: This is a subclass of AudioMuxInput which provides communication between itself and an
 *              EndingRobotAudioClient by means of EngineToRobot and RobotToEngine messages. It's purpose is to perform
 *              audio tasks sent from the engine process to the audio engine in the animation process.
 *
 * Copyright: Anki, Inc. 2017
 */


#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/engineMessages.h"
#include "clad/audio/audioMessageTypes.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include <util/logging/logging.h>


namespace Anki {
namespace Cozmo {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::HandleEngineToRobotMsg(RobotInterface::EngineToRobot& msg)
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
void EngineRobotAudioInput::PostCallback( AudioEngine::Multiplexer::AudioCallbackDuration&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackDuration");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( AudioEngine::Multiplexer::AudioCallbackMarker&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackMarker");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( AudioEngine::Multiplexer::AudioCallbackComplete&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackComplete");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( AudioEngine::Multiplexer::AudioCallbackError&& callbackMessage ) const
{
  if (!RobotInterface::SendMessageToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackError");
  }
}

  
} // Audio
} // Cozmo
} // Anki
