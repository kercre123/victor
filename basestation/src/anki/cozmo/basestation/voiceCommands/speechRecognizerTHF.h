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

class SpeechRecognizerTHF : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerTHF();
  virtual ~SpeechRecognizerTHF();
  SpeechRecognizerTHF(SpeechRecognizerTHF&& other);
  SpeechRecognizerTHF& operator=(SpeechRecognizerTHF&& other);
  SpeechRecognizerTHF(const SpeechRecognizerTHF& other) = delete;
  SpeechRecognizerTHF& operator=(const SpeechRecognizerTHF& other) = delete;
  
  bool Init();
  bool AddRecognitionDataAutoGen(IndexType index, const char* const * phraseList, unsigned int numPhrases);
  bool AddRecognitionDataFromFile(IndexType index,
                                  const std::string& nnFilePath, const std::string& searchFilePath,
                                  bool isPhraseSpotted, bool allowsFollowupRecog);
  void RemoveRecognitionData(IndexType index);
  
  virtual void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) override;
  virtual void SetRecognizerIndex(IndexType index) override;
  virtual void SetRecognizerFollowupIndex(IndexType index) override;
  
private:
  struct SpeechRecognizerTHFData;
  std::unique_ptr<SpeechRecognizerTHFData> _impl;
  
  void SwapAllData(SpeechRecognizerTHF& other);
  
  void HandleInitFail(const std::string& failMessage);
  static bool RecogStatusIsEndCondition(uint16_t status);
  void Cleanup();
  
}; // class SpeechRecognizer

} // end namespace Cozmo
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHF_H_
