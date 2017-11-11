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

#include "anki/messaging/shared/UdpServer.h"
#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/micDataInfo.h"
#include "cozmoAnim/speechRecognizerTHFSimple.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"

#include <iomanip>
#include <sstream>

namespace {
  const std::string kHeyCozmoDataDir = "trigger_anki_x_en_us_01_hey_cosmo_sfs14_b3e3cbba";
  const std::string kHeyCozmoNet = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_am.raw";
  const std::string kHeyCozmoSearch = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_search_4.raw";

  const unsigned short kCloudProcessCommunicationPort = 9880;
}

namespace Anki {
namespace Cozmo {
namespace MicData {

MicDataProcessor::MicDataProcessor(const std::string& writeLocation, const std::string& triggerWordDataDir)
: _writeLocationDir(writeLocation)
, _recognizer(new SpeechRecognizerTHF())
, _udpServer(new UdpServer())
{
  if (!_writeLocationDir.empty())
  {
    Util::FileUtils::CreateDirectory(_writeLocationDir);
  }

  const std::string& pronunciationFileToUse = "";
  (void) _recognizer->Init(pronunciationFileToUse);

  const std::string& netFilePath = Util::FileUtils::FullFilePath({triggerWordDataDir, kHeyCozmoDataDir, kHeyCozmoNet});
  const std::string& searchFilePath = Util::FileUtils::FullFilePath({triggerWordDataDir, kHeyCozmoDataDir, kHeyCozmoSearch});
  const AudioUtil::SpeechRecognizer::IndexType searchIndex = 0;
  const bool isPhraseSpotted = true;
  const bool allowsFollowUpRecog = false;
  const bool success = _recognizer->AddRecognitionDataFromFile(searchIndex, netFilePath, searchFilePath,
                                                               isPhraseSpotted, allowsFollowUpRecog);
  DEV_ASSERT_MSG(success,
                 "MicDataProcessor.Constructor.SpeechRecognizerInit",
                 "Failed to add speechRecognizer search");
  _recognizer->SetRecognizerIndex(searchIndex);
  // Set up the callback that creates the recording job when the trigger is detected
  _recognizer->SetCallback([this] (const char* resultFound, float score) {
    {
      std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
      // Ignore extra triggers during streaming
      if (nullptr != _currentStreamingJob)
      {
        return;
      }
    }

    MicDataInfo* newJob = new MicDataInfo{};
    newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "triggeredCapture"});
    newJob->_writeNameBase = ""; // Use the autogen names in this subfolder
    newJob->_numMaxFiles = 100;
    newJob->_typesToRecord.SetBitFlag(MicDataType::Processed, true);
    newJob->SetTimeToRecord(MicDataInfo::kMaxRecordTime_ms);

    // Copy the current audio chunks in the trigger overlap buffer into 
    for (const auto& audioChunk : _triggerOverlapBuffer)
    {
      newJob->CollectProcessedAudio(audioChunk);
    }

    {
      std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
      _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
      _currentStreamingJob = _micProcessingJobs.back();
    }
  });
  _recognizer->Start();

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
    newJob->SetTimeToRecord(kSecondsPerFile * 1000);
    newJob->_doFFTProcess = false;
    newJob->_repeating = true;
    {
      std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
      _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
    }
  }

  const bool udpSuccess = _udpServer->StartListening(kCloudProcessCommunicationPort);
  ANKI_VERIFY(udpSuccess,
              "MicDataProcessor.Constructor.UdpStartListening",
              "Failed to start listening on port %d",
              kCloudProcessCommunicationPort);
  
  _processThread = std::thread(&MicDataProcessor::ProcessLoop, this);
}

