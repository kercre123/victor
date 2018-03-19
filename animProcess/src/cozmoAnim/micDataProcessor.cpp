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


// Signal Essence Includes
#include "mmif.h"
#include "policy_actions.h"
#include "se_diag.h"

#include "coretech/messaging/shared/LocalUdpServer.h"

#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/faceDisplay/faceDebugDraw.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/micDataInfo.h"
#include "cozmoAnim/micImmediateDirection.h"
#include "cozmoAnim/speechRecognizerTHFSimple.h"

#include "osState/osState.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/threading/threadPriority.h"

#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"

#include <iomanip>
#include <sstream>


namespace {
  struct TriggerData
  {
    std::string dataDir;
    std::string netFile;
    std::string searchFile;
  };

  const TriggerData kTriggerDataList[] = 
  {
    // "HeyCozmo" trigger trained on adults
    {
      .dataDir = "trigger_anki_x_en_us_01_hey_cosmo_sfs14_b3e3cbba",
      .netFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_am.raw",
      .searchFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_search_4.raw"
    },
    // "HeyCozmo" trigger trained on both adults + kids (aka delivery 2)
    {
      .dataDir = "trigger_anki_x_en_us_02_hey_cosmo_sfs14_b3e3cbba",
      .netFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery02_am.raw",
      .searchFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery02_search_4.raw"
    },
    // "Cozmo" trigger trained on both adults + kids
    {
      .dataDir = "trigger_anki_x_en_us_01_cosmo_sfs14_b3e3cbba",
      .netFile = "anki_x_cosmo_en_us_sfs14_b3e3cbba_delivery01_am.raw",
      .searchFile = "anki_x_cosmo_en_us_sfs14_b3e3cbba_delivery01_search_10.raw"
    }
  };
  constexpr int32_t kTriggerDataListLen = (int32_t) sizeof(kTriggerDataList) / sizeof(kTriggerDataList[0]);
  Anki::AudioUtil::SpeechRecognizer::IndexType _currentTriggerSearchIndex = 0;

# define CONSOLE_GROUP "MicData"
  CONSOLE_VAR_RANGED(s32, kMicData_NextTriggerIndex, CONSOLE_GROUP, 0, 0, kTriggerDataListLen-1);
# undef CONSOLE_GROUP
}

