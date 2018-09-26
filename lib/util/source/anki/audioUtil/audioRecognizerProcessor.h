/**
* File: audioRecognizerProcessor.h
*
* Author: Lee Crippen
* Created: 1/20/17
*
* Description: Component that uses native audio capture and feeds it to a specified speech recognizer,
* then holds onto the results to be accessed by another system.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Anki_AudioUtil_AudioRecognizerProcessor_H_
#define __Anki_AudioUtil_AudioRecognizerProcessor_H_

#include "audioDataTypes.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace Anki {
namespace AudioUtil {
  
class IAudioInputSource;
class SpeechRecognizer;
  
class AudioRecognizerProcessor
{
public:
  AudioRecognizerProcessor(const std::string& savedAudioDir);
  virtual ~AudioRecognizerProcessor();
  AudioRecognizerProcessor(AudioRecognizerProcessor&& other) = delete;
  AudioRecognizerProcessor& operator=(AudioRecognizerProcessor&& other) = delete;
  AudioRecognizerProcessor(const AudioRecognizerProcessor& other) = delete;
  AudioRecognizerProcessor& operator=(const AudioRecognizerProcessor& other) = delete;
  
  void SetAudioInputSource(IAudioInputSource* newCaptureSystem);
  void SetSpeechRecognizer(SpeechRecognizer* newRecog);
  void Start();
  void Stop();

  struct ResultType {
    std::string first;
    float second;
    float from_ms;
    float to_ms;
  };
  bool HasResults() const;
  ResultType PopNextResult();
  
private:
  SpeechRecognizer*                       _recognizer = nullptr;
  IAudioInputSource*                      _audioInputSource = nullptr;
  bool                                    _capturingAudio = false;
  std::mutex                              _componentsMutex;
  mutable std::mutex                      _resultMutex;
  std::deque<ResultType>                  _procResults;
  
  void AudioSamplesCallback(const AudioSample* buffer, uint32_t numSamples);
  void AddRecognizerResult(const char* data, float score, float from_ms, float to_ms);
  
}; // class AudioRecognizerProcessor
    
} // end namespace AudioUtil
} // end namespace Anki

#endif // __Anki_AudioUtil_AudioRecognizerProcessor_H_
