/*
 * File: audioCladMessageHelper.h
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


#ifndef __AnkiAudio_AudioCladMessageHelper_H__
#define __AnkiAudio_AudioCladMessageHelper_H__

#include "clad/audio/audioMessage.h"

namespace Anki {
namespace AudioEngine {
namespace Multiplexer {
namespace CladMessageHelper {

PostAudioEvent CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent event,
                                     AudioMetaData::GameObjectType gameObject,
                                     uint16_t callbackId );

StopAllAudioEvents CreateStopAllAudioEvents( AudioMetaData::GameObjectType gameObject );

PostAudioGameState CreatePostAudioGameState( AudioMetaData::GameState::StateGroupType gameStateGroup,
                                             AudioMetaData::GameState::GenericState gameState );

PostAudioSwitchState CreatePostAudioSwitchState( AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                                 AudioMetaData::SwitchState::GenericSwitch switchState,
                                                 AudioMetaData::GameObjectType gameObject );

PostAudioParameter CreatePostAudioParameter( AudioMetaData::GameParameter::ParameterType parameter,
                                             float parameterValue,
                                             AudioMetaData::GameObjectType gameObject,
                                             int32_t timeInMilliSeconds,
                                             CurveType curve );
} // CladMessageHelper
} // Multiplexer
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_AudioCladMessageHelper_H__ */
