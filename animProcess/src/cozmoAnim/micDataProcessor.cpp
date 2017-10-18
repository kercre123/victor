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
#include "speex/speex_resampler.h"

#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/FftComplex.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include "audioUtil/waveFile.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/rollingFileLogger.h"
#include "util/math/numericCast.h"

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
  const std::string kResampledFileExtension = "_resamp.wav";
  
  constexpr bool kSaveAudio = true;
  constexpr bool kSaveRawAudio = true && kSaveAudio; // only possible when saving audio at all
  constexpr bool kSaveResampledAudio = false && kSaveAudio; // only possible when saving audio at all
}

MicDataProcessor::MicDataProcessor(const std::string& writeLocation)
: _writeLocationDir(writeLocation)
{
  if (!_writeLocationDir.empty())
  {
    Util::FileUtils::CreateDirectory(_writeLocationDir);
  }

  MMIfInit(0, nullptr);

  int error = 0;
  _speexState = speex_resampler_init(
    kNumInputChannels, // num channels
    kSampleRateIncoming_hz, // in rate, int
    AudioUtil::kSampleRate_hz, // out rate, int
    10, // quality 0-10
    &error);

  // Enable this to process existing _raw files with no processed counterpart. Prefixes
  // both the original and the processed version with preproc_
  // ProcessExistingRawFiles(_writeLocationDir);
}

MicDataProcessor::~MicDataProcessor()
{
  speex_resampler_destroy(_speexState);
  MMIfDestroy();
}

void MicDataProcessor::CollectRawAudio(const RawAudioChunk& audioChunk)
{
  constexpr uint32_t samplesToCopy = kSamplesPerChunkIncoming * kNumInputChannels;
  AudioUtil::AudioChunk newChunk;
  newChunk.resize(samplesToCopy);
  std::copy(audioChunk, audioChunk + samplesToCopy, newChunk.begin());
  _rawAudioData.push_back(std::move(newChunk));
}

void MicDataProcessor::CollectResampledAudio(const ResampledAudioChunk& audioChunk)
{
  constexpr uint32_t samplesToCopy = kSamplesPerChunkForSE * kNumInputChannels;
  AudioUtil::AudioChunk newChunk;
  newChunk.resize(samplesToCopy);
  std::copy(audioChunk.begin(), audioChunk.end(), newChunk.begin());
  _resampledAudioData.push_back(std::move(newChunk));
}

void MicDataProcessor::ProcessRawAudio(const ResampledAudioChunk& audioChunk)
{
  // Uninterleave the chunks when copying out of the payload, since that's what SE wants
  for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerChunkForSE; ++sampleIdx)
  {
    const uint32_t interleaveBase = (sampleIdx * kNumInputChannels);
    for (uint32_t channelIdx = 0; channelIdx < kNumInputChannels; ++channelIdx)
    {
      uint32_t dataOffset = _inProcessAudioBlockFirstHalf ? 0 : kSamplesPerChunkForSE;
      const uint32_t uninterleaveBase = (channelIdx * kSamplesPerBlock) + dataOffset;
      _inProcessAudioBlock.data()[sampleIdx + uninterleaveBase] = audioChunk[channelIdx + interleaveBase];
    }
  }
  
  // If we aren't starting a block, we're finishing it - time to convert to a single channel
  if (!_inProcessAudioBlockFirstHalf && _audioSamplesToCollect != 0)
  {
    // Process the current audio block with SE software
    // static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
    // AudioUtil::AudioChunk processedBlock;
    // processedBlock.resize(kSamplesPerBlock);
    // MMIfProcessMicrophones(dummySpeakerOut.data(), _inProcessAudioBlock.data(), processedBlock.data());
    
    if (kSaveAudio && _audioSamplesToCollect != 0)
    {
      // _processedAudioData.push_back(std::move(processedBlock));
      _collectedAudioSamples += kSamplesPerBlock;
    }
  }
  
  _inProcessAudioBlockFirstHalf = !_inProcessAudioBlockFirstHalf;
}

void MicDataProcessor::ResampleAudioChunk(const RawAudioChunk& audioChunk, ResampledAudioChunk& out_resampledAudio)
{
  uint32_t numSamplesProcessed = kSamplesPerChunkIncoming;
  uint32_t numSamplesWritten = kSamplesPerChunkForSE;
  speex_resampler_process_interleaved_int(_speexState, 
                                          audioChunk, &numSamplesProcessed, 
                                          out_resampledAudio.data(), &numSamplesWritten);
  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesProcessed",
              "Expected %d processed only processed %d", kSamplesPerChunkIncoming, numSamplesProcessed);

  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesWritten",
              "Expected %d written only wrote %d", kSamplesPerChunkForSE, numSamplesWritten);
}

