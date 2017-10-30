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
#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/micDataInfo.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"

#include <iomanip>
#include <sstream>

namespace Anki {
namespace Cozmo {
namespace MicData {

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
  
  
  // Enable this to test a repeating recording job.
  static constexpr bool enableCircularRecordingJob = false;
  if (enableCircularRecordingJob)
  {
    MicDataInfo* newJob = new MicDataInfo{};
    newJob->_typesToRecord.SetBitFlag(MicDataType::Raw, true);
    newJob->_typesToRecord.SetBitFlag(MicDataType::Resampled, true);
    newJob->_typesToRecord.SetBitFlag(MicDataType::Processed, true);
    newJob->_writeLocationDir = _writeLocationDir;
    newJob->_timeToRecord_ms = kSecondsPerFile * 1000;
    newJob->_doFFTProcess = false;
    newJob->_repeating = true;
    {
      std::lock_guard<std::mutex> lock(_dataRecordJobMutex);
      _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
    }
  }
  
  _processThread = std::thread(&MicDataProcessor::ProcessLoop, this);
}

MicDataProcessor::~MicDataProcessor()
{
  _processThreadStop = true;
  _processThread.join();

  speex_resampler_destroy(_speexState);
  MMIfDestroy();
}

AudioUtil::AudioChunk MicDataProcessor::ProcessResampledAudio(const AudioUtil::AudioChunk& audioChunk)
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
  
  AudioUtil::AudioChunk processedBlock;
  // If we aren't starting a block, we're finishing it - time to convert to a single channel
  if (!_inProcessAudioBlockFirstHalf)
  {
    ANKI_CPU_PROFILE("ProcessMicrophonesSE");
    // Process the current audio block with SE software
    static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
    processedBlock.resize(kSamplesPerBlock);
    MMIfProcessMicrophones(dummySpeakerOut.data(), _inProcessAudioBlock.data(), processedBlock.data());
  }
  
  _inProcessAudioBlockFirstHalf = !_inProcessAudioBlockFirstHalf;
  
  return processedBlock;
}

AudioUtil::AudioChunk MicDataProcessor::ResampleAudioChunk(const AudioUtil::AudioChunk& audioChunk)
{
  ANKI_CPU_PROFILE("ResampleAudioChunk");
  AudioUtil::AudioChunk resampledAudioChunk;
  resampledAudioChunk.resize(kResampledAudioChunkSize);
  uint32_t numSamplesProcessed = kSamplesPerChunkIncoming;
  uint32_t numSamplesWritten = kSamplesPerChunkForSE;
  speex_resampler_process_interleaved_int(_speexState, 
                                          audioChunk.data(), &numSamplesProcessed, 
                                          resampledAudioChunk.data(), &numSamplesWritten);
  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesProcessed",
              "Expected %d processed only processed %d", kSamplesPerChunkIncoming, numSamplesProcessed);

  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesWritten",
              "Expected %d written only wrote %d", kSamplesPerChunkForSE, numSamplesWritten);
  return resampledAudioChunk;
}

