/**
* File: audioFileReader.cpp
*
* Author: Lee Crippen
* Created: 2/6/2017
*
* Description: Stub Android implementation of interface.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioFileReader.h"

#include <iostream>

namespace Anki {
namespace AudioUtil {


struct AudioFileReader::NativeAudioFileData
{
};

bool AudioFileReader::ReadFile(const std::string& audioFilePath)
{
  ClearAudio();
  
  return true;
}

bool AudioFileReader::GetNativeFileData(NativeAudioFileData& fileData, const std::string& audioFilePath)
{
  
  return true;
}

bool AudioFileReader::ConvertAndStoreSamples(const NativeAudioFileData& fileData)
{
  
  return true;
}

bool AudioFileReader::TrimPrimingAndRemainder(const NativeAudioFileData& fileData)
{

  return true;
}

void AudioFileReader::DeliverAudio(bool doRealTime, bool addBeginSilence, bool addEndSilence)
{
  
}
} // namespace AudioUtil
} // namespace Anki
