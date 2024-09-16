// picovoice porcupine

#include "speechRecognizerPicovoice.h"

#include "audioUtil/speechRecognizer.h"
#include "picovoice.h"
#include "pv_porcupine.h"
#include "util/logging/logging.h"

#include <vector>
#include <mutex>

#include <chrono>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "SpeechRecognizer"

const char *keyword_path[] = {"/anki/data/assets/cozmo_resources/assets/porcupineModels/Hey-Vector_v3.0.0.ppn"};
const float sensitivities[] = {0.9f};

struct SpeechRecognizerPicovoice::SpeechRecognizerPicovoiceData
{
  std::vector<AudioUtil::AudioSample> audioBuffer;
  pv_porcupine_t* pvObj = nullptr;
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

  pid_t pid = fork();
  if (pid < 0) {
    LOG_ERROR("SpeechRecognizerPicovoice.Init", "Failed to fork process for pv_server");
    return false;
  } else if (pid == 0) {
    execl("/anki/bin/launch_pv_server.sh", "launch_pv_server.sh", (char*)nullptr);
    LOG_ERROR("SpeechRecognizerPicovoice.Init", "Failed to exec pv_server");
    std::exit(EXIT_FAILURE);
  } else {
    const char* socketPath = "/dev/socket/_anim_pv_wakeword_";
    int maxRetries = 100;
    int retries = 0;
    while (retries < maxRetries) {
      if (access(socketPath, F_OK) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      ++retries;
    }

    if (retries == maxRetries) {
      LOG_ERROR("SpeechRecognizerPicovoice.Init", "Timeout waiting for pv_server to create socket");
      return false;
    }
  }

  _impl->audioBuffer.reserve(512 * 2);

  if (_impl->pvObj) {
    pv_porcupine_delete(_impl->pvObj);
    _impl->pvObj = nullptr;
  }

  std::string apiKey;
  {
    std::ifstream apiKeyFile("/data/picoKey");
    if (!apiKeyFile.is_open()) {
      LOG_ERROR("SpeechRecognizerPicovoice.Init", "Failed to open /data/picoKey");
      return false;
    }
    std::getline(apiKeyFile, apiKey);
    apiKeyFile.close();

    if (apiKey.empty()) {
      LOG_ERROR("SpeechRecognizerPicovoice.Init", "API key doesn't exist in /data/picoKey");
      return false;
    }
  }
  
  if (pv_porcupine_init(apiKey.c_str(),
        "/anki/data/assets/cozmo_resources/assets/porcupineModels/porcupine_params_v3.0.0.pv", 
        1,
        keyword_path, 
        sensitivities, 
        &_impl->pvObj
      ) != PV_STATUS_SUCCESS) {
    LOG_ERROR("SpeechRecognizerPicovoice.Init", "Error setting up recognizer");
    return false;
  }

  LOG_INFO("SpeechRecognizerPicovoice.Init", "Picovoice set up successfully!");
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

        // idk what i did, but process always gives a -1 result. when wakeword, it becomes 0
        int result = -1;
        if (pv_porcupine_process(_impl->pvObj, frame.data(), &result) != PV_STATUS_SUCCESS) {
            LOG_ERROR("SpeechRecognizerPicovoice.Update", "pv process error");
            return;
        }
        if (result == 0) {
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