namespace Anki {
namespace Cozmo {
namespace MicData {

static_assert(std::is_same<decltype(RobotInterface::MicData::data), RawAudioChunk>::value,
              "Expecting type of MicData::data to match RawAudioChunk");

static_assert(
  std::is_same<std::remove_reference<decltype(RobotInterface::MicDirection::confidenceList[0])>::type,
  decltype(MicDirectionData::confidenceList)::value_type>::value,
  "Expecting type of RobotInterface::MicDirection::confidenceList items "\
  "to match MicDirectionData::confidenceList items");

constexpr auto kMicDirectionConfListSize = sizeof(RobotInterface::MicDirection::confidenceList);
constexpr auto kMicDirectionConfListItemSize = sizeof(RobotInterface::MicDirection::confidenceList[0]);
static_assert(
  kMicDirectionConfListSize / kMicDirectionConfListItemSize ==
  decltype(MicDirectionData::confidenceList)().size(),
  "Expecting length of RobotInterface::MicDirection::confidenceList to match MicDirectionData::confidenceList");


MicDataProcessor::MicDataProcessor(const std::string& writeLocation, const std::string& triggerWordDataDir)
: _writeLocationDir(writeLocation)
, _recognizer(new SpeechRecognizerTHF())
, _udpServer(new LocalUdpServer())
, _micImmediateDirection(new MicImmediateDirection())
{
  if (!_writeLocationDir.empty())
  {
    Util::FileUtils::CreateDirectory(_writeLocationDir);
  }

  const std::string& pronunciationFileToUse = "";
  (void) _recognizer->Init(pronunciationFileToUse);

  for (int i=0; i < kTriggerDataListLen; ++i)
  {
    const auto& data = kTriggerDataList[i];
    const std::string& netFilePath = Util::FileUtils::FullFilePath({triggerWordDataDir, data.dataDir, data.netFile});
    const std::string& searchFilePath = Util::FileUtils::FullFilePath({triggerWordDataDir, data.dataDir, data.searchFile});
    const AudioUtil::SpeechRecognizer::IndexType searchIndex = i;
    const bool isPhraseSpotted = true;
    const bool allowsFollowUpRecog = false;
    const bool success = _recognizer->AddRecognitionDataFromFile(searchIndex, netFilePath, searchFilePath,
                                                                isPhraseSpotted, allowsFollowUpRecog);
    DEV_ASSERT_MSG(success,
                  "MicDataProcessor.Constructor.SpeechRecognizerInit",
                  "Failed to add speechRecognizer index: %d netFile: %s searchFile %s",
                  searchIndex, netFilePath.c_str(), searchFilePath.c_str());
  }

  _recognizer->SetRecognizerIndex(_currentTriggerSearchIndex);
  // Set up the callback that creates the recording job when the trigger is detected
  auto triggerCallback = std::bind(&MicDataProcessor::TriggerWordDetectCallback, 
                                   this, std::placeholders::_1, std::placeholders::_2);
  _recognizer->SetCallback(triggerCallback);
  _recognizer->Start();

  MMIfInit(0, nullptr);
  InitVAD();

  _bestSearchBeamIndex = SEDiagGetIndex("fdsearch_best_beam_index");
  _bestSearchBeamConfidence = SEDiagGetIndex("fdsearch_best_beam_confidence");
  _selectedSearchBeamIndex = SEDiagGetIndex("search_result_best_beam_index");
  _selectedSearchBeamConfidence = SEDiagGetIndex("search_result_best_beam_confidence");
  _searchConfidenceState = SEDiagGetIndex("fdsearch_confidence_state");

  const RobotID_t robotID = OSState::getInstance()->GetRobotID();
  const std::string sockName = std::string{LOCAL_SOCKET_PATH} + "mic_sock" + (robotID == 0 ? "" : std::to_string(robotID));
  _udpServer->SetBindClients(false);
  const bool udpSuccess = _udpServer->StartListening(sockName);
  ANKI_VERIFY(udpSuccess,
              "MicDataProcessor.Constructor.UdpStartListening",
              "Failed to start listening on socket %s",
              sockName.c_str());

  _processThread = std::thread(&MicDataProcessor::ProcessRawLoop, this);
  _processTriggerThread = std::thread(&MicDataProcessor::ProcessTriggerLoop, this);
}

void MicDataProcessor::InitVAD()
{
  _sVadConfig.reset(new SVadConfig_t());
  _sVadObject.reset(new SVadObject_t());

  /* set up VAD */
  SVadSetDefaultConfig(_sVadConfig.get(), kSamplesPerBlock, (float)AudioUtil::kSampleRate_hz);
  _sVadConfig->AbsThreshold = 250.0f; // was 400
  _sVadConfig->HangoverCountDownStart = 10;  // was 25, make 25 blocks (1/4 second) to see it actually end a couple times
  SVadInit(_sVadObject.get(), _sVadConfig.get());
}

void MicDataProcessor::TriggerWordDetectCallback(const char* resultFound, float score)
{
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    // Ignore extra triggers during streaming
    if (nullptr != _currentStreamingJob
    #if ANKI_DEV_CHEATS
      || _fakeStreamingState
    #endif
    )
    {
      return;
    }
  }

  MicDataInfo* newJob = new MicDataInfo{};
  newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "triggeredCapture"});
  newJob->_writeNameBase = ""; // Use the autogen names in this subfolder
  newJob->_numMaxFiles = 100;
  newJob->_typesToRecord.SetBitFlag(MicDataType::Processed, true);
  newJob->_typesToRecord.SetBitFlag(MicDataType::Raw, false);
  newJob->SetTimeToRecord(MicDataInfo::kMaxRecordTime_ms);

