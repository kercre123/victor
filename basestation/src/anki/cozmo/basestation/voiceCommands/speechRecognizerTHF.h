/**
* File: speechRecognizerTHF.h
*
* Author: Lee Crippen
* Created: 12/12/2016
*
* Description: SpeechRecognizer implementation for Sensory TrulyHandsFree.
*
* Copyright: Anki, Inc. 2016
*
*/
#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHF_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHF_H_

#include "audioUtil/speechRecognizer.h"

#include <string>
#include <memory>

namespace Anki {
namespace Cozmo {
  
struct SpeechRecognizerTHFData;

class SpeechRecognizerTHF : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerTHF();
  virtual ~SpeechRecognizerTHF();
  SpeechRecognizerTHF(SpeechRecognizerTHF&& other);
  SpeechRecognizerTHF& operator=(SpeechRecognizerTHF&& other);
  SpeechRecognizerTHF(const SpeechRecognizerTHF& other) = delete;
  SpeechRecognizerTHF& operator=(const SpeechRecognizerTHF& other) = delete;
  
  bool Init(const char* const * phraseList, unsigned int numPhrases);
  
  virtual void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) override;
  
private:
  std::unique_ptr<SpeechRecognizerTHFData> _impl;
  
  void SwapAllData(SpeechRecognizerTHF& other);
  
  void HandleInitFail(const std::string& failMessage);
  static bool RecogStatusIsEndCondition(uint16_t status);
  void Cleanup();
  
}; // class SpeechRecognizer

} // end namespace Cozmo
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHF_H_
