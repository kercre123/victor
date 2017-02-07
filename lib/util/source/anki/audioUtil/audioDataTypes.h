/**
* File: audioDataTypes.h
*
* Author: Lee Crippen
* Created: 12/13/2016
*
* Description: Some often reused audio defines and types.
*
* Copyright: Anki, Inc. 2016
*
*/

#ifndef __Anki_AudioUtil_AudioDataTypes_H_
#define __Anki_AudioUtil_AudioDataTypes_H_

#include <cstdint>
#include <deque>
#include <vector>

// Fwd delcared mac struct type to be filled out in the .mm file for iOS and OSX
struct AudioStreamBasicDescription;

namespace Anki {
namespace AudioUtil {
  
using AudioSample = int16_t;
using AudioChunk = std::vector<AudioSample>;
using AudioChunkList = std::deque<AudioChunk>;
  
constexpr uint32_t kSampleRate_hz = 16000;
constexpr uint32_t kTimePerAudioChunk_ms = 125;
constexpr uint32_t kSamplesPerChunk = kTimePerAudioChunk_ms * kSampleRate_hz / 1000;
constexpr uint32_t kBytesPerChunk = kSamplesPerChunk * sizeof(AudioSample);

void GetStandardAudioDescriptionFormat(AudioStreamBasicDescription& description_out);

} // end namespace AudioUtil
} // end namespace Anki


#endif //__Anki_AudioUtil_AudioDataTypes_H_