  TimeStamp_t mostRecentTimestamp;
  {
    std::lock_guard<std::mutex> lock(_procAudioXferMutex);
    DEV_ASSERT(_procAudioRawComplete >= _procAudioXferCount,
               "MicDataProcessor.TriggerWordDetectCallback.AudioProcIdx");
    const auto maxIndex = _procAudioRawComplete - _procAudioXferCount;
    // Copy the current audio chunks in the trigger overlap buffer
    for (size_t i=0; i<maxIndex; ++i)
    {
      const auto& audioBlock = _immediateAudioBuffer[i].audioBlock;
      newJob->CollectProcessedAudio(audioBlock.data(), audioBlock.size());
    }
    mostRecentTimestamp = _immediateAudioBuffer[_procAudioRawComplete-1].timestamp;
  }

  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
    _currentStreamingJob = _micProcessingJobs.back();
  }

  const auto currentDirection = _micImmediateDirection->GetDominantDirection();

  // Set up a message to send out about the triggerword
  RobotInterface::TriggerWordDetected twDetectedMessage;
  twDetectedMessage.timestamp = mostRecentTimestamp;
  twDetectedMessage.direction = currentDirection;
  auto engineMessage = std::unique_ptr<RobotInterface::RobotToEngine>(
    new RobotInterface::RobotToEngine(std::move(twDetectedMessage)));
  {
    std::lock_guard<std::mutex> lock(_msgsMutex);
    _msgsToEngine.push_back(std::move(engineMessage));
  }

  // Tell signal essence software to lock in on the current direction if it's known
  if (currentDirection != kDirectionUnknown)
  {
    std::lock_guard<std::mutex> lock(_seInteractMutex);
    PolicySetKeyPhraseDirection(currentDirection);
  }

  PRINT_NAMED_INFO("MicDataProcessor.TWCallback",
                    "Direction index %d at timestamp %d",
                    currentDirection,
                    mostRecentTimestamp);
}

MicDataProcessor::~MicDataProcessor()
{
  _processThreadStop = true;
  _xferAvailableCondition.notify_all();
  _dataReadyCondition.notify_all();
  _processThread.join();
  _processTriggerThread.join();

  _udpServer->StopListening();

  MMIfDestroy();
  _recognizer->Stop();
}

void MicDataProcessor::ProcessRawAudio(TimeStamp_t timestamp,
                                       const AudioUtil::AudioSample* audioChunk,
                                       uint32_t robotStatus,
                                       float robotAngle)
{
  {
    ANKI_CPU_PROFILE("UninterleaveAudioForSE");
    // Uninterleave the chunks when copying out of the payload, since that's what SE wants
    for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerChunkIncoming; ++sampleIdx)
    {
      const uint32_t interleaveBase = (sampleIdx * kNumInputChannels);
      for (uint32_t channelIdx = 0; channelIdx < kNumInputChannels; ++channelIdx)
      {
        uint32_t dataOffset = _inProcessAudioBlockFirstHalf ? 0 : kSamplesPerChunkIncoming;
        const uint32_t uninterleaveBase = (channelIdx * kSamplesPerBlock) + dataOffset;
        _inProcessAudioBlock[sampleIdx + uninterleaveBase] = audioChunk[channelIdx + interleaveBase];
      }
    }
  }
  
  // If we aren't starting a block, we're finishing it - time to convert to a single channel
  if (!_inProcessAudioBlockFirstHalf)
  {
    TimedMicData* nextSampleSpot = nullptr;
    {
      // Note we don't bother to free any slots here that have been consumed (by comparing size to _procAudioXferCount)
      // because it's unnecessary with the circular buffer.

      std::unique_lock<std::mutex> lock(_procAudioXferMutex);
      auto xferAvailableCheck = [this] () {
        return _processThreadStop || _procAudioXferCount < _immediateAudioBuffer.capacity();
      };
      _xferAvailableCondition.wait(lock, xferAvailableCheck);

      if (_processThreadStop)
      {
        return;
      }

      // Now we can be sure we have a free slot, so go ahead and grab it
      if (_immediateAudioBuffer.size() < _immediateAudioBuffer.capacity())
      {
         _procAudioRawComplete = _immediateAudioBuffer.size();
      }
      else
      {
         _procAudioRawComplete = _immediateAudioBuffer.size() - 1;
      }
      nextSampleSpot = &_immediateAudioBuffer.push_back();
    }

    TimedMicData& nextSample = *nextSampleSpot;
    nextSample.timestamp = timestamp;
    MicDirectionData directionResult = ProcessMicrophonesSE(
      _inProcessAudioBlock.data(),
      nextSample.audioBlock.data(),
      robotStatus,
      robotAngle);

    // Now we're done filling out this slot, update the count so it can be consumed
    {
      std::lock_guard<std::mutex> lock(_procAudioXferMutex);
      ++_procAudioXferCount;
      _procAudioRawComplete = _immediateAudioBuffer.size();
    }
    _dataReadyCondition.notify_all();

    // Store off this most recent result in our immedate direction tracking
    _micImmediateDirection->AddDirectionSample(directionResult);

    // Set up a message to send out about the direction
    RobotInterface::MicDirection newMessage;
    newMessage.timestamp = timestamp;
    newMessage.direction = directionResult.winningDirection;
    newMessage.confidence = directionResult.winningConfidence;
    newMessage.selectedDirection = directionResult.selectedDirection;
    newMessage.selectedConfidence = directionResult.selectedConfidence;
    newMessage.activeState = directionResult.activeState;
    std::copy(
      directionResult.confidenceList.begin(),
      directionResult.confidenceList.end(),
      newMessage.confidenceList);
    
    auto engineMessage = std::unique_ptr<RobotInterface::RobotToEngine>(
      new RobotInterface::RobotToEngine(std::move(newMessage)));
    {
      std::lock_guard<std::mutex> lock(_msgsMutex);
      _msgsToEngine.push_back(std::move(engineMessage));
    }
  }
  
  _inProcessAudioBlockFirstHalf = !_inProcessAudioBlockFirstHalf;
}

