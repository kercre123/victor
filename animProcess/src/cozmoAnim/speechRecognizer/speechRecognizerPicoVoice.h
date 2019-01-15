/**
* File: speechRecognizerPicoVoice.h
*
* Author: chapados
* Created: Jan 10 2019
*
* Description: SpeechRecognizer implementation for PicoVoice (Porcupine)
*
* Copyright: Anki, Inc. 2019
*
*/
#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicoVoice_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicoVoice_H_

#include "audioUtil/speechRecognizer.h"

#include <memory>

namespace Anki {
namespace Vector {


class SpeechRecognizerPicoVoice : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerPicoVoice();
  virtual ~SpeechRecognizerPicoVoice();
  SpeechRecognizerPicoVoice(SpeechRecognizerPicoVoice&& other);
  SpeechRecognizerPicoVoice& operator=(SpeechRecognizerPicoVoice&& other);
  SpeechRecognizerPicoVoice(const SpeechRecognizerPicoVoice& other) = delete;
  SpeechRecognizerPicoVoice& operator=(const SpeechRecognizerPicoVoice& other) = delete;
  
  bool Init(const std::string& modelBasePath);
  
  virtual void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen) override;
  virtual void SetRecognizerIndex(IndexType index) override;
  virtual void SetRecognizerFollowupIndex(IndexType index) override;
  virtual IndexType GetRecognizerIndex() const override;

private:
  struct SpeechRecognizerPicoVoiceData;
  std::unique_ptr<SpeechRecognizerPicoVoiceData> _impl;
  std::string _modelBasePath;

  void SwapAllData(SpeechRecognizerPicoVoice& other);

  void Cleanup();
  void SetupConsoleFuncs();

  virtual void StartInternal() override;
  virtual void StopInternal() override;
  

}; // class SpeechRecognizer

} // end namespace Vector
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicoVoice_H_
