/**
* File: speechRecognizerTHFSimple_v6.h
*
* Author: Lee Crippen
* Created: 12/12/2016
* Updated: 10/29/2017 Simple version rename to differentiate from legacy implementation.
*
* Description: SpeechRecognizer implementation for Sensory TrulyHandsFree.
*
* Copyright: Anki, Inc. 2016
*
*/
#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFSimple_V6_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFSimple_V6_H_

#include "audioUtil/speechRecognizer.h"

#include <memory>
#include <string>
#include <vector>

namespace Anki {
namespace Vector {
  
namespace VoiceCommand {
  class PhraseData;
  using PhraseDataSharedPtr = std::shared_ptr<PhraseData>;
  
  class RecognitionSetupData;
}

class SpeechRecognizerTHF_v6 : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerTHF_v6();
  virtual ~SpeechRecognizerTHF_v6();
  SpeechRecognizerTHF_v6(SpeechRecognizerTHF_v6&& other);
  SpeechRecognizerTHF_v6& operator=(SpeechRecognizerTHF_v6&& other);
  SpeechRecognizerTHF_v6(const SpeechRecognizerTHF_v6& other) = delete;
  SpeechRecognizerTHF_v6& operator=(const SpeechRecognizerTHF_v6& other) = delete;
  
  bool Init(const std::string& pronunPath);
  
  bool AddRecognitionDataFromFile(IndexType index,
                                  const std::string& nnFilePath, const std::string& searchFilePath,
                                  bool isPhraseSpotted, bool allowsFollowupRecog);
  void RemoveRecognitionData(IndexType index);
  void SetForceHeardPhrase(const char* phrase);
  
  virtual void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) override;
  virtual void SetRecognizerIndex(IndexType index) override;
  virtual void SetRecognizerFollowupIndex(IndexType index) override;
  virtual IndexType GetRecognizerIndex() const override;
  
  // TEMP
  void PerformCallBack(const char* callbackArg, float score, int from_ms, int to_ms);
  
private:
  struct SpeechRecognizerTHFData;
  std::unique_ptr<SpeechRecognizerTHFData> _impl;
  
  void SwapAllData(SpeechRecognizerTHF_v6& other);
  
  void HandleInitFail(const std::string& failMessage);
  static bool RecogStatusIsEndCondition(uint16_t status);
  void Cleanup();
  
}; // class SpeechRecognizer

} // end namespace Vector
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerTHFSimple_V6_H_