MicDirectionData MicDataProcessor::ProcessMicrophonesSE(const AudioUtil::AudioSample* audioChunk,
                                                        AudioUtil::AudioSample* bufferOut,
                                                        uint32_t robotStatus,
                                                        float robotAngle)
{
  std::lock_guard<std::mutex> lock(_seInteractMutex);
  PolicySetAbsoluteOrientation(robotAngle);
  // Note that currently we are only monitoring the moving flag. We _could_ also discard mic data when the robot
  // is picked up, but that is being evaluated with design before implementation, see VIC-1219
  const bool robotIsMoving = static_cast<bool>(robotStatus & (uint16_t)RobotStatusFlag::IS_MOVING);
  const bool robotStoppedMoving = !robotIsMoving && _robotWasMoving;
  _robotWasMoving = robotIsMoving;
  if (robotStoppedMoving)
  {
    // When the robot has stopped moving (and the gears are no longer making noise) we reset the mic direciton
    // confidence values to be based on non-noisy data
    MMIfResetLocationSearch();
  }

  // We only care about checking one channel, and since the channel data is uninterleaved when passed in here,
  // we simply give the start of the buffer as the input to run the vad detection on
  int activityFlag = 0;
  {
    ANKI_CPU_PROFILE("ProcessVAD");

    // Note while we _can_ pass a confidence value here adjusted while the robot is moving, we'd rather err on the side
    // of always thinking we hear a voice when the robot moves, so we maximize our chances of hearing any triggers
    // over the noise. So when the robot is moving, don't even bother running the VAD, and instead just set activity
    // to true.
    if (robotIsMoving)
    {
      activityFlag = 1;
    }
    else
    {
      const float vadConfidence = 1.0f;
      activityFlag = DoSVad(_sVadObject.get(),           // object
                            vadConfidence,               // confidence it is okay to measure noise floor, i.e. no known activity like gear noise
                            (int16_t*)audioChunk);       // pointer to input data
    }
  }

  static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
  {
    ANKI_CPU_PROFILE("ProcessMicrophonesSE");
    // Process the current audio block with SE software
    MMIfProcessMicrophones(dummySpeakerOut.data(), audioChunk, bufferOut);
  }

  MicDirectionData result{};
  result.activeState = activityFlag;

  if (robotIsMoving)
  {
    result.winningDirection = result.selectedDirection = kDirectionUnknown;
  }
  else
  {
    const auto latestDirection = SEDiagGetUInt16(_bestSearchBeamIndex);
    const auto latestConf = SEDiagGetInt16(_bestSearchBeamConfidence);
    const auto selectedDirection = SEDiagGetUInt16(_selectedSearchBeamIndex);
    const auto selectedConf = SEDiagGetInt16(_selectedSearchBeamConfidence);
    const auto* searchConfState = SEDiagGet(_searchConfidenceState);
    result.winningDirection = latestDirection;
    result.winningConfidence = latestConf;
    result.selectedDirection = selectedDirection;
    result.selectedConfidence = selectedConf;
    const auto* confListSrc = reinterpret_cast<const float*>(searchConfState->u.vp);
    // NOTE currently SE only calculates the 12 main directions (not "unknown" or directly above the mics)
    // so we only copy the 12 main directions
    std::copy(confListSrc, confListSrc + kLastValidIndex + 1, result.confidenceList.begin());
  }
  return result;
}

