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
  
class AudioCaptureSystem;
class SpeechRecognizer;
  
class AudioRecognizerProcessor
{
public:
  AudioRecognizerProcessor();
  virtual ~AudioRecognizerProcessor();
  AudioRecognizerProcessor(AudioRecognizerProcessor&& other) = delete;
  AudioRecognizerProcessor& operator=(AudioRecognizerProcessor&& other) = delete;
  AudioRecognizerProcessor(const AudioRecognizerProcessor& other) = delete;
  AudioRecognizerProcessor& operator=(const AudioRecognizerProcessor& other) = delete;
  bool IsValid() const { return _captureSystem != nullptr; }
  
  void SetSpeechRecognizer(SpeechRecognizer* newRecog);
  void Start();
  void Stop();

  using ResultType = std::string;
  bool HasResults() const;
  ResultType PopNextResult();
  
private:
  SpeechRecognizer*                       _recognizer = nullptr;
  std::unique_ptr<AudioCaptureSystem>     _captureSystem;
  bool                                    _capturingAudio = false;
  mutable std::mutex                      _recognizerMutex;
  mutable std::mutex                      _resultMutex;
  std::deque<ResultType>                  _procResults;
  
  void AudioSamplesCallback(const AudioSample* buffer, uint32_t numSamples);
  void AddRecognizerResult(const char* data);
  
}; // class AudioRecognizerProcessor
    
} // end namespace AudioUtil
} // end namespace Anki

#endif // __Anki_AudioUtil_AudioRecognizerProcessor_H_
