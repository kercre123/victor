/**
* File: micDataProcessor.cpp
*
* Author: Lee Crippen
* Created: 9/27/2017
*
* Description: Handles processing the mic samples from the robot process: combining the channels,
*              and extracting direction data.
*
* Copyright: Anki, Inc. 2017
*
*/


#include "mmif.h"

#include "cozmoAnim/micDataProcessor.h"
#include "audioUtil/waveFile.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/rollingFileLogger.h"

#include <iomanip>
#include <sstream>
#include <thread>

namespace Anki {
namespace Cozmo {

namespace {
  const std::string kMicCapturePrefix = "miccapture_";
  const std::string kPreProcPrefix = "preproc_";
  const std::string kWavFileExtension = ".wav";
  const std::string kRawFileExtension = "_raw.wav";
  
  constexpr bool kSaveAudio = true;
  constexpr bool kSaveRawAudio = false; // only possible when saving audio at all
}

MicDataProcessor::MicDataProcessor(const std::string& writeLocation)
: _writeLocationDir(writeLocation)
{
  if (!_writeLocationDir.empty())
  {
    Util::FileUtils::CreateDirectory(_writeLocationDir);
  }

  MMIfInit(0, nullptr);

  // Enable this to process existing _raw files with no processed counterpart. Prefixes
  // both the original and the processed version with preproc_
  // ProcessExistingRawFiles(_writeLocationDir);
}

MicDataProcessor::~MicDataProcessor()
{
  MMIfDestroy();
}

void MicDataProcessor::CollectRawAudio(const AudioUtil::AudioSample* audioChunk)
{
  constexpr uint32_t samplesToCopy = kSamplesPerChunk * kNumInputChannels;
  AudioUtil::AudioChunk newChunk;
  newChunk.resize(samplesToCopy);
  std::copy(audioChunk, audioChunk + samplesToCopy, newChunk.data());
  _rawAudioData.push_back(std::move(newChunk));
}

void MicDataProcessor::ProcessRawAudio(const AudioUtil::AudioSample* audioChunk)
{
  // Uninterleave the chunks when copying out of the payload, since that's what SE wants
  for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerChunk; ++sampleIdx)
  {
    const uint32_t interleaveBase = (sampleIdx * kNumInputChannels);
    for (uint32_t channelIdx = 0; channelIdx < kNumInputChannels; ++channelIdx)
    {
      uint32_t dataOffset = _inProcessAudioBlockFirstHalf ? 0 : kSamplesPerChunk;
      const uint32_t uninterleaveBase = (channelIdx * kSamplesPerBlock) + dataOffset;
      _inProcessAudioBlock.data()[sampleIdx + uninterleaveBase] = audioChunk[channelIdx + interleaveBase];
    }
  }
  
  // If we aren't starting a block, we're finishing it - time to convert to a single channel
  if (!_inProcessAudioBlockFirstHalf)
  {
    // Process the current audio block with SE software
    static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
    AudioUtil::AudioChunk processedBlock;
    processedBlock.resize(kSamplesPerBlock);
    MMIfProcessMicrophones(dummySpeakerOut.data(), _inProcessAudioBlock.data(), processedBlock.data());
    
    if (kSaveAudio)
    {
      _processedAudioData.push_back(std::move(processedBlock));
      _collectedAudioSamples += kSamplesPerBlock;
    }
  }
  
