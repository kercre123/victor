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

#include "audioUtil/audioRecognizerProcessor.h"

#include "audioUtil/iAudioInputSource.h"
#include "audioUtil/speechRecognizer.h"
#include "audioUtil/waveFile.h"

#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"

#include <chrono>

#include "util/console/consoleInterface.h"

namespace {
  std::string                             _savedAudioDir;
  Anki::AudioUtil::AudioChunkList         _savedAudio;
  uint32_t                                _minSamplesToCapture = 0;
  
#if ANKI_DEV_CHEATS
  void RecordAudioInput(ConsoleFunctionContextRef context)
  {
    constexpr uint32_t                      _kDefaultNumSecondsToCapture = 5;
    constexpr uint32_t                      _kMaxNumSecondsToCapture = 60;
    
    uint32_t numSeconds = ConsoleArg_GetOptional_UInt(context, "seconds", 0);
    if (numSeconds > _kMaxNumSecondsToCapture)
    {
      numSeconds = _kMaxNumSecondsToCapture;
    }
    else if (numSeconds == 0)
    {
      numSeconds = _kDefaultNumSecondsToCapture;
    }
    _minSamplesToCapture = numSeconds * Anki::AudioUtil::kSampleRate_hz;
  }
  
  CONSOLE_FUNC(RecordAudioInput, "VoiceCommand", optional uint32_t seconds);
#endif
}

namespace Anki {
namespace AudioUtil {
  
AudioRecognizerProcessor::AudioRecognizerProcessor(const std::string& savedAudioDir)
{
  if (ANKI_DEV_CHEATS)
  {
    _savedAudioDir = savedAudioDir;
  }
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
    _recognizer->SetCallback(std::bind(&AudioRecognizerProcessor::AddRecognizerResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  }
}


void AudioRecognizerProcessor::SetAudioInputSource(IAudioInputSource* audioInputSource)
{
  std::lock_guard<std::mutex> lock(_componentsMutex);
  if (_audioInputSource)
  {
    _audioInputSource->SetCallback(IAudioInputSource::DataCallback{});
  }
  
  _audioInputSource = audioInputSource;
  if (_audioInputSource)
  {
    _audioInputSource->SetCallback(std::bind(&AudioRecognizerProcessor::AudioSamplesCallback, this, std::placeholders::_1, std::placeholders::_2));
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
  {
    std::lock_guard<std::mutex> lock(_componentsMutex);
    if (_recognizer)
    {
      _recognizer->Update(buffer, numSamples);
    }
  }
  
  if (ANKI_DEV_CHEATS && _minSamplesToCapture > 0)
  {
    AudioChunk newChunk;
    newChunk.reserve(numSamples);
    newChunk.insert(newChunk.end(), buffer, buffer + numSamples);
    _savedAudio.push_back(std::move(newChunk));
    
    if (_minSamplesToCapture <= numSamples)
    {
      _minSamplesToCapture = 0;
      auto curTimeString = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
      if (!_savedAudioDir.empty() && Util::FileUtils::DirectoryDoesNotExist(_savedAudioDir))
      {
        Util::FileUtils::CreateDirectory(_savedAudioDir);
      }
      const auto& saveFilePath = Util::FileUtils::FullFilePath({_savedAudioDir, curTimeString + ".wav"});
      if (!WaveFile::SaveFile(saveFilePath, _savedAudio))
      {
        PRINT_NAMED_ERROR("AudioRecognizerProcessor.AudioSamplesCallback.SaveAudioClipFail",
                          "Audio clip saving failed at path %s",
                          saveFilePath.c_str());
      }
      _savedAudio.clear();
    }
    else
    {
      _minSamplesToCapture -= numSamples;
    }
  }
}
                           
void AudioRecognizerProcessor::AddRecognizerResult(const char* data, float score, float from_ms, float to_ms)
{
  std::lock_guard<std::mutex> lock(_resultMutex);
  if (_capturingAudio)
  {
    _procResults.push_back( {data, score, from_ms, to_ms} );
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
