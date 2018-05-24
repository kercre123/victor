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

#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/beatDetector/beatDetector.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/micData/micDataProcessor.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/micData/micDataInfo.h"
#include "cozmoAnim/micData/micImmediateDirection.h"
#include "cozmoAnim/speechRecognizerTHFSimple.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/threading/threadPriority.h"

#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"


namespace {
  struct TriggerData
  {
    std::string dataDir;
    std::string netFile;
    std::string searchFile;
  };

  const TriggerData kTriggerDataList[] = 
  {
    // "HeyVector" trigger trained on adults search 5
    {
      .dataDir = "hey_vector/trigger_anki_x_enUS_01s_hey_vector_sfs14_a326a14b",
      .netFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_am.raw",
      .searchFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_search_5.raw"
    },
    // "HeyCozmo" trigger trained on adults
    {
      .dataDir = "hey_cosmo_and_commands/trigger_anki_x_en_us_01_hey_cosmo_sfs14_b3e3cbba",
      .netFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_am.raw",
      .searchFile = "anki_x_hey_cosmo_en_us_sfs14_b3e3cbba_delivery01_search_4.raw"
    },
    // "HeyVector" trigger trained on adults search 7
    {
      .dataDir = "hey_vector/trigger_anki_x_enUS_01s_hey_vector_sfs14_a326a14b",
      .netFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_am.raw",
      .searchFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_search_7.raw"
    },
    // "HeyVector" trigger trained on adults search 10
    {
      .dataDir = "hey_vector/trigger_anki_x_enUS_01s_hey_vector_sfs14_a326a14b",
      .netFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_am.raw",
      .searchFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_search_10.raw"
    },
    // "HeyVector" trigger trained on adults search 14
    {
      .dataDir = "hey_vector/trigger_anki_x_enUS_01s_hey_vector_sfs14_a326a14b",
      .netFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_am.raw",
      .searchFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_search_14.raw"
    },
    // "HeyVector" trigger trained on adults search 18
    {
      .dataDir = "hey_vector/trigger_anki_x_enUS_01s_hey_vector_sfs14_a326a14b",
      .netFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_am.raw",
      .searchFile = "anki_x_hey_vector_enUS_sfs14_a326a14b_delivery01s_search_18.raw"
    },
    // "HeyVector" frFR trigger trained on adults search 11
    {
      .dataDir = "hey_vector/trigger_ai_x_fr_01s_heyvector_sfs14_a51d9f15",
      .netFile = "ai_x_heyvector_fr_sfs14_a51d9f15_delivery01s_am.raw",
      .searchFile = "ai_x_heyvector_fr_sfs14_a51d9f15_delivery01s_search_11.raw"
    }
  };
  constexpr int32_t kTriggerDataListLen = (int32_t) sizeof(kTriggerDataList) / sizeof(kTriggerDataList[0]);
  Anki::AudioUtil::SpeechRecognizer::IndexType _currentTriggerSearchIndex = 5;

# define CONSOLE_GROUP "MicData"
  CONSOLE_VAR_RANGED(s32, kMicData_NextTriggerIndex, CONSOLE_GROUP, _currentTriggerSearchIndex, 0, kTriggerDataListLen-1);
  CONSOLE_VAR(bool, kMicData_SaveRawFullIntent, CONSOLE_GROUP, false);
  CONSOLE_VAR(bool, kMicData_UseFallbackBeam, CONSOLE_GROUP, false);
  CONSOLE_VAR(bool, kMicData_ForceDisableMicDataProc, CONSOLE_GROUP, false);
  CONSOLE_VAR(bool, kMicData_ForceEnableMicDataProc, CONSOLE_GROUP, false);
# undef CONSOLE_GROUP

}

