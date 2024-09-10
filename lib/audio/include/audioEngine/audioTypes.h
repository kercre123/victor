/***********************************************************************************************************************
 *
 *  Audio Types
 *  Audio
 *
 *  Created by Jarrod Hatfield on 1/29/15.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Interface that encapsulates the underlying sound engine
 *  - Hides all engine specific details behind this interface
 *  - Most types mirror those of Wwise's AkTypes.h, therefore don't change without confirming
 *
 ***********************************************************************************************************************/

#ifndef __AnkiAudio_AudioTypes_H__
#define __AnkiAudio_AudioTypes_H__

#include <stdint.h>
#include <functional>
#include <string>


namespace Anki {
namespace AudioEngine {

using AudioUInt16 = uint16_t;
using AudioUInt32 = uint32_t;
using AudioReal32 = float;

// Unique identifier for each object in the game the emits sound
using AudioGameObject = uint64_t;
enum { kInvalidAudioGameObject = (AudioGameObject)-1 };

// Unique ID to represent every audio event that can be posted to the audio engine
using AudioEventId = uint32_t;
enum { kInvalidAudioEventId = 0 };

// Unique Id to represent every game parameter that can be posted to the audio engine
using AudioParameterId = uint32_t;
enum { kInvalidAudioParameterId = 0 };

// RTPC value Type
using AudioRTPCValue = float;

// Unique Id to represent every switch group that can be posted to the audio engine
using AudioSwitchGroupId = uint32_t;
enum { kInvalidAudioSwitchGroupId = 0 };

// Unique Id to represent every switch state that can be posted to the audio engine
using AudioSwitchStateId = uint32_t;
enum { kInvalidAudioSwitchStateId = 0 };

// Unique Id to represent every state group that can be posted to the audio engine
using AudioStateGroupId = uint32_t;
enum { kInvalidAudioStateGroupId = 0 };

// Unique Id to represent every state that can be posted to the audio engine
using AudioStateId = uint32_t;
enum { kInvalidAudioStateId = 0 };

// Unique identifier for each event that has been posted
using AudioPlayingId = uint32_t;
enum { kInvalidAudioPlayingId = 0 };

// Unique identifier for Aux Bus
using AudioAuxBusId = uint32_t;
enum { kInvalidAudioAuxBusId = 0 };

// Aux Bus send information per game object
struct AudioAuxBusValue {
  AudioAuxBusId auxBusId;
  AudioReal32   controlValue; // Send level value [0.0 : 1.0]
  AudioAuxBusValue( AudioAuxBusId auxBusId = kInvalidAudioAuxBusId,
                    AudioReal32 controlValue = 0.0f)
  : auxBusId( auxBusId )
  , controlValue( controlValue ) {}
};
  
// Unique Id to represent every sound bank that can be posted to the audio engine
using AudioBankId = uint32_t;
enum { kInvalidAudioBankId = 0 };

// RTPC Value types
enum AudioRTPCValueType {
  Default = 0,
  Global,
  GameObject,
  PlayingId,
  Unavailable
};

// Game sync grounp
enum AudioGroupType {
  AudioGroupTypeSwitch = 0,
  AudioGroupTypeState
};

// Time in milliseconds
using AudioTimeMs = int32_t;
  
// Plug Identifier
using AudioPluginId = uint32_t;

// Simple 3D vector (no use for w)
struct AudioVector
{
  float x;
  float y;
  float z;
};

enum class AudioCurveType : uint8_t
{
  Linear = 0,       // Linear (Default)
  SCurve,           // S Curve
  InversedSCurve,   // Inversed S Curve
  Sine,             // Sine Curve
  SineReciprocal,   // Reciprocal of Sine Curve
  Exp1,             // Exponential 1 Curve
  Exp3,             // Exponential 3 Curve
  Log1,             // Logarithm 1 Curve
  Log3              // Logarithm 3 Curve
};

} // AudioEngine
} // Anki

#endif // __AnkiAudio_AudioTypes_H__
