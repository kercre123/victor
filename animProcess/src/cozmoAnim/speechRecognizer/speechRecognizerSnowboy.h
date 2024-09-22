#ifndef __Cozmo_Basestation_VoiceCommands_SpeechRecognizerSnowboy_H_
#define __Cozmo_Basestation_VoiceCommands_SpeechRecognizerSnowboy_H_

#include "audioUtil/speechRecognizer.h"

#include <memory>
#include <string>

namespace Anki {
namespace Vector {

class SpeechRecognizerSnowboy : public AudioUtil::SpeechRecognizer
{
public:
  SpeechRecognizerSnowboy();
  virtual ~SpeechRecognizerSnowboy();
  SpeechRecognizerSnowboy(SpeechRecognizerSnowboy&& other);
  SpeechRecognizerSnowboy& operator=(SpeechRecognizerSnowboy&& other);
  SpeechRecognizerSnowboy(const SpeechRecognizerSnowboy& other) = delete;
  SpeechRecognizerSnowboy& operator=(const SpeechRecognizerSnowboy& other) = delete;

  bool Init();

  virtual void Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen) override;

  void Reset();

private:
  struct SpeechRecognizerSnowboyData;
  std::unique_ptr<SpeechRecognizerSnowboyData> _impl;

  virtual void StartInternal() override;
  virtual void StopInternal() override;

}; // class SpeechRecognizerSnowboy

} // end namespace Vector
} // end namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_SpeechRecognizerSnowboy_H_
