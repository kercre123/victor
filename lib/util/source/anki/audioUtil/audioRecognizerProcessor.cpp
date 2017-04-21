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
}

AudioRecognizerProcessor::~AudioRecognizerProcessor()
{
  Stop();
}
  
void AudioRecognizerProcessor::SetSpeechRecognizer(SpeechRecognizer* newRecog)
{
  std::lock_guard<std::mutex> lock(_componentsMutex);
  if (_recognizer)
  {
    _recognizer->SetCallback();
  }
  
  _recognizer = newRecog;
  if (_recognizer)
  {
    _recognizer->SetCallback(std::bind(&AudioRecognizerProcessor::AddRecognizerResult, this, std::placeholders::_1, std::placeholders::_2));
  }
}


void AudioRecognizerProcessor::SetAudioCaptureSystem(AudioCaptureSystem* newCaptureSystem)
{
  std::lock_guard<std::mutex> lock(_componentsMutex);
  if (_captureSystem)
  {
    _captureSystem->SetCallback();
  }
  
  _captureSystem = newCaptureSystem;
  if (_captureSystem)
  {
    _captureSystem->SetCallback(std::bind(&AudioRecognizerProcessor::AudioSamplesCallback, this, std::placeholders::_1, std::placeholders::_2));
  }
}

void AudioRecognizerProcessor::Start()
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  _capturingAudio = true;
}

void AudioRecognizerProcessor::Stop()
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  _capturingAudio = false;
}
  
void AudioRecognizerProcessor::AudioSamplesCallback(const AudioSample* buffer, uint32_t numSamples)
{
  std::lock_guard<std::mutex> lock(_componentsMutex);
  if (_recognizer)
  {
    _recognizer->Update(buffer, numSamples);
  }
}
                           
void AudioRecognizerProcessor::AddRecognizerResult(const char* data, float score)
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  if (_capturingAudio)
  {
    _procResults.push_back( {data, score} );
  }
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
