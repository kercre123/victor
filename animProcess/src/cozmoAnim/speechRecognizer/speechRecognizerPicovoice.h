#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicovoice_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicovoice_H_

#include "audioUtil/speechRecognizer.h"

#include <memory>
#include <string>

namespace Anki {
namespace Vector {

class SpeechRecognizerPicovoice : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerPicovoice();
  virtual ~SpeechRecognizerPicovoice();
  SpeechRecognizerPicovoice(SpeechRecognizerPicovoice&& other);
  SpeechRecognizerPicovoice& operator=(SpeechRecognizerPicovoice&& other);
  SpeechRecognizerPicovoice(const SpeechRecognizerPicovoice& other) = delete;
  SpeechRecognizerPicovoice& operator=(const SpeechRecognizerPicovoice& other) = delete;

  bool Init();

  virtual void Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen) override;

  void Reset();

private:
  struct SpeechRecognizerPicovoiceData;
  std::unique_ptr<SpeechRecognizerPicovoiceData> _impl;

  virtual void StartInternal() override;
  virtual void StopInternal() override;

}; // class SpeechRecognizerPicovoice

} // end namespace Vector
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerPicovoice_H_