  _inProcessAudioBlockFirstHalf = !_inProcessAudioBlockFirstHalf;
}

void MicDataProcessor::ProcessNextAudioChunk(const AudioUtil::AudioSample* audioChunk)
{
  ProcessRawAudio(audioChunk);
  if (kSaveAudio && kSaveRawAudio)
  {
    CollectRawAudio(audioChunk);
  }

  if (_collectedAudioSamples >= _audioSamplesToCollect)
  {
    std::string deletedFile = "";
    std::string nextFileNameBase = ChooseAndClearNextFileNameBase(deletedFile);
    if (!deletedFile.empty())
    {
      Util::FileUtils::DeleteFile(GetRawFileNameFromProcessed(deletedFile));
    }
    
    std::string writeLocationBase = Util::FileUtils::FullFilePath({ _writeLocationDir, nextFileNameBase });
    
    if (!_rawAudioData.empty())
    {
      auto saveRawWave = [dest = (writeLocationBase + kRawFileExtension), data = std::move(_rawAudioData)] () {
        AudioUtil::WaveFile::SaveFile(dest, data, kNumInputChannels);
        PRINT_NAMED_INFO("MicDataProcessor.WriteRawWaveFile", "%s", dest.c_str());
      };
      std::thread(saveRawWave).detach();
      _rawAudioData.clear();
    }

    if (!_processedAudioData.empty())
    {
      auto saveProcessedWave = [dest = (writeLocationBase + kWavFileExtension), data = std::move(_processedAudioData)] () {
        AudioUtil::WaveFile::SaveFile(dest, data);
        PRINT_NAMED_INFO("ProcessNextAudioChunk.WriteProcessedWaveFile", "%s", dest.c_str());
      };
      std::thread(saveProcessedWave).detach();
      _processedAudioData.clear();
    }

    _collectedAudioSamples = 0;
  }
}

void MicDataProcessor::ProcessExistingRawFiles(const std::string& micDataDir)
{
  const bool useFullPath = false;
  auto fileList = Util::FileUtils::FilesInDirectory(micDataDir, useFullPath, kRawFileExtension.c_str());
  
  // Remove files starting with prefix
  auto listIter = fileList.begin();
  while (listIter != fileList.end())
  {
    if (listIter->compare(0, kPreProcPrefix.length(), kPreProcPrefix) == 0)
    {
      listIter = fileList.erase(listIter);
    }
    else
    {
      listIter++;
    }
  }
  
  for (const auto& fileName : fileList)
  {
    const std::string& processedFileName = GetProcessedFileNameFromRaw(fileName);
    // Skip files that have already been processed
    if (Util::FileUtils::FileExists(Util::FileUtils::FullFilePath({micDataDir, processedFileName})))
    {
      continue;
    }
    const std::string& originalFilePath = Util::FileUtils::FullFilePath({micDataDir, fileName});
    auto audioChunkList = AudioUtil::WaveFile::ReadFile(originalFilePath, kSamplesPerChunk);
    for (const auto& chunk : audioChunkList)
    {
      ProcessRawAudio(chunk.data());
    }
    // If there's no processed data (like when processing is disabled) don't bother making empty files
    if (!_processedAudioData.empty())
    {
      std::string destFileName = Util::FileUtils::FullFilePath({micDataDir, kPreProcPrefix + processedFileName});
      auto saveProcessedWave = [dest = std::move(destFileName), data = std::move(_processedAudioData)] () {
        AudioUtil::WaveFile::SaveFile(dest, data);
        PRINT_NAMED_INFO("ProcessNextAudioChunk.WriteProcessedWaveFile", "%s", dest.c_str());
      };
      std::thread(saveProcessedWave).detach();
      _processedAudioData.clear();
      
      // now rename the original so it matches
      const std::string& newFilePath = Util::FileUtils::FullFilePath({micDataDir, kPreProcPrefix + fileName});
      std::rename(originalFilePath.c_str(), newFilePath.c_str());
    }
  }
}

// Since the local time on the robot is not reliable (especially over multiple reboots) we use a 
// 2 part numerical naming convention to identify the oldest file and overwrite it. The first number
// indicates which iteration of the file it is (number of times overwritten) and the second number indicates
// the sequence of that file. This way a simple sort reveals the oldest file next in line to be overwritten.
// This also deletes the original file found that needs to be overwritten, and sets the name of that file to output.
std::string MicDataProcessor::ChooseAndClearNextFileNameBase(std::string& out_deletedFileName)
{
  // Get list of files ending in .wav
  constexpr bool useFullPath = false;
  std::vector<std::string> fileNames = Util::FileUtils::FilesInDirectory(_writeLocationDir, useFullPath, kWavFileExtension.c_str());

  auto listIter = fileNames.begin();
  while (listIter != fileNames.end())
  {
    // Remove files not starting with prefix
    if (listIter->compare(0, kMicCapturePrefix.length(), kMicCapturePrefix) != 0)
    {
      listIter = fileNames.erase(listIter);
      continue;
    }

    // Also remove files that end in the _raw postfix
    const auto& iterLen = listIter->length();
    static const auto& extensionLen = kRawFileExtension.length();
    if (iterLen > extensionLen && 
        listIter->compare(iterLen - extensionLen, extensionLen, kRawFileExtension) == 0)
    {
      listIter = fileNames.erase(listIter);
      continue;
    }

    listIter++;
  }
  
  // If number of files is less than max, name the file miccapture_0000_(filecount())
  if (fileNames.size() < _filesToStore)
  {
    std::ostringstream newNameStream;
    newNameStream << kMicCapturePrefix << "0000_" << std::setfill('0') << std::setw(4) << fileNames.size();
    return newNameStream.str();
  }
  
  // Otherwise:
  // Sort list of files
  std::sort(fileNames.begin(), fileNames.end());
  
  // Take the first name in the list
  const auto& fileToReplace = fileNames.front();
  
  // Pull out the iteration number
  const auto iterStrBegin = kMicCapturePrefix.length();
  const auto iterationNum = std::stoi(fileToReplace.substr(iterStrBegin, 4));
  {
    auto fileToDelete = Util::FileUtils::FullFilePath({ _writeLocationDir, fileToReplace });
    Util::FileUtils::DeleteFile(fileToDelete);
    out_deletedFileName = std::move(fileToDelete);
  }
  const auto seqStrBegin = iterStrBegin + 4 + 1; // add one for the underscore
  const auto seqStr = fileToReplace.substr(seqStrBegin, 4);
  
  // use increased iteration number and old seq number to make the new filename
  std::ostringstream newNameStream;
  newNameStream << kMicCapturePrefix << std::setfill('0') << std::setw(4) << (iterationNum + 1) << "_" << seqStr;
  return newNameStream.str();
}

std::string MicDataProcessor::GetProcessedFileNameFromRaw(const std::string& rawFileName)
{
  return rawFileName.substr(0, rawFileName.length() - kRawFileExtension.length()) + kWavFileExtension;
}

std::string MicDataProcessor::GetRawFileNameFromProcessed(const std::string& processedName)
{
  return processedName.substr(0, processedName.length() - kWavFileExtension.length()) + kRawFileExtension;
}

} // namespace Cozmo
} // namespace Anki