void MicDataProcessor::ProcessLoop()
{
  static constexpr uint32_t expectedAudioDropsPerAnimLoop = 7;
  static constexpr uint32_t maxProcessingTimePerDrop_ms = 5;
  static constexpr uint32_t maxProcTime_ms = expectedAudioDropsPerAnimLoop * maxProcessingTimePerDrop_ms;
  const auto maxProcTime = std::chrono::milliseconds(maxProcTime_ms);
  while (!_processThreadStop)
  {
    const auto start = std::chrono::steady_clock::now();
  
    _resampleMutex.lock();
    while (_rawAudioToProcess.size() > 0)
    {
      ANKI_CPU_TICK("MicDataProcessor::ProcessLoop", maxProcessingTimePerDrop_ms, Util::CpuThreadProfiler::kLogFrequencyNever);
      auto audioChunk = std::move(_rawAudioToProcess.front());
      _rawAudioToProcess.pop_front();
      _resampleMutex.unlock();
      
      // Steal the current set of jobs we have for recording audio, so the list can be added to while processing
      // continues
      std::deque<std::shared_ptr<MicDataInfo>> stolenJobs;
      {
        std::lock_guard<std::mutex> lock(_dataRecordJobMutex);
        stolenJobs = std::move(_micProcessingJobs);
        _micProcessingJobs.clear();
      }

      // Collect the raw audio if desired
      for (auto& job : stolenJobs)
      {
        job->CollectRawAudio(audioChunk);
      }
      
      // Resample the audio, then collect it if desired
      AudioUtil::AudioChunk resampledAudioChunk = ResampleAudioChunk(audioChunk);
      for (auto& job : stolenJobs)
      {
        job->CollectResampledAudio(resampledAudioChunk);
      }
      
      // Process the audio into a single channel, and collect it if desired
      AudioUtil::AudioChunk processedAudio = ProcessResampledAudio(resampledAudioChunk);
      if (!processedAudio.empty())
      {
        for (auto& job : stolenJobs)
        {
          job->CollectProcessedAudio(processedAudio);
        }
      }
      
      // Check if each of the jobs are done, removing the ones that are
      auto jobIter = stolenJobs.begin();
      while (jobIter != stolenJobs.end())
      {
        bool jobDone = (*jobIter)->CheckDone();
        if (jobDone)
        {
          jobIter = stolenJobs.erase(jobIter);
        }
        else
        {
          ++jobIter;
        }
      }
      
      // Add back the remaining stolen jobs
      {
        std::lock_guard<std::mutex> lock(_dataRecordJobMutex);
        _micProcessingJobs.insert(_micProcessingJobs.begin(), stolenJobs.begin(), stolenJobs.end());
      }
      
      // Locking at the end since we will be checking remaining items in the loop condition
      _resampleMutex.lock();
    }
    _resampleMutex.unlock();

    const auto end = std::chrono::steady_clock::now();
    const auto elapsedTime = (end - start);
    if (elapsedTime < maxProcTime)
    {
      std::this_thread::sleep_for(maxProcTime - elapsedTime);
    }
  }
}
  
void MicDataProcessor::ProcessNextAudioChunk(const RawAudioChunk& audioChunk)
{
  // Store off this next job
  AudioUtil::AudioChunk nextJob;
  nextJob.resize(kRawAudioChunkSize);
  std::copy(audioChunk, audioChunk + kRawAudioChunkSize, nextJob.begin());

  {
    std::lock_guard<std::mutex> lock(_resampleMutex);
    _rawAudioToProcess.push_back(std::move(nextJob));
  }
}

void MicDataProcessor::RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT)
{
  MicDataInfo* newJob = new MicDataInfo{};
  
  // If the input path has a file separator, remove the filename at the end and use as the write directory
  if (path.find('/') != std::string::npos)
  {
    std::string nameBase = Util::FileUtils::GetFileName(path);
    newJob->_writeLocationDir = path.substr(0, path.length() - nameBase.length());
    newJob->_writeNameBase = nameBase;
  }
  else
  {
    // otherwise used the saved off write directory, and just the input path as the name base
    newJob->_writeLocationDir = _writeLocationDir;
    newJob->_writeNameBase = path;
  }
  
  newJob->_typesToRecord.SetBitFlag(MicDataType::Raw, true);
  newJob->_timeToRecord_ms = duration_ms;
  newJob->_doFFTProcess = runFFT;
  if (runFFT)
  {
    newJob->_rawAudioFFTCallback = [this] (std::vector<uint32>&& result) {
      std::lock_guard<std::mutex> _lock(_fftResultMutex);
      _fftResultList.push_back(std::move(result));
    };
  }
  
  {
    std::lock_guard<std::mutex> lock(_dataRecordJobMutex);
    _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
  }
}

void MicDataProcessor::Update()
{
  _fftResultMutex.lock();
  while (_fftResultList.size() > 0)
  {
    auto result = std::move(_fftResultList.front());
    _fftResultList.pop_front();
    _fftResultMutex.unlock();
    
    // Populate the fft result message
    auto msg = RobotInterface::AudioFFTResult();

    for(uint8_t i = 0; i < result.size(); ++i)
    {
      msg.result[i] = result[i];
    }
    RobotInterface::SendMessageToEngine(std::move(msg));


    _fftResultMutex.lock();
  }
  _fftResultMutex.unlock();
}

} // namespace MicData
} // namespace Cozmo
} // namespace Anki