void MicDataProcessor::ProcessRawLoop()
{
  Anki::Util::SetThreadName(pthread_self(), "MicProcRaw");
  static constexpr uint32_t expectedAudioDropsPerAnimLoop = 7;
  static constexpr uint32_t maxProcessingTimePerDrop_ms = 5;
  static constexpr uint32_t maxProcTime_ms = expectedAudioDropsPerAnimLoop * maxProcessingTimePerDrop_ms;
  const auto maxProcTime = std::chrono::milliseconds(maxProcTime_ms);
  while (!_processThreadStop)
  {
    const auto start = std::chrono::steady_clock::now();
  
    // Switch which buffer we're processing if it's empty
    {
      std::lock_guard<std::mutex> lock(_rawMicDataMutex);
      if (_rawAudioBuffers[_rawAudioProcessingIndex].empty())
      {
        _rawAudioProcessingIndex = (_rawAudioProcessingIndex == 1) ? 0 : 1;
      }
    }

    auto& rawAudioToProcess = _rawAudioBuffers[_rawAudioProcessingIndex];
    while (rawAudioToProcess.size() > 0)
    {
      ANKI_CPU_TICK("MicDataProcessorRaw", maxProcessingTimePerDrop_ms, Util::CpuThreadProfiler::kLogFrequencyNever);
      ANKI_CPU_PROFILE("ProcessLoop");

      const auto& nextData = rawAudioToProcess.front();
      const auto* audioChunk = nextData.data;
      
      // Copy the current set of jobs we have for recording audio, so the list can be added to while processing
      // continues
      std::deque<std::shared_ptr<MicDataInfo>> jobs;
      {
        std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
        jobs = _micProcessingJobs;
      }
      // Collect the raw audio if desired
      for (auto& job : jobs)
      {
        job->CollectRawAudio(audioChunk, kRawAudioChunkSize);
      }
      
      // Factory test doesn't need to do any mic processing, it just uses raw data
      if(!FACTORY_TEST)
      {
        // Process the audio into a single channel, and collect it if desired
        (void) ProcessRawAudio(
          nextData.timestamp,
          audioChunk,
          nextData.robotStatusFlags,
          nextData.robotRotationAngle);
      }
      
      {
        std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
        // Check if each of the jobs are done, removing the ones that are
        auto jobIter = _micProcessingJobs.begin();
        while (jobIter != _micProcessingJobs.end())
        {
          (*jobIter)->UpdateForNextChunk();
          bool jobDone = (*jobIter)->CheckDone();
          if (jobDone)
          {
            jobIter = _micProcessingJobs.erase(jobIter);
          }
          else
          {
            ++jobIter;
          }
        }
      }
      
      rawAudioToProcess.pop_front();
    }

    const auto end = std::chrono::steady_clock::now();
    const auto elapsedTime = (end - start);
    if (elapsedTime < maxProcTime)
    {
      std::this_thread::sleep_for(maxProcTime - elapsedTime);
    }
  }
}

