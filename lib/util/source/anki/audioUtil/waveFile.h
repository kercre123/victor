/**
* File: waveFile.h
*
* Author: Lee Crippen
* Created: 07/03/17
*
* Description: Simple wave file saving functionality.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Anki_AudioUtil_WaveFile_H_
#define __Anki_AudioUtil_WaveFile_H_

#include "audioUtil/audioDataTypes.h"

#include <string>

namespace Anki {
namespace AudioUtil {

class WaveFile {
public:
  static bool SaveFile(const std::string& filename,
                       const AudioChunkList& chunkList,
                       uint16_t numChannels = 1,
                       uint32_t sampleRate_hz = AudioUtil::kSampleRate_hz);
  
  static AudioChunkList ReadFile(const std::string& filename, std::size_t desiredSamplesPerChunk = kSamplesPerChunk);
  
}; // class WaveFile

  
} // namespace AudioUtil
} // namespace Anki

#endif // __Anki_AudioUtil_WaveFile_H_
