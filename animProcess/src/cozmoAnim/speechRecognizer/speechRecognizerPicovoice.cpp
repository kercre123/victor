// originally had picovoice. that didn't work, so vosk instead!

#include "speechRecognizerPicovoice.h"

#include "audioUtil/speechRecognizer.h"
#include "vosk_api.h"
#include "util/logging/logging.h"

#include <vector>
#include <mutex>

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "SpeechRecognizerPicovoice"

const char* CustomWakeWord = "alexa";

struct SpeechRecognizerPicovoice::SpeechRecognizerPicovoiceData
{
  VoskRecognizer* voskReg = nullptr;
  VoskModel* voskModel = nullptr;
  std::recursive_mutex recogMutex;
  bool disabled = false;
  bool reset = false;
};

SpeechRecognizerPicovoice::SpeechRecognizerPicovoice()
  : _impl(new SpeechRecognizerPicovoiceData())
{
}

SpeechRecognizerPicovoice::~SpeechRecognizerPicovoice()
{
  if (_impl->voskReg)
  {
    vosk_recognizer_free(_impl->voskReg);
    vosk_model_free(_impl->voskModel);
    _impl->voskReg = nullptr;
    _impl->voskModel = nullptr;
  }
}

SpeechRecognizerPicovoice::SpeechRecognizerPicovoice(SpeechRecognizerPicovoice&& other)
  : AudioUtil::SpeechRecognizer(std::move(other)),
    _impl(std::move(other._impl))
{
}

SpeechRecognizerPicovoice& SpeechRecognizerPicovoice::operator=(SpeechRecognizerPicovoice&& other)
{
  AudioUtil::SpeechRecognizer::operator=(std::move(other));
  _impl = std::move(other._impl);
  return *this;
}

bool SpeechRecognizerPicovoice::Init()
{
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);

  if (_impl->voskReg)
  {
    vosk_recognizer_free(_impl->voskReg);
    vosk_model_free(_impl->voskModel);
    _impl->voskReg = nullptr;
    _impl->voskModel = nullptr;
  }
  _impl->voskModel = vosk_model_new("/data/model");
  char GrmList[50];
  sprintf(GrmList, "[\"%s\", \"clunk\"]", CustomWakeWord);
  _impl->voskReg = vosk_recognizer_new_grm(_impl->voskModel, 16000.0, GrmList);

  return true;
}

void SpeechRecognizerPicovoice::DoNew() {
    LOG_INFO("SpeechRecognizerPicovoice.Update", "VOSK: resetting rec");
    vosk_recognizer_free(_impl->voskReg);
    char GrmList[50];
    sprintf(GrmList, "[\"%s\", \"clunk\", \"beep\", \"hello\"]", CustomWakeWord);
    _impl->voskReg = vosk_recognizer_new_grm(_impl->voskModel, 16000.0, GrmList);
    LOG_INFO("SpeechRecognizerPicovoice.Update", "New rec created");
}

void SpeechRecognizerPicovoice::Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen)
{
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);

  if (!_impl->voskReg || _impl->disabled)
  {
    return;
  }

  int final;

  // Porcupine expects audio frames of specific size

    final = vosk_recognizer_accept_waveform(_impl->voskReg, (char*)audioData, audioDataLen);
    if (final < 0) {
      LOG_ERROR("SpeechRecognizerPicovoice.Update", "VOSK exception occurred :(");
      return;
    }

    if (final)
    {
      LOG_INFO("SpeechRecognizerPicovoice.Update", "%s", vosk_recognizer_result(_impl->voskReg));
      LOG_INFO("SpeechRecognizerPicovoice.Update", "FINAL");
      DoNew();
    } else {
      if (strstr(vosk_recognizer_partial_result(_impl->voskReg), CustomWakeWord) != nullptr) {
        AudioUtil::SpeechRecognizerCallbackInfo info{
          .result = "Wake word detected",
          .startTime_ms = 0,
          .endTime_ms = 0,
          .score = 0.0f
        };
        DoCallback(info);
        LOG_INFO("SpeechRecognizerPicovoice.Update", "Wake word detected");
        DoNew();
      }
    }
}

void SpeechRecognizerPicovoice::Reset()
{
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);
  _impl->reset = true;
}

void SpeechRecognizerPicovoice::StartInternal()
{
  _impl->disabled = false;
}

void SpeechRecognizerPicovoice::StopInternal()
{
  _impl->disabled = true;
}

} // end namespace Vector
} // end namespace Anki
