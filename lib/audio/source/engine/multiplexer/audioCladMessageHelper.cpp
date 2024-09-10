/*
 * File: audioCladMessageHelper.cpp
 *
 * Author: Jordan Rivas
 * Created: 02/20/18
 *
 * Description: Create Audio CLAD struct helper functions. Since the Audio messages can be
 *              generated for C++ Lite the variables need to be in a specific order to properly
 *              align the bytes. These functions abstract that issue to allow the parameters to be
 *              in the expected order.
 *
 * Copyright: Anki, Inc. 2018
 */

#include "audioEngine/multiplexer/audioCladMessageHelper.h"

namespace Anki {
namespace AudioEngine {
namespace Multiplexer {
namespace CladMessageHelper {

PostAudioEvent CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent event,
                                     AudioMetaData::GameObjectType gameObject,
                                     uint16_t callbackId )
{
  return PostAudioEvent( gameObject, event, callbackId, 0 );
}

StopAllAudioEvents CreateStopAllAudioEvents( AudioMetaData::GameObjectType gameObject )
{
  return StopAllAudioEvents( gameObject );
}

PostAudioGameState CreatePostAudioGameState( AudioMetaData::GameState::StateGroupType gameStateGroup,
                                             AudioMetaData::GameState::GenericState gameState )
{
  return PostAudioGameState( gameStateGroup, gameState );
}

PostAudioSwitchState CreatePostAudioSwitchState( AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                                 AudioMetaData::SwitchState::GenericSwitch switchState,
                                                 AudioMetaData::GameObjectType gameObject )
{
  return PostAudioSwitchState( gameObject, switchGroup, switchState );
}

PostAudioParameter CreatePostAudioParameter( AudioMetaData::GameParameter::ParameterType parameter,
                                             float parameterValue,
                                             AudioMetaData::GameObjectType gameObject,
                                             int32_t timeInMilliSeconds,
                                             CurveType curve )
{
  return PostAudioParameter( gameObject, parameter, parameterValue, timeInMilliSeconds, curve );
}

} // CladMessageHelper
} // Multiplexer
} // AudioEngine
} // Anki
