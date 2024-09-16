// picovoice porcupine

#include "speechRecognizerPicovoice.h"

#include "audioUtil/speechRecognizer.h"
#include "picovoice.h"
#include "pv_porcupine.h"
#include "util/logging/logging.h"

#include <vector>
#include <mutex>

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "SpeechRecognizerPicovoice"

struct SpeechRecognizerPicovoice::SpeechRecognizerPicovoiceData
{
  std::vector<AudioUtil::AudioSample> audioBuffer;
  pv_porcupine_object_t* pvObj = nullptr;
  std::recursive_mutex recogMutex;
  bool disabled = true;
  bool reset = false;
};

SpeechRecognizerPicovoice::SpeechRecognizerPicovoice()
  : _impl(new SpeechRecognizerPicovoiceData())
{
}

SpeechRecognizerPicovoice::~SpeechRecognizerPicovoice()
{
  if (_impl->pvObj)
  {
    pv_porcupine_delete(_impl->pvObj);
    _impl->pvObj = nullptr;
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

  _impl->audioBuffer.reserve(512 * 2);

  if (_impl->pvObj)
  {
    pv_porcupine_delete(_impl->pvObj);
    _impl->pvObj = nullptr;
  }
  if (pv_porcupine_init("/data/pv/pv_params.pv", "/data/pv/keyword.ppn", 0.9f, &_impl->pvObj) != PV_STATUS_SUCCESS) {
    LOG_ERROR("SpeechRecognizerPicovoice.Update", "error setting up recognizer");
    return false;
  }

  LOG_ERROR("SpeechRecognizerPicovoice.Update", "Picovoice set up successfully!");
  _impl->disabled = false;

  return true;
}

void SpeechRecognizerPicovoice::Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen)
{
    std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);

    if (_impl->disabled)
    {
        return;
    }
    uint32_t frameLength = 512;
    if (frameLength == 0) {
        LOG_ERROR("SpeechRecognizerPicovoice.Update", "Invalid frame length from Picovoice");
        return;
    }

    _impl->audioBuffer.insert(_impl->audioBuffer.end(), audioData, audioData + audioDataLen);

    while (_impl->audioBuffer.size() >= frameLength)
    {
        std::vector<AudioUtil::AudioSample> frame(_impl->audioBuffer.begin(),
                                                  _impl->audioBuffer.begin() + frameLength);

        bool result = false;
        if (pv_porcupine_process(_impl->pvObj, frame.data(), &result) != PV_STATUS_SUCCESS) {
            LOG_ERROR("SpeechRecognizerPicovoice.Update", "pv process error");
            return;
        }
        LOG_INFO("SpeechRecognizerPicovoice.Update", "pv process success");
        if (result) {
            AudioUtil::SpeechRecognizerCallbackInfo info{
                .result = "Wake word detected",
                .startTime_ms = 0,
                .endTime_ms = 0,
                .score = 0.0f
            };
            DoCallback(info);
        }
        _impl->audioBuffer.erase(_impl->audioBuffer.begin(),
                                 _impl->audioBuffer.begin() + frameLength);
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