void MicDataProcessor::ProcessNextAudioChunk(const RawAudioChunk& audioChunk)
{
  if (kSaveRawAudio && _audioSamplesToCollect != 0)
  {
    CollectRawAudio(audioChunk);
  }
  ResampledAudioChunk resampledAudioChunk;
  ResampleAudioChunk(audioChunk, resampledAudioChunk);
  if (kSaveResampledAudio && _audioSamplesToCollect != 0)
  {
    CollectResampledAudio(resampledAudioChunk);
  }

  ProcessRawAudio(resampledAudioChunk);

  if (_audioSamplesToCollect != 0 && _collectedAudioSamples >= _audioSamplesToCollect)
  {
    std::string deletedFile = "";
    std::string nextFileNameBase = ChooseAndClearNextFileNameBase(deletedFile);
    
    if (!deletedFile.empty())
    {
      Util::FileUtils::DeleteFile(GetRawFileNameFromProcessed(deletedFile));
      Util::FileUtils::DeleteFile(GetResampleFileNameFromProcessed(deletedFile));
    }
    
    std::string writeLocationBase = Util::FileUtils::FullFilePath({ _writeLocationDir, nextFileNameBase });
    
    if (!_rawAudioData.empty())
    {
      if(_runFFTOnRecordedData)
      {
        // Create a packaged task to run fft asynchronously on a copy of the raw mic data
        // The result of the fft will be in availible in a future at some point
        std::packaged_task<std::vector<uint32_t>()> fftTask([data = _rawAudioData]() {
          std::vector<uint32_t> perChannelFFT;

          // Run a seperate fft for each of the channels/mics
          for(u8 i = 0; i < kNumInputChannels; ++i)
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
            // Use every other to effectively scale the data to the appropriate range. 2 because we recorded 2
            // seconds of data
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
            perChannelFFT.push_back(largestValueIdx/2);
          }

          return perChannelFFT;
        });

        // Get the future that will contain the result of the fft task
        _fftFuture = fftTask.get_future();
        std::thread(std::move(fftTask)).detach();
      }
      _runFFTOnRecordedData = false;

      auto saveRawWave = [dest = (_saveFilesPath + kRawFileExtension), data = std::move(_rawAudioData)] () {
        AudioUtil::WaveFile::SaveFile(dest, data, kNumInputChannels, kSampleRateIncoming_hz);
        PRINT_NAMED_INFO("MicDataProcessor.WriteRawWaveFile", "%s", dest.c_str());
      };
      std::thread(saveRawWave).detach();

      _rawAudioData.clear();
    }

    // if (!_resampledAudioData.empty())
    // {
    //   auto saveResampledWave = [dest = (writeLocationBase + kResampledFileExtension), data = std::move(_resampledAudioData)] () {
    //     AudioUtil::WaveFile::SaveFile(dest, data, kNumInputChannels);
    //     PRINT_NAMED_INFO("MicDataProcessor.WriteResampledWaveFile", "%s", dest.c_str());
    //   };
    //   std::thread(saveResampledWave).detach();
    //   _resampledAudioData.clear();
    // }

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
    _audioSamplesToCollect = 0;
  }
}

void MicDataProcessor::ProcessExistingRawFiles(const std::string& micDataDir)
{
  // const bool useFullPath = false;
  // auto fileList = Util::FileUtils::FilesInDirectory(micDataDir, useFullPath, kRawFileExtension.c_str());
  
  // // Remove files starting with prefix
  // auto listIter = fileList.begin();
  // while (listIter != fileList.end())
  // {
  //   if (listIter->compare(0, kPreProcPrefix.length(), kPreProcPrefix) == 0)
  //   {
  //     listIter = fileList.erase(listIter);
  //   }
  //   else
  //   {
  //     listIter++;
  //   }
  // }
  
  // for (const auto& fileName : fileList)
  // {
  //   const std::string& processedFileName = GetProcessedFileNameFromRaw(fileName);
  //   // Skip files that have already been processed
  //   if (Util::FileUtils::FileExists(Util::FileUtils::FullFilePath({micDataDir, processedFileName})))
  //   {
  //     continue;
  //   }
  //   const std::string& originalFilePath = Util::FileUtils::FullFilePath({micDataDir, fileName});
  //   auto audioChunkList = AudioUtil::WaveFile::ReadFile(originalFilePath, kSamplesPerChunkForSE);
  //   for (const auto& chunk : audioChunkList)
  //   {
  //     ProcessRawAudio(chunk.data());
  //   }
  //   // If there's no processed data (like when processing is disabled) don't bother making empty files
  //   if (!_processedAudioData.empty())
  //   {
  //     std::string destFileName = Util::FileUtils::FullFilePath({micDataDir, kPreProcPrefix + processedFileName});
  //     auto saveProcessedWave = [dest = std::move(destFileName), data = std::move(_processedAudioData)] () {
  //       AudioUtil::WaveFile::SaveFile(dest, data);
  //       PRINT_NAMED_INFO("ProcessNextAudioChunk.WriteProcessedWaveFile", "%s", dest.c_str());
  //     };
  //     std::thread(saveProcessedWave).detach();
  //     _processedAudioData.clear();
      
  //     // now rename the original so it matches
  //     const std::string& newFilePath = Util::FileUtils::FullFilePath({micDataDir, kPreProcPrefix + fileName});
  //     std::rename(originalFilePath.c_str(), newFilePath.c_str());
  //   }
  // }
}