void MicDataProcessor::ProcessTriggerLoop()
{
  Anki::Util::SetThreadName(pthread_self(), "MicProcTrigger");
  while (!_processThreadStop)
  {
#if ANKI_CPU_PROFILER_ENABLED
    static constexpr uint32_t maxTriggerProcTime_ms = 10;
#endif
    ANKI_CPU_TICK("MicDataProcessorTrigger", maxTriggerProcTime_ms, Util::CpuThreadProfiler::kLogFrequencyNever);
    ANKI_CPU_PROFILE("ProcessTriggerLoop");
    TimedMicData* readyDataSpot = nullptr;
    {
      ANKI_CPU_PROFILE("WaitForData");
      std::unique_lock<std::mutex> lock(_procAudioXferMutex);
      auto dataReadyCheck = [this] () { return _processThreadStop || _procAudioXferCount > 0; };
      _dataReadyCondition.wait(lock, dataReadyCheck);

      // Special case if we're being signalled to shut down
      if (_processThreadStop)
      {
        return;
      }

      // Grab a handle to the next available data that's been processed out of raw but not yet
      // "transferred" to the trigger word recognition processing
      readyDataSpot = &_immediateAudioBuffer[_procAudioRawComplete - _procAudioXferCount];
    }

    const auto& processedAudio = readyDataSpot->audioBlock;
    std::deque<std::shared_ptr<MicDataInfo>> jobs;
    {
      std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
      jobs = _micProcessingJobs;
    }
    for (auto& job : jobs)
    {
      job->CollectProcessedAudio(processedAudio.data(), processedAudio.size());
    }

    kMicData_NextTriggerIndex = Util::Clamp(kMicData_NextTriggerIndex, 0, kTriggerDataListLen-1);
    if (kMicData_NextTriggerIndex != _currentTriggerSearchIndex)
    {
      _currentTriggerSearchIndex = kMicData_NextTriggerIndex;
      _recognizer->SetRecognizerIndex(_currentTriggerSearchIndex);
    }

    // Keep a counter from the last active vad flag. When it hits 0 don't bother doing
    // the trigger recognition, then reset the counter when the flag is active again
    constexpr uint32_t kVadCountdown_ms = 4 * 1000;
    constexpr uint32_t kVadCountdownLimit = kVadCountdown_ms / kTimePerSEBlock_ms;
    if (_micImmediateDirection->GetLatestSample().activeState != 0)
    {
      _vadCountdown = kVadCountdownLimit;
    }
    else if (_vadCountdown > 0)
    {
      --_vadCountdown;
    }

    // Run the trigger detection, which will use the callback defined above
    if (_vadCountdown != 0)
    {
      ANKI_CPU_PROFILE("RecognizeTriggerWord");
      bool streamingInProgress = false;
      {
        std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
        streamingInProgress = (_currentStreamingJob != nullptr);
      }
      if (!streamingInProgress)
      {
        _recognizer->Update(processedAudio.data(), (unsigned int)processedAudio.size());
      }
    }

    // Now we're done using this audio with the recognizer, so let it go
    {
      std::lock_guard<std::mutex> lock(_procAudioXferMutex);
      --_procAudioXferCount;
    }
    _xferAvailableCondition.notify_all();
  }
}
  
