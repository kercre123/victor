#include "audioUtil/audioCaptureSystem.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "voicego.h"

#include <memory>

using AudioSample = Anki::AudioUtil::AudioSample;
static void AudioInputCallback(const AudioSample* samples, uint32_t numSamples);

int main()
{
  // add logging
  auto logger = std::make_unique<Anki::Util::PrintfLoggerProvider>();
  Anki::Util::gLoggerProvider = logger.get();

  // create audio capture system
  Anki::AudioUtil::AudioCaptureSystem audioCapture{1600};
  audioCapture.SetCallback(std::bind(&AudioInputCallback, std::placeholders::_1, std::placeholders::_2));
  audioCapture.Init();

  GoMain();
  return 0;
}

static void AudioInputCallback(const int16_t* samples, uint32_t numSamples)
{
  // need a non-const buffer to pass to Go :(
  std::vector<int16_t> data{samples, samples+numSamples};
  GoSlice audioSlice{data.data(), numSamples, numSamples};
  GoAudioCallback(audioSlice);
}
