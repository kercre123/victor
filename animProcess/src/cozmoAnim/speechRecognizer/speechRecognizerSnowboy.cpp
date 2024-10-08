#include "speechRecognizerSnowboy.h"

#include "audioUtil/speechRecognizer.h"
#include "snowboy_client.h"
#include "util/logging/logging.h"

#include <vector>
#include <mutex>

#include <chrono>
#include <thread>
#include <sys/stat.h>
#include <cstdlib>
#include <unistd.h>

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "SpeechRecognizer"

struct SpeechRecognizerSnowboy::SpeechRecognizerSnowboyData
{
  std::vector<AudioUtil::AudioSample> audioBuffer;
  std::recursive_mutex recogMutex;
  bool disabled = true;
  bool reset = false;
};

SpeechRecognizerSnowboy::SpeechRecognizerSnowboy()
  : _impl(new SpeechRecognizerSnowboyData())
{
}

SpeechRecognizerSnowboy::~SpeechRecognizerSnowboy()
{
  // if (_impl->pvObj)
  // {
  //   pv_porcupine_delete(_impl->pvObj);
  //   _impl->pvObj = nullptr;
  // }
}

SpeechRecognizerSnowboy::SpeechRecognizerSnowboy(SpeechRecognizerSnowboy&& other)
  : AudioUtil::SpeechRecognizer(std::move(other)),
    _impl(std::move(other._impl))
{
}

SpeechRecognizerSnowboy& SpeechRecognizerSnowboy::operator=(SpeechRecognizerSnowboy&& other)
{
  AudioUtil::SpeechRecognizer::operator=(std::move(other));
  _impl = std::move(other._impl);
  return *this;
}

bool SpeechRecognizerSnowboy::Init()
{
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);

  pid_t pid = fork();
  if (pid < 0) {
    LOG_ERROR("SpeechRecognizerSnowboy.Init", "Failed to fork process for pv_server");
    return false;
  } else if (pid == 0) {
    struct stat buffer;
    const char* wakewordPath;

    if (stat("/data/data/com.anki.victor/persistent/customWakeWord/wakeword.pmdl", &buffer) == 0) {
      wakewordPath = "/data/data/com.anki.victor/persistent/customWakeWord/wakeword.pmdl";
    } else {
      wakewordPath = "/anki/data/assets/cozmo_resources/assets/snowboyModels/hey_vector.pmdl";
    }

    execl("/anki/bin/sb_server", "sb_server", "/anki/data/assets/cozmo_resources/assets/snowboyModels/common.res", wakewordPath, (char*)nullptr);
    LOG_ERROR("SpeechRecognizerSnowboy.Init", "Failed to exec sb_server");
    std::exit(EXIT_FAILURE);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));


  _impl->audioBuffer.reserve(2048 * 2);

  if (snowboy_init() != 0) {
    LOG_ERROR("SpeechRecognizerSnowboy.Init", "snowboy failed to init :(");
    return false;
  }

  LOG_INFO("SpeechRecognizerSnowboy.Init", "snowboy set up successfully!");
  _impl->disabled = false;

  return true;
}

void SpeechRecognizerSnowboy::Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen)
{
    std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);

    if (_impl->disabled)
    {
        return;
    }
    uint32_t frameLength = 2048;
    if (frameLength == 0) {
        LOG_ERROR("SpeechRecognizerSnowboy.Update", "Invalid frame length from Snowboy");
        return;
    }

    _impl->audioBuffer.insert(_impl->audioBuffer.end(), audioData, audioData + audioDataLen);

    while (_impl->audioBuffer.size() >= frameLength)
    {
        std::vector<AudioUtil::AudioSample> frame(_impl->audioBuffer.begin(),
                                                  _impl->audioBuffer.begin() + frameLength);

        const char* frameData = reinterpret_cast<const char*>(frame.data());
        uint32_t dataLength = frameLength * sizeof(AudioUtil::AudioSample);

        int result = snowboy_process(frameData, dataLength);
        if (result == 1) {
            AudioUtil::SpeechRecognizerCallbackInfo info{
                .result = "Wake word detected",
                .startTime_ms = 0,
                .endTime_ms = 0,
                .score = 0.0f
            };
            DoCallback(info);
        } else if (result == -1) {
          LOG_ERROR("SpeechRecognizerSnowboy.Update", "Snowboy error :(");
        }
        _impl->audioBuffer.erase(_impl->audioBuffer.begin(),
                                 _impl->audioBuffer.begin() + frameLength);
    }
}

void SpeechRecognizerSnowboy::Reset()
{
  std::lock_guard<std::recursive_mutex> lock(_impl->recogMutex);
  _impl->reset = true;
}

void SpeechRecognizerSnowboy::StartInternal()
{
  _impl->disabled = false;
}

void SpeechRecognizerSnowboy::StopInternal()
{
  _impl->disabled = true;
}

} // end namespace Vector
} // end namespace Anki