void MicDataProcessor::ProcessMicDataPayload(const RobotInterface::MicData& payload)
{
  // Store off this next job
  std::lock_guard<std::mutex> lock(_rawMicDataMutex);
  // Use whichever buffer is currently _not_ being processed
  auto& bufferToUse = (_rawAudioProcessingIndex == 1) ? _rawAudioBuffers[0] : _rawAudioBuffers[1];
  RobotInterface::MicData& nextJob = bufferToUse.push_back();
  nextJob = payload;
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

void MicDataProcessor::Update(BaseStationTime_t currTime_nanosec)
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
    RobotInterface::SendAnimToEngine(std::move(msg));


    _fftResultMutex.lock();
  }
  _fftResultMutex.unlock();

  bool receivedStopMessage = false;
  static constexpr int kMaxReceiveBytes = 2000;
  char receiveArray[kMaxReceiveBytes];
  
  const ssize_t bytesReceived = _udpServer->Recv(receiveArray, kMaxReceiveBytes);
  if (bytesReceived == 2)
  {
    if (receiveArray[0] == 0 && receiveArray[1] == 0)
    {
      PRINT_NAMED_INFO("MicDataProcessor.Update.RecvCloudProcess.StopSignal", "");
      receivedStopMessage = true;
    }
#if ANKI_DEV_CHEATS
    else if (receiveArray[0] == 0 && receiveArray[1] == 1)
    {
      PRINT_NAMED_INFO("MicDataProcessor.Update.RecvCloudProcess.FakeTrigger", "");
      _fakeStreamingState = true;

      // Set up a message to send out about the triggerword
      RobotInterface::TriggerWordDetected twDetectedMessage;
      const auto mostRecentTimestamp = _immediateAudioBuffer.back().timestamp;
      twDetectedMessage.timestamp = mostRecentTimestamp;
      twDetectedMessage.direction = _micImmediateDirection->GetDominantDirection();
      auto engineMessage = std::unique_ptr<RobotInterface::RobotToEngine>(
        new RobotInterface::RobotToEngine(std::move(twDetectedMessage)));
      {
        std::lock_guard<std::mutex> lock(_msgsMutex);
        _msgsToEngine.push_back(std::move(engineMessage));
      }
    } 
#endif
    else
    {
      PRINT_NAMED_INFO("MicDataProcessor.Update.RecvCloudProcess.UnexpectedSignal", "0x%x 0x%x", receiveArray[0], receiveArray[1]);
      receivedStopMessage = true;
    }
  }

#if ANKI_DEV_CHEATS
  uint32_t recordingSecondsRemaining = 0;
  static std::shared_ptr<MicDataInfo> _saveJob;
  if (_saveJob != nullptr)
  {
    if (_saveJob->CheckDone())
    {
      _saveJob = nullptr;
      _forceRecordClip = false;
    }
    else
    {
      recordingSecondsRemaining = (_saveJob->GetTimeToRecord_ms() - _saveJob->GetTimeRecorded_ms()) / 1000;
    }
  }

  const bool isMicFace = FaceDisplay::GetDebugDraw()->GetDrawState() == FaceDebugDraw::DrawState::MicDirectionClock;
  if (!isMicFace)
  {
    _forceRecordClip = false;
  }
  else if (_forceRecordClip && nullptr == _saveJob)
  {
    MicDataInfo* newJob = new MicDataInfo{};
    newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "debugCapture"});
    newJob->_writeNameBase = ""; // Use the autogen names in this subfolder
    newJob->_numMaxFiles = 30;
    newJob->_typesToRecord.SetBitFlag(MicDataType::Raw, true);
    newJob->_typesToRecord.SetBitFlag(MicDataType::Processed, true);
    newJob->SetTimeToRecord(15000);

    {
      std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
      _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
      _saveJob = _micProcessingJobs.back();
    }
  }

  // Minimum length of time to display the "trigger heard" symbol on the mic data debug screen
  // (is extended by streaming)
  constexpr uint32_t kTriggerDisplayTime_ns = 1000 * 1000 * 2000; // 2 seconds
  static BaseStationTime_t endTriggerDispTime_ns = 0;
  if (endTriggerDispTime_ns > 0 && endTriggerDispTime_ns < currTime_nanosec)
  {
    endTriggerDispTime_ns = 0;
  }