// Since the local time on the robot is not reliable (especially over multiple reboots) we use a 
// 2 part numerical naming convention to identify the oldest file and overwrite it. The first number
// indicates which iteration of the file it is (number of times overwritten) and the second number indicates
// the sequence of that file. This way a simple sort reveals the oldest file next in line to be overwritten.
// This also deletes the original file that needs to be overwritten, and sets the name of that file to output.
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
    static const auto& rawExtensionLen = kRawFileExtension.length();
    if (iterLen > rawExtensionLen && 
        listIter->compare(iterLen - rawExtensionLen, rawExtensionLen, kRawFileExtension) == 0)
    {
      listIter = fileNames.erase(listIter);
      continue;
    }

    // Also remove files that end in the _resamp postfix
    static const auto& resampExtensionLen = kResampledFileExtension.length();
    if (iterLen > resampExtensionLen && 
        listIter->compare(iterLen - resampExtensionLen, resampExtensionLen, kResampledFileExtension) == 0)
    {
      listIter = fileNames.erase(listIter);
      continue;
    }

    listIter++;
  }
  
  // If number of files is less than max, name the file miccapture_0000_(filecount())
  // if (fileNames.size() < _filesToStore)
  // {
    static int i = 0;
    std::ostringstream newNameStream;
    newNameStream << kMicCapturePrefix << "0000_" << std::setfill('0') << std::setw(4) << i;
    i++;
    return newNameStream.str();
  // }
  
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
  {
    std::ostringstream newNameStream;
    newNameStream << kMicCapturePrefix << std::setfill('0') << std::setw(4) << (iterationNum + 1) << "_" << seqStr;
    return newNameStream.str();
  }
}

std::string MicDataProcessor::GetProcessedFileNameFromRaw(const std::string& rawFileName)
{
  return rawFileName.substr(0, rawFileName.length() - kRawFileExtension.length()) + kWavFileExtension;
}

std::string MicDataProcessor::GetRawFileNameFromProcessed(const std::string& processedName)
{
  return processedName.substr(0, processedName.length() - kWavFileExtension.length()) + kRawFileExtension;
}

std::string MicDataProcessor::GetResampleFileNameFromProcessed(const std::string& processedName)
{
  return processedName.substr(0, processedName.length() - kWavFileExtension.length()) + kResampledFileExtension;
}

void MicDataProcessor::RecordAudio(uint32_t duration_ms, std::string path, bool runFFT)
{ 
  if(_audioSamplesToCollect != 0)
  {
    PRINT_NAMED_WARNING("MicDataProcessor.RecordAudio.AlreadyRecording", "");
    return;
  }

  PRINT_NAMED_INFO("MicDataProcessor.RecordAudio",
                   "Recording the next %u ms of audio",
                   duration_ms);

  _audioSamplesToCollect = AudioUtil::kSampleRate_hz * (duration_ms / 1000);
  
  _rawAudioData.clear();

  if(kSaveResampledAudio)
  {
    _resampledAudioData.clear();
  }

  if(kSaveAudio)
  {
    _processedAudioData.clear();
  }

  _collectedAudioSamples = 0;

  if(!path.empty())
  {
    _saveFilesPath = path;
  }

  _runFFTOnRecordedData = runFFT;
}

void MicDataProcessor::Update()
{
  // Wait until the future is valid and ready
  if(_fftFuture.valid() && 
     _fftFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
  {
    // Get the result from the future and populate the fft result message
    auto msg = RobotInterface::AudioFFTResult();
    std::vector<uint32_t> res = _fftFuture.get();

    for(u8 i = 0; i < res.size(); ++i)
    {
      msg.result[i] = res[i];
    }
    RobotInterface::SendMessageToEngine(std::move(msg));
  }
}

} // namespace Cozmo
} // namespace Anki
