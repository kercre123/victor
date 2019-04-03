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
#include "cozmoAnim/animProcessMessages.h"
#include "clad/audio/audioMessageTypes.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include <util/logging/logging.h>

#ifdef USES_CLAD_CPPLITE
#define CLAD(ns) CppLite::Anki::ns
#define CLAD_VECTOR(ns) CppLite::Anki::Vector::ns
#define CLAD_AUDIOENGINE(ns) CppLite::Anki::AudioEngine::ns
#else
#define CLAD(ns) ns
#define CLAD_VECTOR(ns) ns
#define CLAD_AUDIOENGINE(ns) AudioEngine::ns
#endif

namespace Anki {
namespace Vector {
namespace Audio {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioEvent& postAudioEvent ) 
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(postAudioEvent);
}
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::StopAllAudioEvents& stopAllAudioEvents )
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(stopAllAudioEvents);
}
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioGameState& postAudioGameState )
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(postAudioGameState);
}
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioSwitchState& postAudioSwitchState )
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(postAudioSwitchState);
}
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioParameter& postAudioParameter )
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(postAudioParameter);
}
void EngineRobotAudioInput::HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioMusicState& postAudioMusicState )
{
  AudioEngine::Multiplexer::AudioMuxInput::HandleMessage(postAudioMusicState);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackDuration&& callbackMessage ) const
{
  if (!SendAnimToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackDuration");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackMarker&& callbackMessage ) const
{
  if (!SendAnimToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackMarker");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackComplete&& callbackMessage ) const
{
  if (!SendAnimToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackComplete");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EngineRobotAudioInput::PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackError&& callbackMessage ) const
{
  if (!SendAnimToEngine(callbackMessage)) {
    PRINT_NAMED_ERROR("EngineRobotAudioInput.PostCallback", "Failed.SendMessageToEngine.AudioCallbackError");
  }
}

  
} // Audio
} // Cozmo
} // Anki