namespace Anki {
namespace Cozmo {
namespace MicData {

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_RANGED(float, maxProcessingTimePerDrop_ms,      ANKI_CPU_CONSOLEVARGROUP, 5, 5, 32);
  CONSOLE_VAR_RANGED(float, maxTriggerProcTime_ms,            ANKI_CPU_CONSOLEVARGROUP, 10, 10, 32);
  CONSOLE_VAR_ENUM(u8,      kMicDataProcessorRaw_Logging,     ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
  CONSOLE_VAR_ENUM(u8,      kMicDataProcessorTrigger_Logging, ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

constexpr auto kCladMicDataTypeSize = sizeof(RobotInterface::MicData::data)/sizeof(RobotInterface::MicData::data[0]);
static_assert(kCladMicDataTypeSize == kRawAudioChunkSize, "Expecting size of MicData::data to match RawAudioChunk");

MicDataProcessor::MicDataProcessor(MicDataSystem* micDataSystem, const std::string& writeLocation, const std::string& triggerWordDataDir)
: _micDataSystem(micDataSystem)
, _writeLocationDir(writeLocation)
, _recognizer(new SpeechRecognizerTHF())
, _micImmediateDirection(new MicImmediateDirection())
, _beatDetector(std::make_unique<BeatDetector>())
{
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
  _policyFallbackFlag = SEDiagGetIndex("policy_fallback_flag");

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
  // Ignore extra triggers during streaming
  if (_micDataSystem->HasStreamingJob())
  {
    return;
  }

  MicDataInfo* newJob = new MicDataInfo{};
  newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "triggeredCapture"});
  newJob->_writeNameBase = ""; // Use the autogen names in this subfolder
  newJob->_numMaxFiles = 100;
  bool saveToFile = false;
#if ANKI_DEV_CHEATS
  saveToFile = true;
  if (kMicData_SaveRawFullIntent)
  {
    newJob->EnableDataCollect(MicDataType::Raw, true);
  }
  newJob->_audioSaveCallback = std::bind(&MicDataSystem::AudioSaveCallback, _micDataSystem, std::placeholders::_1);
#endif
  newJob->EnableDataCollect(MicDataType::Processed, saveToFile);
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
    // Copy the current audio chunks in the trigger overlap buffer
    for (size_t i=0; i<_immediateAudioBufferRaw.size(); ++i)
    {
      const auto& audioBlock = _immediateAudioBufferRaw[i];
      newJob->CollectRawAudio(audioBlock.data(), audioBlock.size());
    }
    mostRecentTimestamp = _immediateAudioBuffer[_procAudioRawComplete-1].timestamp;
  }

  const auto isStreamingJob = true;
  _micDataSystem->AddMicDataJob(std::shared_ptr<MicDataInfo>(newJob), isStreamingJob);

  const auto currentDirection = _micImmediateDirection->GetDominantDirection();

  // Set up a message to send out about the triggerword
  RobotInterface::TriggerWordDetected twDetectedMessage;
  twDetectedMessage.timestamp = mostRecentTimestamp;
  twDetectedMessage.direction = currentDirection;
  auto engineMessage = std::make_unique<RobotInterface::RobotToEngine>(std::move(twDetectedMessage));
  _micDataSystem->SendMessageToEngine(std::move(engineMessage));

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

