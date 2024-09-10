/*
 * File: audioTypeTranslator.cpp
 *
 * Author: Jordan Rivas
 * Created: 9/29/17
 *
 * Description: Helper functions to properly convert CLAD and standard types to AudioEngine types
 *
 * Copyright: Anki, Inc. 2017
 */

#include "audioEngine/audioTypeTranslator.h"

namespace Anki {
namespace AudioEngine {

// Translate AudioMetaData Clad types to AudioEngine Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioGameObject ToAudioGameObject(AudioMetaData::GameObjectType gameObject)
{
  return static_cast<AudioGameObject>(gameObject);
}

AudioEventId ToAudioEventId(AudioMetaData::GameEvent::GenericEvent event)
{
  return Anki::Util::numeric_cast<AudioEventId>(event);
}

AudioParameterId ToAudioParameterId(AudioMetaData::GameParameter::ParameterType parameter)
{
  return Anki::Util::numeric_cast<AudioParameterId>( parameter);
}

AudioSwitchGroupId ToAudioSwitchGroupId(AudioMetaData::SwitchState::SwitchGroupType switchGroup)
{
  return Anki::Util::numeric_cast<AudioStateGroupId>(switchGroup);
}

AudioSwitchStateId ToAudioSwitchStateId(AudioMetaData::SwitchState::GenericSwitch switchState)
{
  return Anki::Util::numeric_cast<AudioSwitchStateId>(switchState);
}

AudioStateGroupId ToAudioStateGroupId(AudioMetaData::GameState::StateGroupType stateGroup)
{
  return Anki::Util::numeric_cast<AudioStateGroupId>(stateGroup);
}

AudioStateId ToAudioStateId(AudioMetaData::GameState::GenericState state)
{
  return Anki::Util::numeric_cast<AudioStateId>(state);
}

// Translate standard types to AudioEngine Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioRTPCValue ToAudioRTPCValue(float value)
{
  return Util::numeric_cast<AudioRTPCValue>(value);
}

AudioTimeMs ToAudioTimeMs(int32_t timeMs)
{
  return Util::numeric_cast<AudioTimeMs>( timeMs );
}
  
// Translate Multiplexer types to AudioEngine Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioCurveType ToAudioCurveType( Multiplexer::CurveType curve )
{
  using CT = Multiplexer::CurveType;
  AudioCurveType audioCurve = AudioCurveType::Linear;
  // Translate Curve Enum types
  switch( curve ) {
    case CT::Linear:
      audioCurve = AudioCurveType::Linear;
      break;
    case CT::SCurve:
      audioCurve = AudioCurveType::SCurve;
      break;
    case CT::InversedSCurve:
      audioCurve = AudioCurveType::InversedSCurve;
      break;
    case CT::Sine:
      audioCurve = AudioCurveType::Sine;
      break;
    case CT::SineReciprocal:
      audioCurve = AudioCurveType::SineReciprocal;
      break;
    case CT::Exp1:
      audioCurve = AudioCurveType::Exp1;
      break;
    case CT::Exp3:
      audioCurve = AudioCurveType::Exp3;
      break;
    case CT::Log1:
      audioCurve = AudioCurveType::Log1;
      break;
    case CT::Log3:
      audioCurve = AudioCurveType::Log3;
      break;
  }
  return audioCurve;
}

}
}
