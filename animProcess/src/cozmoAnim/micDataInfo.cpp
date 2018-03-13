/**
* File: micDataInfo.cpp
*
* Author: Lee Crippen
* Created: 10/25/2017
*
* Description: Holds onto info related to recording and processing mic data.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "cozmoAnim/FftComplex.h"
#include "cozmoAnim/micDataInfo.h"
#include "audioUtil/waveFile.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/threading/threadPriority.h"
#include <iomanip>
#include <sstream>
#include <thread>

#define LOG_CHANNEL    "MicData"

namespace Anki {
namespace Cozmo {
namespace MicData {

namespace {
  const std::string kMicCapturePrefix = "miccapture_";
  const std::string kWavFileExtension = ".wav";
  const std::string kRawFileExtension = "_raw.wav";
}

void MicDataInfo::CollectRawAudio(const AudioUtil::AudioSample* audioChunk, size_t size)
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  if (_typesToRecord.IsBitFlagSet(MicDataType::Raw))
  {
    AudioUtil::AudioChunk newChunk;
    newChunk.resize(kRawAudioChunkSize);
    std::copy(audioChunk, audioChunk + size, newChunk.begin());
    _rawAudioData.push_back(std::move(newChunk));
  }
}

void MicDataInfo::CollectProcessedAudio(const AudioUtil::AudioSample* audioChunk, size_t size)
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  if (_typesToRecord.IsBitFlagSet(MicDataType::Processed))
  {
    AudioUtil::AudioChunk newChunk;
    newChunk.resize(kSamplesPerBlock);
    std::copy(audioChunk, audioChunk + size, newChunk.begin());
    _processedAudioData.push_back(std::move(newChunk));
  }
}

AudioUtil::AudioChunkList MicDataInfo::GetProcessedAudio(size_t beginIndex)
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  AudioUtil::AudioChunkList copiedData;
  auto currentSize = _processedAudioData.size();
  if (beginIndex >= currentSize)
  {
    return copiedData;
  }

  auto chunkIter = _processedAudioData.begin() + beginIndex;
  while (chunkIter != _processedAudioData.end())
  {
    const auto& audioChunk = *chunkIter;
    ++chunkIter;
    AudioUtil::AudioChunk newChunk;
    newChunk.resize(kSamplesPerBlock);
    std::copy(audioChunk.begin(), audioChunk.end(), newChunk.begin());
    copiedData.push_back(std::move(newChunk));
  }
  return copiedData;
}

void MicDataInfo::SetTimeToRecord(uint32_t timeToRecord)
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  _timeToRecord_ms = timeToRecord;
}

void MicDataInfo::UpdateForNextChunk()
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  _timeRecorded_ms += kTimePerChunk_ms;
  if (_timeRecorded_ms >= _timeToRecord_ms)
  {
    std::string dirToReplace;
    std::string nextFileNameBase;
    if (!_writeNameBase.empty())
    {
      nextFileNameBase = _writeNameBase;
      dirToReplace = _writeNameBase;
    }
    else
    {
      nextFileNameBase = ChooseNextFileNameBase(dirToReplace);
      // If we fail to find a name with which to save a file, bail now
      if (nextFileNameBase.empty())
      {
        _typesToRecord.ClearFlags();
        return;
      }
    }
    // Note this results in consuming the current audio chunks and fftcallback
    SaveCollectedAudio(_writeLocationDir, nextFileNameBase, dirToReplace);
    _timeRecorded_ms = 0;
    
    if (!_repeating)
    {
      _typesToRecord.ClearFlags();
    }
  }
}

bool MicDataInfo::CheckDone() const
{
  std::lock_guard<std::mutex> lock(_dataMutex);
  return !_typesToRecord.AreAnyFlagsSet();
}

uint32_t MicDataInfo::GetTimeToRecord_ms() const
{  
  std::lock_guard<std::mutex> lock(_dataMutex);
  return _timeToRecord_ms;
}

uint32_t MicDataInfo::GetTimeRecorded_ms() const
{  
  std::lock_guard<std::mutex> lock(_dataMutex);
  return _timeRecorded_ms;
}

void MicDataInfo::SaveCollectedAudio(const std::string& dataDirectory,
                                     const std::string& nameToUse,
                                     const std::string& nameToRemove)
{
  // Check against a min recording length. If we're not recording raw and our recorded processed time
  // is too short, we're going to abandon saving it.
  if (_rawAudioData.empty() && 
      (_processedAudioData.size() * kTimePerSEBlock_ms) < kMinAudioSizeToSave_ms)
  {
    return;
  }

  if (!nameToRemove.empty())
  {
    Util::FileUtils::RemoveDirectory(Util::FileUtils::FullFilePath({ dataDirectory, nameToRemove }));
  }
  
  const std::string& newDirPath = Util::FileUtils::FullFilePath({ dataDirectory, nameToUse });
  Util::FileUtils::CreateDirectory(newDirPath);
  const std::string& writeLocationBase = Util::FileUtils::FullFilePath({ newDirPath, nameToUse });
  if (!_rawAudioData.empty())
  {
    auto saveRawWave = [dest = (writeLocationBase + kRawFileExtension),
                        data = std::move(_rawAudioData),
                        doFFTProcess = _doFFTProcess,
                        fftCallback = std::move(_rawAudioFFTCallback),
                        length_ms = _timeRecorded_ms] () {
      Anki::Util::SetThreadName(pthread_self(), "saveRawWave");
      AudioUtil::WaveFile::SaveFile(dest, data, kNumInputChannels, kSampleRateIncoming_hz);

      LOG_INFO("MicDataInfo.WriteRawWaveFile", "%s", dest.c_str());
      
      if (doFFTProcess)
      {
        const float length_s = Util::numeric_cast<float>(length_ms / 1000.0f);
        if (!Util::IsNearZero(length_s))
        {
          std::vector<uint32_t> result = GetFFTResultFromRaw(data, length_s);
          LOG_INFO("MicDataInfo.FFTResultFromRaw", "%d %d %d %d", result[0], result[1], result[2], result[3]);
          if (fftCallback)
          {
            fftCallback(std::move(result));
          }
        }
      }
    };
    std::thread(saveRawWave).detach();
    _rawAudioData.clear();
  }

  if (!_processedAudioData.empty())
  {
    auto saveProcessedWave = [dest = (writeLocationBase + kWavFileExtension),
                              data = std::move(_processedAudioData)] () {
      Anki::Util::SetThreadName(pthread_self(), "saveProcWave");
      AudioUtil::WaveFile::SaveFile(dest, data);
      LOG_INFO("MicDataInfo.WriteProcessedWaveFile", "%s", dest.c_str());
    };
    std::thread(saveProcessedWave).detach();
    _processedAudioData.clear();
  }
}

// Since the local time on the robot is not reliable (especially over multiple reboots) we use a 
// 2 part numerical naming convention to identify the oldest file and overwrite it. The first number
// indicates which iteration of the file it is (number of times overwritten) and the second number indicates
// the sequence of that file. This way a simple sort reveals the oldest file next in line to be overwritten.
// This also selects the original file that needs to be overwritten, and sets the name of that file to output.
std::string MicDataInfo::ChooseNextFileNameBase(std::string& out_dirToDelete)
{
  std::vector<std::string> dirNames;
  Util::FileUtils::ListAllDirectories(_writeLocationDir, dirNames);
  auto listIter = dirNames.begin();
  while (listIter != dirNames.end())
  {
    // Remove entries not starting with prefix
    if (listIter->compare(0, kMicCapturePrefix.length(), kMicCapturePrefix) != 0)
    {
      listIter = dirNames.erase(listIter);
      continue;
    }

    listIter++;
  }
  
  // If number of entries is less than max, pick the name miccapture_0000_(count())
  if (dirNames.size() < _numMaxFiles)
  {
    std::ostringstream newNameStream;
    newNameStream << kMicCapturePrefix << "0000_" << std::setfill('0') << std::setw(4) << dirNames.size();
    return newNameStream.str();
  }
  
  // Otherwise:
  // Sort list of entries
  std::sort(dirNames.begin(), dirNames.end());
  
  // Take the first name in the list
  const auto& entryToReplace = dirNames.front();
  
  // Pull out the iteration number
  const auto iterStrBegin = kMicCapturePrefix.length();
  static constexpr uint32_t kNumberDigitsLength = 4;
  static constexpr uint32_t kMaxIterationNum = 9999;
  const auto iterationNum = std::stoi(entryToReplace.substr(iterStrBegin, kNumberDigitsLength));
  if (iterationNum == kMaxIterationNum)
  {
    LOG_ERROR("MicDataInfo.ChooseNextFileNameBase",
              "Reached max number of iterations %d. Won't save more files.",
              kMaxIterationNum);
    return "";
  }
  out_dirToDelete = entryToReplace;
  const auto seqStrBegin = iterStrBegin + kNumberDigitsLength + 1; // add one for the underscore
  const auto seqStr = entryToReplace.substr(seqStrBegin, kNumberDigitsLength);
  
  // use increased iteration number and old seq number to make the new filename
  std::ostringstream newNameStream;
  newNameStream << kMicCapturePrefix
                << std::setfill('0')
                << std::setw(kNumberDigitsLength)
                << (iterationNum + 1)
                << "_"
                << seqStr;
  return newNameStream.str();
}
  
std::vector<uint32_t> MicDataInfo::GetFFTResultFromRaw(const AudioUtil::AudioChunkList& data, float length_s)
{
  std::vector<uint32_t> perChannelFFT;
  
  // Run a seperate fft for each of the channels/mics
  for(auto i = 0; i < kNumInputChannels; ++i)
  {
    // Deinterlace the current channel from the raw audio chunks
    // Order in each raw audio chunk is channel 0,1,2,3,0,1,2,3,...
    std::vector<std::complex<double> > fftArray;
    for(const auto& chunk : data)
    {
      for(uint32_t j = i; j < chunk.size(); j += kNumInputChannels)
      {
        fftArray.push_back(chunk[j]);
      }
    }
    
    // Run fft in place
    Fft::transform(fftArray);
    
    // Keep track of the largest/most prominent value and index
    // from the fft
    // The index of the largest value will correspond to the most prominent
    // frequency in the audio data
    float    largestValue    = 0;
    uint32_t largestValueIdx = 0;
    
    // Skip the first one since it is garbage and often really large
    // Only look at the first half since the second half is just the inverse of the first
    // Only look at every other element to save processing time
    for(int i = 1; i < fftArray.size()/2; i += 2)
    {
      const auto& e = fftArray[i];
      float magSq = e.real()*e.real() + e.imag()*e.imag();
      if(magSq > largestValue)
      {
        largestValue = magSq;
        largestValueIdx = i;
      }
    }
    perChannelFFT.push_back((uint32_t)(largestValueIdx/length_s));
  }
  
  return perChannelFFT;
}

} // namespace MicData
} // namespace Cozmo
} // namespace Anki
