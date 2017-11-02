#include "audioUtil/audioCaptureSystem.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

#ifndef HEADER_NAME
#define HEADER_NAME "voicego.h"
#endif

#include HEADER_NAME

#include <functional>
#include <memory>

using AudioSample = Anki::AudioUtil::AudioSample;
static void AudioInputCallback(const AudioSample* samples, uint32_t numSamples);

std::function<void()> startRecordingFunc;
std::function<void()> stopRecordingFunc;

// C exports
extern "C" {

  void StartRecording() {
    startRecordingFunc();
  }

  void StopRecording() {
    stopRecordingFunc();
  }

}

int main()
{
  // add logging
  auto logger = std::make_unique<Anki::Util::PrintfLoggerProvider>();
  Anki::Util::gLoggerProvider = logger.get();

  // create audio capture system
  Anki::AudioUtil::AudioCaptureSystem audioCapture{100};
  audioCapture.SetCallback(std::bind(&AudioInputCallback, std::placeholders::_1, std::placeholders::_2));
  audioCapture.Init();

  // bind C callbacks
  startRecordingFunc = [&audioCapture] {
    audioCapture.StartRecording();
  };
  stopRecordingFunc = [&audioCapture] {
    audioCapture.StopRecording();
  };

  GoMain(StartRecording, StopRecording);
  return 0;
}

static void AudioInputCallback(const int16_t* samples, uint32_t numSamples)
{
  // need a non-const buffer to pass to Go :(
  std::vector<int16_t> data{samples, samples+numSamples};
  GoSlice audioSlice{data.data(), numSamples, numSamples};
  GoAudioCallback(audioSlice);
}
