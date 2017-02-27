/**
* File: audioRecognizerProcessor.cpp
*
* Author: Lee Crippen
* Created: 1/20/17
*
* Description:
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioRecognizerProcessor.h"

#include "audioCaptureSystem.h"
#include "speechRecognizer.h"

namespace Anki {
namespace AudioUtil {
  

AudioRecognizerProcessor::AudioRecognizerProcessor()
: _captureSystem(new AudioCaptureSystem())
{
  if (!_captureSystem->IsValid())
  {
    _captureSystem.reset();
  }
}

AudioRecognizerProcessor::~AudioRecognizerProcessor()
{
  Stop();
}
  
void AudioRecognizerProcessor::SetSpeechRecognizer(SpeechRecognizer* newRecog)
{
  std::lock_guard<std::mutex> lock(_recognizerMutex);
  if (_recognizer)
  {
    _recognizer->Stop();
    _recognizer->SetCallback();
  }
  
  _recognizer = newRecog;
  _recognizer->SetCallback(std::bind(&AudioRecognizerProcessor::AddRecognizerResult, this, std::placeholders::_1));
  _recognizer->Start();
}

void AudioRecognizerProcessor::Start()
{
  if (!_captureSystem || _capturingAudio)
  {
    return;
  }
  
  _capturingAudio = true;
  _captureSystem->SetCallback(std::bind(&AudioRecognizerProcessor::AudioSamplesCallback, this, std::placeholders::_1, std::placeholders::_2));
  _captureSystem->StartRecording();
}
  
void AudioRecognizerProcessor::AudioSamplesCallback(const AudioSample* buffer, uint32_t numSamples)
{
  std::lock_guard<std::mutex> lock(_recognizerMutex);
  if (_recognizer)
  {
    _recognizer->Update(buffer, numSamples);
  }
}

void AudioRecognizerProcessor::Stop()
{
  if (!_captureSystem || !_capturingAudio)
  {
    return;
  }
  
  _capturingAudio = false;
  _captureSystem->StopRecording();
  _captureSystem->ClearCallback();
}
                           
void AudioRecognizerProcessor::AddRecognizerResult(const char* data)
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  _procResults.push_back(data);
}

bool AudioRecognizerProcessor::HasResults() const
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  return !_procResults.empty();
}

AudioRecognizerProcessor::ResultType AudioRecognizerProcessor::PopNextResult()
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  ResultType returnData = std::move(_procResults.front());
  _procResults.pop_front();
  return returnData;
}

} // end namespace AudioUtil
} // end namespace Anki
