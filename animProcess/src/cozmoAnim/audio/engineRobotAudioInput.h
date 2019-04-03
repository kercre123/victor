/*
 * File: engineRobotAudioInput.h
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

#ifndef __Cozmo_Anim_Audio_EngineRobotAudioInput_H__
#define __Cozmo_Anim_Audio_EngineRobotAudioInput_H__


#include "audioEngine/multiplexer/audioMuxInput.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#ifdef USES_CLAD_CPPLITE
#define CLAD_AUDIOENGINE(ns) CppLite::Anki::AudioEngine::ns
#else
#define CLAD_AUDIOENGINE(ns) AudioEngine::ns
#endif

namespace Anki {
namespace Vector {
namespace Audio {

class EngineRobotAudioInput : public AudioEngine::Multiplexer::AudioMuxInput {
  
public:
  
  virtual void PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackDuration&& callbackMessage ) const override;
  virtual void PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackMarker&& callbackMessage ) const override;
  virtual void PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackComplete&& callbackMessage ) const override;
  virtual void PostCallback( CLAD_AUDIOENGINE(Multiplexer)::AudioCallbackError&& callbackMessage ) const override;
  
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioEvent& eventMessage ) override;
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::StopAllAudioEvents& stopEventMessage ) override;
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioGameState& gameStateMessage ) override;
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioSwitchState& switchStateMessage ) override;
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioParameter& parameterMessage ) override;
  virtual void HandleMessage( const CLAD_AUDIOENGINE(Multiplexer)::PostAudioMusicState& musicStateMessage ) override;
};


} // Audio
} // Cozmo
} // Anki

#undef CLAD

#endif /* __Cozmo_Anim_Audio_EngineRobotAudioInput_H__ */