  MMIfDestroy();
  _recognizer->Stop();
}

void MicDataProcessor::ProcessRawAudio(TimeStamp_t timestamp,
                                       const AudioUtil::AudioSample* audioChunk,
                                       uint32_t robotStatus,
                                       float robotAngle)
{
  ANKI_CPU_PROFILE("MicDataProcessor::ProcessRawAudio");
  
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
    {
      ANKI_CPU_PROFILE("BeatDetectorAddSamples");
      // For beat detection, we only need a single channel of audio. Use the
      // first quarter of the un-interleaved audio block
      const bool beatDetected = _beatDetector->AddSamples(_inProcessAudioBlock.data(), kSamplesPerBlock);
      if (beatDetected) {
        auto beatMessage = RobotInterface::BeatDetectorState{_beatDetector->GetLatestBeat()};
        auto engineMessage = std::make_unique<RobotInterface::RobotToEngine>(std::move(beatMessage));
        _micDataSystem->SendMessageToEngine(std::move(engineMessage));
      }
    }
    
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
    newMessage.latestPowerValue = directionResult.latestPowerValue;
    newMessage.latestNoiseFloor = directionResult.latestNoiseFloor;
    std::copy(
      directionResult.confidenceList.begin(),
      directionResult.confidenceList.end(),
      newMessage.confidenceList);
    
    auto engineMessage = std::make_unique<RobotInterface::RobotToEngine>(std::move(newMessage));
    _micDataSystem->SendMessageToEngine(std::move(engineMessage));
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
  const bool robotStartedMoving = robotIsMoving && !_robotWasMoving;
  const bool robotStoppedMoving = !robotIsMoving && _robotWasMoving;
  _robotWasMoving = robotIsMoving;

  // When the robot is moving, we use the "fallback policy", meaning we essentially disable beamforming so that we aren't
  // focusing on the gear noise (note we still run SE processing to combine the channels when using the fallback policy).
  if (robotStartedMoving || robotStoppedMoving)
  {
    int32_t newSetting = robotStartedMoving ? 1 : 0;
    SEDiagSetInt32(_policyFallbackFlag, newSetting);
    PRINT_NAMED_INFO("MicDataProcessor.ProcessMicrophonesSE.SetUseFallbackBeam", "%s", robotStartedMoving ? "true" : "false");
  }

  if (robotStoppedMoving)
  {
    // When the robot has stopped moving (and the gears are no longer making noise) we reset the mic direciton
    // confidence values to be based on non-noisy data
    MMIfResetLocationSearch();
  }

  // We only care about checking one channel, and since the channel data is uninterleaved when passed in here,
  // we simply give the start of the buffer as the input to run the vad detection on
  float latestPowerValue = 0.f;
  float latestNoiseFloor = 0.f;
  int activityFlag = 0;
  {
    ANKI_CPU_PROFILE("ProcessVAD");

    // Note while we _can_ pass a confidence value here adjusted while the robot is moving, we'd rather err on the side
    // of always thinking we hear a voice when the robot moves, so we maximize our chances of hearing any triggers
    // over the noise. So when the robot is moving, don't even bother running the VAD, and instead just set activity
    // to true.
    const float vadConfidence = 1.0f;
    activityFlag = DoSVad(_sVadObject.get(),           // object
                          vadConfidence,               // confidence it is okay to measure noise floor, i.e. no known activity like gear noise
                          (int16_t*)audioChunk);       // pointer to input data
    latestPowerValue = _sVadObject->AvePowerInBlock;
    latestNoiseFloor = _sVadObject->NoiseFloor;
    if (robotIsMoving)
    {
      activityFlag = 1;
    }

    // Keep a counter from the last active vad flag. When it hits 0 don't bother doing
    // the trigger recognition, then reset the counter when the flag is active again
    constexpr uint32_t kVadCountdown_ms = 4 * 1000;
    constexpr uint32_t kVadCountdownLimit = kVadCountdown_ms / kTimePerSEBlock_ms;
    if (activityFlag != 0)
    {
      _vadCountdown = kVadCountdownLimit;
    }
    else if (_vadCountdown > 0)
    {
      --_vadCountdown;
    }

    if (_vadCountdown != 0)
    {
      activityFlag = 1;
    }
  }

  // Allow overriding (for testing) to force enable or disable mic data processing
  if (kMicData_ForceEnableMicDataProc)
  {
    activityFlag = 1;
  }
  else if (kMicData_ForceDisableMicDataProc)
  {
    activityFlag = 0;
  }

  // If we've detected activity, SE signal processing (which includes the fallback policy
  // that will skip spatial filtering, which is enabled while the robot is moving).
  if (activityFlag == 1)
  {
    static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
    {
      ANKI_CPU_PROFILE("ProcessMicrophonesSE");
      // Process the current audio block with SE software
      MMIfProcessMicrophones(dummySpeakerOut.data(), audioChunk, bufferOut);
    }
  }
  else
  {
    // Otherwise if it's determined there's really no activity right now, we're skipping the standard SE processing
    // We simply remove the DC bias and apply some gain to the first mic channel and pass it along
    
    // Our bias is stored as a fixed-point value
    constexpr int iirCoefPower = 10;
    constexpr int iirMult = 1023; // (2 ^ iirCoefPower) - 1
    static int bias = audioChunk[0] << iirCoefPower;
    for (int i=0; i<kSamplesPerBlock; ++i)
    {
      // First update our bias with the latest audio sample
      bias = ((bias * iirMult) >> iirCoefPower) + audioChunk[i];
      // Push out the next sample using the updated bias
      bufferOut[i] = audioChunk[i] - (bias >> iirCoefPower);
      // Gain multiplier of 8 gives good results
      bufferOut[i] <<= 3;
    }
  }

  MicDirectionData result{};
  result.activeState = activityFlag;
  result.latestPowerValue = latestPowerValue;
  result.latestNoiseFloor = latestNoiseFloor;

  // When we know the robot is moving or there's no current activity (so we didn't do beamforming) clear direction data
  if (robotIsMoving || (activityFlag != 1))
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
  static const uint32_t maxProcTime_ms = expectedAudioDropsPerAnimLoop * maxProcessingTimePerDrop_ms;
  const auto maxProcTime = std::chrono::milliseconds(maxProcTime_ms);
  while (!_processThreadStop)
  {
    ANKI_CPU_TICK("MicDataProcessorRaw", maxProcTime_ms, Util::CpuProfiler::CpuProfilerLoggingTime(kMicDataProcessorRaw_Logging));
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
      ANKI_CPU_PROFILE("ProcessLoop");

      const auto& nextData = rawAudioToProcess.front();
      const auto* audioChunk = nextData.data;
      std::copy(audioChunk, audioChunk + kCladMicDataTypeSize, _immediateAudioBufferRaw.push_back().data());
      
      // Copy the current set of jobs we have for recording audio, so the list can be added to while processing
      // continues
      std::deque<std::shared_ptr<MicDataInfo>> jobs = _micDataSystem->GetMicDataJobs();
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
      
      _micDataSystem->UpdateMicJobs();
      
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
    ANKI_CPU_TICK("MicDataProcessorTrigger", maxTriggerProcTime_ms, Util::CpuProfiler::CpuProfilerLoggingTime(kMicDataProcessorTrigger_Logging));
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
    std::deque<std::shared_ptr<MicDataInfo>> jobs = _micDataSystem->GetMicDataJobs();
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

    // Run the trigger detection, which will use the callback defined above
    // Note we skip it if there is no activity as of the latest processed audioblock
    if (_micImmediateDirection->GetLatestSample().activeState != 0)
    {
      ANKI_CPU_PROFILE("RecognizeTriggerWord");
      _recognizer->Update(processedAudio.data(), (unsigned int)processedAudio.size());
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