MicDataProcessor::~MicDataProcessor()
{
  _processThreadStop = true;
  _processThread.join();

  _udpServer->StopListening();

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
        std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
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

        // Make a copy to our trigger overlap buffer
        {
          AudioUtil::AudioChunk newChunk;
          newChunk.resize(kSamplesPerBlock);
          std::copy(processedAudio.begin(), processedAudio.end(), newChunk.begin());
          _triggerOverlapBuffer.push_back(std::move(newChunk));
        }
        if ((_triggerOverlapBuffer.size() * kChunksPerSEBlock * kTimePerChunk_ms) >= kTriggerOverlapSize_ms)
        {
          _triggerOverlapBuffer.pop_front();
        }

        // Run the trigger detection, which will use the callback defined above
        {
          ANKI_CPU_PROFILE("RecognizeTriggerWord");
          _recognizer->Update(processedAudio.data(), (unsigned int)processedAudio.size());
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
        std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
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
  newJob->SetTimeToRecord(duration_ms);
  newJob->_doFFTProcess = runFFT;
  if (runFFT)
  {
    newJob->_rawAudioFFTCallback = [this] (std::vector<uint32>&& result) {
      std::lock_guard<std::mutex> _lock(_fftResultMutex);
      _fftResultList.push_back(std::move(result));
    };
  }
  
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
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
    
    // Do something with the result
    PRINT_NAMED_INFO("MicDataProcessor.Update.FFTResult", "%d %d %d %d", result[0], result[1], result[2], result[3]);
    
    _fftResultMutex.lock();
  }
  _fftResultMutex.unlock();

  bool receivedStopMessage = false;
  static constexpr int kMaxReceiveBytes = 2000;
  char receiveArray[kMaxReceiveBytes];
  
  const int bytesReceived = _udpServer->Recv(receiveArray, kMaxReceiveBytes);
  if (bytesReceived == 2)
  {
    if (receiveArray[0] != '\0' || receiveArray[1] != '\0')
    {
      PRINT_NAMED_INFO("MicDataProcessor.Update.RecvCloudProcess.UnexpectedSignal", "0x%x 0x%x", receiveArray[0], receiveArray[1]);
    }
    else
    {
      PRINT_NAMED_INFO("MicDataProcessor.Update.RecvCloudProcess.StopSignal", "");
    }
    receivedStopMessage = true;
  }

  static size_t streamingAudioIndex = 0;
  // lock the job mutex
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    // check if the pointer to the currently streaming job is valid
    if (!_currentlyStreaming && _currentStreamingJob != nullptr)
    {
      if (_udpServer->GetNumClients() > 0)
      {
        _currentlyStreaming = true;
        streamingAudioIndex = 0;
  
        // Send out the message announcing the trigger word has been detected
        static const char* const hotwordSignal = "hotword";
        static const size_t hotwordSignalLen = std::strlen(hotwordSignal) + 1;
        _udpServer->Send(hotwordSignal, Util::numeric_cast<int>(hotwordSignalLen));
        PRINT_NAMED_INFO("MicDataProcessor.Update.StreamingStart", "");
        RobotInterface::SendMessageToEngine(RobotInterface::TriggerWordDetected());
      }
      else
      {
        ClearCurrentStreamingJob();
        PRINT_NAMED_INFO("MicDataProcessor.Update.StreamingStartIgnored", "Ignoring stream start as no clients connected.");
      }
    }

    if (_currentlyStreaming)
    {
      // Are we done with what we want to stream?
      static constexpr size_t kMaxRecordTime_ms = 10000;
      static constexpr size_t kMaxRecordNumChunks = (kMaxRecordTime_ms / (kTimePerChunk_ms * kChunksPerSEBlock)) + 1;
      if (receivedStopMessage || streamingAudioIndex >= kMaxRecordNumChunks)
      {
        ClearCurrentStreamingJob();
        PRINT_NAMED_INFO("MicDataProcessor.Update.StreamingEnd", "%zu ms", streamingAudioIndex * kTimePerChunk_ms * kChunksPerSEBlock);
      }
      else
      {
        // Copy any new data that has been pushed onto the currently streaming job
        AudioUtil::AudioChunkList newAudio = _currentStreamingJob->GetProcessedAudio(streamingAudioIndex);
        streamingAudioIndex += newAudio.size();
    
        // Send the audio to any clients we've got
        if (_udpServer->GetNumClients() > 0)
        {
          for(const auto& audioChunk : newAudio)
          {
            const auto entrySize = !audioChunk.empty() ? sizeof(audioChunk[0]) : 0;
            _udpServer->Send((char*)audioChunk.data(), Util::numeric_cast<int>(audioChunk.size() * entrySize));
          }
        }
      }
    }
  }
}

void MicDataProcessor::ClearCurrentStreamingJob()
{
  std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
  _currentlyStreaming = false;
  if (_currentStreamingJob != nullptr)
  {
    _currentStreamingJob->SetTimeToRecord(0);
    _currentStreamingJob = nullptr;
  }
}

} // namespace MicData
} // namespace Cozmo
} // namespace Anki
