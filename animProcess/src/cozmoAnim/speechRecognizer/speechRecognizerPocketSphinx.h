/**
* File: speechRecognizerPocketSphinx.h
*
* Author: ross
* Created: Jan 3 2019
*
* Description: SpeechRecognizer implementation for CMU's pocketsphinx
*
* Copyright: Anki, Inc. 2016
*
*/
#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPocketSphinx_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPocketSphinx_H_

#include "audioUtil/speechRecognizer.h"

#include <memory>

namespace Anki {
namespace Vector {


class SpeechRecognizerPocketSphinx : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerPocketSphinx();
  virtual ~SpeechRecognizerPocketSphinx();
  SpeechRecognizerPocketSphinx(SpeechRecognizerPocketSphinx&& other);
  SpeechRecognizerPocketSphinx& operator=(SpeechRecognizerPocketSphinx&& other);
  SpeechRecognizerPocketSphinx(const SpeechRecognizerPocketSphinx& other) = delete;
  SpeechRecognizerPocketSphinx& operator=(const SpeechRecognizerPocketSphinx& other) = delete;
  
  bool Init(const std::string& modelBasePath);
  
  virtual void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) override;
  virtual void SetRecognizerIndex(IndexType index) override;
  virtual void SetRecognizerFollowupIndex(IndexType index) override;
  virtual IndexType GetRecognizerIndex() const override;

private:
  struct SpeechRecognizerPocketSphinxData;
  std::unique_ptr<SpeechRecognizerPocketSphinxData> _impl;

  void SwapAllData(SpeechRecognizerPocketSphinx& other);

  void Cleanup();
  
  virtual void StartInternal() override;
  virtual void StopInternal() override;
  

}; // class SpeechRecognizer

} // end namespace Vector
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPocketSphinx_H_
