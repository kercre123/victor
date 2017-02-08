/**
* File: audioFileReader.h
*
* Author: Lee Crippen
* Created: 11/22/16
*
* Description: Platform independent interface for accessing a compressed audio file's data in
* a "standard" format (16 bit signed PCM at 16kHz).
*
* Copyright: Anki, Inc. 2016
*
*/

#ifndef __Anki_AudioUtil_AudioFileReader_H_
#define __Anki_AudioUtil_AudioFileReader_H_

#include "audioDataTypes.h"

#include <string>

namespace Anki {
namespace AudioUtil {

class AudioFileReader {
public:
  bool ReadFile(const std::string& audioFilePath);
  const AudioChunkList& GetAudioSamples() const { return _audioSamples; }
  void ClearAudio() { _audioSamples.clear(); }
  
private:
  AudioChunkList  _audioSamples;
  
  struct NativeAudioFileData;
  bool GetNativeFileData(NativeAudioFileData& fileData, const std::string& audioFilePath);
  bool ConvertAndStoreSamples(const NativeAudioFileData& fileData);
  bool TrimPrimingAndRemainder(const NativeAudioFileData& fileData);
  
}; // class AudioFileReader

  
} // namespace AudioUtil
} // namespace Anki

#endif // __Anki_AudioUtil_AudioFileReader_H_
