/**
* File: speechRecognizer.cpp
*
* Author: Lee Crippen
* Created: 12/06/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
*/

#include "speechRecognizer.h"

namespace Anki {
namespace AudioUtil {

void SpeechRecognizer::Start()
{
  StartInternal();
}

void SpeechRecognizer::Stop()
{
  StopInternal();
}

void SpeechRecognizer::DoCallback(const SpeechRecognizerCallbackInfo& info) const
{
  if (_speechCallback)
  {
    _speechCallback(info);
  }
}
  
const std::string SpeechRecognizerCallbackInfo::Description() const
{
  const auto desc = result + " StartTime_ms: " + std::to_string(startTime_ms) + " EndTime_ms: " +
                    std::to_string(endTime_ms) + " Score: " + std::to_string(score) +
                    " startSample: " + std::to_string(startSampleIndex) +
                    " endSample: "   + std::to_string(endSampleIndex);
  return desc;
}

} // end namespace AudioUtil
} // end namespace Anki