#endif

  // lock the job mutex
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    // check if the pointer to the currently streaming job is valid
    if (!_currentlyStreaming 
      && (_currentStreamingJob != nullptr
      #if ANKI_DEV_CHEATS
        || _fakeStreamingState
      #endif
    ))
    {
      #if ANKI_DEV_CHEATS
        endTriggerDispTime_ns = currTime_nanosec + kTriggerDisplayTime_ns;
      #endif
      if (_udpServer->HasClient())
      {
        _currentlyStreaming = true;
        _streamingAudioIndex = 0;
  
        // Send out the message announcing the trigger word has been detected
        static const char* const hotwordSignal = "hotword";
        static const size_t hotwordSignalLen = std::strlen(hotwordSignal) + 1;
        _udpServer->Send(hotwordSignal, Util::numeric_cast<int>(hotwordSignalLen));
        PRINT_NAMED_INFO("MicDataProcessor.Update.StreamingStart", "");
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
      static constexpr size_t kMaxRecordNumChunks = (kStreamingTimeout_ms / kTimePerSEBlock_ms) + 1;
      if (receivedStopMessage || _streamingAudioIndex >= kMaxRecordNumChunks)
      {
        ClearCurrentStreamingJob();
        PRINT_NAMED_INFO("MicDataProcessor.Update.StreamingEnd", "%zu ms", _streamingAudioIndex * kTimePerSEBlock_ms);
      }
      else
      {
      #if ANKI_DEV_CHEATS
        if (!_fakeStreamingState)
      #endif
        {
          // Copy any new data that has been pushed onto the currently streaming job
          AudioUtil::AudioChunkList newAudio = _currentStreamingJob->GetProcessedAudio(_streamingAudioIndex);
          _streamingAudioIndex += newAudio.size();
      
          // Send the audio to any clients we've got
          if (_udpServer->HasClient())
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

  // Send out any messages we have to the engine
  std::vector<std::unique_ptr<RobotInterface::RobotToEngine>> stolenMessages;
  {
    std::lock_guard<std::mutex> lock(_msgsMutex);
    stolenMessages = std::move(_msgsToEngine);
    _msgsToEngine.clear();
  }

  #if ANKI_DEV_CHEATS
    // Store off a copy of (one of) the micDirectionData from this update for debug drawing
    Anki::Cozmo::RobotInterface::MicDirection micDirectionData{};
    bool updatedMicDirection = false;
  #endif
  for (const auto& msg : stolenMessages)
  {
    if (msg->tag == RobotInterface::RobotToEngine::Tag_triggerWordDetected)
    {
      RobotInterface::SendAnimToEngine(msg->triggerWordDetected);
    }
    else if (msg->tag == RobotInterface::RobotToEngine::Tag_micDirection)
    {
      #if ANKI_DEV_CHEATS
        micDirectionData = msg->micDirection;
        updatedMicDirection = true;
      #endif
      RobotInterface::SendAnimToEngine(msg->micDirection);
    }
    else
    {
      DEV_ASSERT_MSG(false, 
                     "MicDataProcessor.Update.UnhandledOutgoingMessageType",
                     "%s", RobotInterface::RobotToEngine::TagToString(msg->tag));
    }
  }

  #if ANKI_DEV_CHEATS
    if (updatedMicDirection || recordingSecondsRemaining != 0)
    {
      FaceDisplay::GetDebugDraw()->DrawConfidenceClock(
        micDirectionData,
        GetIncomingMicDataPercentUsed(),
        recordingSecondsRemaining,
        endTriggerDispTime_ns != 0 || _currentlyStreaming);
    }
  #endif
}

void MicDataProcessor::ClearCurrentStreamingJob()
{
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    _currentlyStreaming = false;
    if (_currentStreamingJob != nullptr)
    {
      _currentStreamingJob->SetTimeToRecord(0);
      _currentStreamingJob = nullptr;
    }
  }
  
  ResetMicListenDirection();
}

void MicDataProcessor::ResetMicListenDirection()
{
  std::lock_guard<std::mutex> lock(_seInteractMutex);
  PolicyDoAutoSearch();
}

float MicDataProcessor::GetIncomingMicDataPercentUsed()
{
  std::lock_guard<std::mutex> lock(_rawMicDataMutex);
  // Use whichever buffer is currently _not_ being processed
  const auto inUseIndex = (_rawAudioProcessingIndex == 1) ? 0 : 1;
  const auto& bufferInUse = _rawAudioBuffers[inUseIndex];
  const auto updatedFullness = ((float)bufferInUse.size()) / ((float)bufferInUse.capacity());
  // Cache the current fullness for this buffer and use the greater of the two buffer fullnesses
  // This way the "fullness" returned is less variable and better covers the worst case
  _rawAudioBufferFullness[inUseIndex] = updatedFullness;
  return MAX(_rawAudioBufferFullness[0], _rawAudioBufferFullness[1]);
}


} // namespace MicData
} // namespace Cozmo
} // namespace Anki
