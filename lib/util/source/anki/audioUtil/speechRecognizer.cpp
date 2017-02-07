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

void SpeechRecognizer::DoCallback(const char* callbackArg)
{
  if (_speechCallback)
  {
    _speechCallback(callbackArg);
  }
}

} // end namespace AudioUtil
} // end namespace Anki
