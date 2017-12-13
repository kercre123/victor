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
#include "se_diag.h"
#include "speex/speex_resampler.h"

#include "anki/messaging/shared/UdpServer.h"
#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/micDataInfo.h"
#include "cozmoAnim/micImmediateDirection.h"
#include "cozmoAnim/speechRecognizerTHFSimple.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/threading/threadPriority.h"

#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"

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

  const unsigned short kCloudProcessCommunicationPort = 9880;
}

namespace Anki {
namespace Cozmo {
namespace MicData {

static_assert(std::is_same<decltype(RobotInterface::MicData::data), RawAudioChunk>::value,
              "Expecting type of MicData::data to match RawAudioChunk");


// TODO: VIC-752 Remove this code copied from robotProcess for reading configure data when victor mics aren't broken
// ------------------------ Begin Stolen HAL Config Loading functionality from hal_config.cpp/h --------------------
//forward Declarations
namespace HALConfig {
  typedef enum {
    INVALID,
    FLOAT,
    DOUBLE,
    //Add more types here
  } ValueType;
  
  typedef struct {
    const char* key;
    ValueType type;
    void* address;
  } Item;

  Result ReadConfigFile(const char* path, const HALConfig::Item config[]);
  void ParseValue(const char* valstr, const char* end, const HALConfig::Item* item);
  void ParseConfigLine(const char* line, int sz, const HALConfig::Item config[]);
}

namespace {
  static const char CONFIG_SEPARATOR = ':';
}
  
void HALConfig::ParseValue(const char* valstr, const char* end, const HALConfig::Item* item)
{
  char* endconv = NULL;
  assert(strlen(valstr) <= end-valstr);
  double val = strtod(valstr, &endconv);
  if (endconv > valstr) //valid conversion of at least some chars. 
    //Open Question: do we reject partially valid entries like "2.0pi", which would get scanned as 2.0?
  {
    switch (item->type)
    {
    case HALConfig::DOUBLE:
      *(double*)(item->address) = val;
      break;
    case HALConfig::FLOAT:
      *(float*)(item->address) = val;
      break;
    default:
      assert(!"Invalid Conversion Type");
      break;
    }
  }
}

void HALConfig::ParseConfigLine(const char* line, int sz, const HALConfig::Item config[])
{
  int i;
  const char* end = line+sz;
  if (sz <1 ) { return; }
  while (isspace(*line) ) {if (++line>= end) { return; } } //find first non-whitespace
  const char* sep = line;
  while (*sep != CONFIG_SEPARATOR) { if (++sep >= end) { return;} } //find end of key.
  for (i=0;config[i].address != NULL;i++) {
    if (strncmp(line, config[i].key, sep-line) == 0) { //match
      ParseValue(sep+1, end, &config[i]);
      break;
    }
  }
  return;
}

Result HALConfig::ReadConfigFile(const char* path, const HALConfig::Item config[] )
{
  FILE* fp = fopen(path, "r");
  if (!fp) {
    PRINT_NAMED_WARNING("HALConfig::ReadConfigFile","Can't open %s\n", path);
    return RESULT_FAIL_FILE_OPEN;
  }
  else {
    while (1) {
      char* line = NULL;
      size_t bufsz = 0;
      ssize_t nread = getline(&line, &bufsz, fp);
      if (nread > 0) {
        ParseConfigLine(line, (int) nread, config);
      }
      else {
        break; //EOF;
      }
      free(line);
    }
  }
  fclose(fp);
  return RESULT_OK;
}

static f32 HAL_SOME_MICS_BROKEN = 0.0f;
#ifdef SIMULATOR
static const char* HAL_INI_PATH = "hal.conf";
#else
static const char* HAL_INI_PATH = "/data/persist/hal.conf";
#endif
const HALConfig::Item  configitems_[]  = {
  {"Some Mics Broken",     HALConfig::FLOAT, &HAL_SOME_MICS_BROKEN},
  {0} //Need zeros as end-of-list marker
};
// ------------------------ End Stolen HAL Config Loading functionality from hal_config.cpp/h --------------------




MicDataProcessor::MicDataProcessor(const std::string& writeLocation, const std::string& triggerWordDataDir)
: _writeLocationDir(writeLocation)
, _recognizer(new SpeechRecognizerTHF())
, _udpServer(new UdpServer())
, _micImmediateDirection(new MicImmediateDirection())
{
  // Set up aubio tempo/beat detector
  const char* const kTempoMethod = "";
  const uint_t kTempoBufSize = 1024;
  const uint_t kTempoHopSize = 512;
  const uint_t kTempoSampleRate = 44100;
  _tempoDetector = new_aubio_tempo(kTempoMethod, kTempoBufSize, kTempoHopSize, kTempoSampleRate);
  DEV_ASSERT(_tempoDetector != nullptr, "MicDataProcessor.Constructor.FailedCreatingAubioTempoObject");
  
  if (!_writeLocationDir.empty())
  {
    Util::FileUtils::CreateDirectory(_writeLocationDir);
  }

  HALConfig::ReadConfigFile(HAL_INI_PATH, configitems_);

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

void MicDataProcessor::TriggerWordDetectCallback(const char* resultFound, float score)
{
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
  newJob->_typesToRecord.SetBitFlag(MicDataType::Resampled, _forceRecordClip);
  newJob->_typesToRecord.SetBitFlag(MicDataType::Raw, _forceRecordClip);
  newJob->SetTimeToRecord(MicDataInfo::kMaxRecordTime_ms);

  // Copy the current audio chunks in the trigger overlap buffer
  for (uint32_t i=0; i<_immediateAudioBuffer.size(); ++i)
  {
    const auto& audioBlock = _immediateAudioBuffer[i].audioBlock;
    newJob->CollectProcessedAudio(audioBlock.data(), audioBlock.size());
  }

  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
    _currentStreamingJob = _micProcessingJobs.back();
  }

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

  PRINT_NAMED_INFO("MicDataProcessor.TWCallback",
                    "Direction index %d at timestamp %d",
                    _micImmediateDirection->GetDominantDirection(),
                    mostRecentTimestamp);
}

MicDataProcessor::~MicDataProcessor()
{
  // delete Aubio tempo detection object
  if (_tempoDetector != nullptr) {
    del_aubio_tempo(_tempoDetector);
  }
  
  _processThreadStop = true;
  _processThread.join();

  _udpServer->StopListening();

  speex_resampler_destroy(_speexState);
  MMIfDestroy();
  _recognizer->Stop();
}

bool MicDataProcessor::ProcessResampledAudio(TimeStamp_t timestamp,
                                             const AudioUtil::AudioSample* audioChunk)
{
  {
    ANKI_CPU_PROFILE("UninterleaveAudioForSE");
    // Uninterleave the chunks when copying out of the payload, since that's what SE wants
    for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerChunkForSE; ++sampleIdx)
    {
      const uint32_t interleaveBase = (sampleIdx * kNumInputChannels);
      for (uint32_t channelIdx = 0; channelIdx < kNumInputChannels; ++channelIdx)
      {
        uint32_t dataOffset = _inProcessAudioBlockFirstHalf ? 0 : kSamplesPerChunkForSE;
        const uint32_t uninterleaveBase = (channelIdx * kSamplesPerBlock) + dataOffset;
        _inProcessAudioBlock[sampleIdx + uninterleaveBase] = audioChunk[channelIdx + interleaveBase];
      }
    }
  }
  
  // If we aren't starting a block, we're finishing it - time to convert to a single channel
  if (!_inProcessAudioBlockFirstHalf)
  {
    TimedMicData& nextSample = _immediateAudioBuffer.push_back();
    nextSample.timestamp = timestamp;
    const bool kAverageValues = (HAL_SOME_MICS_BROKEN != 0.0f);
    DirConfResult directionConfidence{};
    // TODO: VIC-752 Remove this code for averaging mic data when victor mics aren't broken
    if (kAverageValues)
    {
      std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> averagedAudioChunk;
      {
        ANKI_CPU_PROFILE("AverageMicDataAcrossChannels");
        // Average the 4 channels and store into our temp array
        for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerBlock; ++sampleIdx)
        {
          int32_t sumValue = _inProcessAudioBlock[sampleIdx + (0 * kSamplesPerBlock)];
          sumValue +=        _inProcessAudioBlock[sampleIdx + (1 * kSamplesPerBlock)];
          sumValue +=        _inProcessAudioBlock[sampleIdx + (2 * kSamplesPerBlock)];
          sumValue +=        _inProcessAudioBlock[sampleIdx + (3 * kSamplesPerBlock)];
          
          const auto averagedValue = static_cast<AudioUtil::AudioSample>(sumValue / 4);
          averagedAudioChunk[sampleIdx + (kSamplesPerBlock * 0)] = averagedValue;
          averagedAudioChunk[sampleIdx + (kSamplesPerBlock * 1)] = averagedValue;
          averagedAudioChunk[sampleIdx + (kSamplesPerBlock * 2)] = averagedValue;
          averagedAudioChunk[sampleIdx + (kSamplesPerBlock * 3)] = averagedValue;
        }
      }
      directionConfidence = ProcessMicrophonesSE(averagedAudioChunk.data(), nextSample.audioBlock.data());
      // When we've averaged the values force set the first direction
      directionConfidence.direction = MicImmediateDirection::kFirstIndex;
    }
    else
    {
      directionConfidence = ProcessMicrophonesSE(_inProcessAudioBlock.data(), nextSample.audioBlock.data());
    }

    // Store off this most recent result in our immedate direction tracking
    _micImmediateDirection->AddDirectionSample(directionConfidence.direction, directionConfidence.confidence);

    // Set up a message to send out about the direction
    RobotInterface::MicDirection newMessage;
    newMessage.timestamp = timestamp;
    newMessage.direction = directionConfidence.direction;
    newMessage.confidence = directionConfidence.confidence;
    auto engineMessage = std::unique_ptr<RobotInterface::RobotToEngine>(
      new RobotInterface::RobotToEngine(std::move(newMessage)));
    {
      std::lock_guard<std::mutex> lock(_msgsMutex);
      _msgsToEngine.push_back(std::move(engineMessage));
    }
  }
  
  _inProcessAudioBlockFirstHalf = !_inProcessAudioBlockFirstHalf;
  
  return _inProcessAudioBlockFirstHalf;
}

MicDataProcessor::DirConfResult MicDataProcessor::ProcessMicrophonesSE(const AudioUtil::AudioSample* audioChunk,
                                                                       AudioUtil::AudioSample* bufferOut) const
{
  static const std::array<AudioUtil::AudioSample, kSamplesPerBlock * kNumInputChannels> dummySpeakerOut{};
  {
    ANKI_CPU_PROFILE("ProcessMicrophonesSE");
    // Process the current audio block with SE software
    MMIfProcessMicrophones(dummySpeakerOut.data(), audioChunk, bufferOut);
  }

  const char * const kBestSearchBeamDiagName = "fdsearch_best_beam_index";
  const auto bestDirIdx = SEDiagGetIndex(kBestSearchBeamDiagName);
  const auto latestDirection = SEDiagGetUInt16(bestDirIdx);

  const char * const kBestSearchBeamConfDiagName = "fdsearch_best_beam_confidence";
  const auto bestDirConfIdx = SEDiagGetIndex(kBestSearchBeamConfDiagName);
  const auto latestConf = SEDiagGetInt16(bestDirConfIdx);

  DirConfResult result = 
  {
    .direction = latestDirection,
    .confidence = latestConf
  };
  return result;
}

void MicDataProcessor::ResampleAudioChunk(const AudioUtil::AudioSample* audioChunk,
                                          AudioUtil::AudioSample* bufferOut)
{
  ANKI_CPU_PROFILE("ResampleAudioChunk");
  uint32_t numSamplesProcessed = kSamplesPerChunkIncoming;
  uint32_t numSamplesWritten = kSamplesPerChunkForSE;
  speex_resampler_process_interleaved_int(_speexState, 
                                          audioChunk, &numSamplesProcessed, 
                                          bufferOut, &numSamplesWritten);
  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesProcessed",
              "Expected %d processed only processed %d", kSamplesPerChunkIncoming, numSamplesProcessed);

  ANKI_VERIFY(numSamplesProcessed == kSamplesPerChunkIncoming,
              "MicDataProcessor.ResampleAudioChunk.SamplesWritten",
              "Expected %d written only wrote %d", kSamplesPerChunkForSE, numSamplesWritten);
}

void MicDataProcessor::ProcessLoop()
{
  Anki::Util::SetThreadName(pthread_self(), "MicDataProc");
  static constexpr uint32_t expectedAudioDropsPerAnimLoop = 7;
  static constexpr uint32_t maxProcessingTimePerDrop_ms = 5;
  static constexpr uint32_t maxProcTime_ms = expectedAudioDropsPerAnimLoop * maxProcessingTimePerDrop_ms;
  const auto maxProcTime = std::chrono::milliseconds(maxProcTime_ms);
  while (!_processThreadStop)
  {
    const auto start = std::chrono::steady_clock::now();
  
    // Switch which buffer we're processing if it's empty
    {
      std::lock_guard<std::mutex> lock(_resampleMutex);
      if (_rawAudioBuffers[_rawAudioProcessingIndex].empty())
      {
        _rawAudioProcessingIndex = (_rawAudioProcessingIndex == 1) ? 0 : 1;
      }
    }

    auto& rawAudioToProcess = _rawAudioBuffers[_rawAudioProcessingIndex];
    while (rawAudioToProcess.size() > 0)
    {
      ANKI_CPU_TICK("MicDataProcessor::ProcessLoop", maxProcessingTimePerDrop_ms, Util::CpuThreadProfiler::kLogFrequencyNever);

      const auto& nextData = rawAudioToProcess.front();
      const auto& audioChunk = nextData.audioChunk;
      
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
        job->CollectRawAudio(audioChunk.data(), audioChunk.size());
      }
      
      // Resample the audio, then collect it if desired
      std::array<AudioUtil::AudioSample, kResampledAudioChunkSize> resampledAudioChunk;
      ResampleAudioChunk(audioChunk.data(), resampledAudioChunk.data());
      for (auto& job : stolenJobs)
      {
        job->CollectResampledAudio(resampledAudioChunk.data(), resampledAudioChunk.size());
      }
      
      // Process the audio into a single channel, and collect it if desired
      bool audioBlockReady = ProcessResampledAudio(nextData.timestamp, resampledAudioChunk.data());
      if (audioBlockReady)
      {
        const auto& processedAudio = _immediateAudioBuffer.back().audioBlock;
        for (auto& job : stolenJobs)
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
  
void MicDataProcessor::ProcessMicDataPayload(const RobotInterface::MicData& payload)
{
  static uint32_t sLatestSequenceID = 0;
  // Since mic data is sent unreliably, make sure the sequence id increases appropriately
  if (payload.sequenceID < sLatestSequenceID &&
      (sLatestSequenceID - payload.sequenceID) <= (UINT32_MAX / 2)) // To handle rollover case
  {
    return;
  }
  sLatestSequenceID = payload.sequenceID;
  
  const RawAudioChunk& audioChunk = payload.data;
  // Store off this next job
  {
    std::lock_guard<std::mutex> lock(_resampleMutex);
    // Use whichever buffer is currently _not_ being processed
    auto& bufferToUse = (_rawAudioProcessingIndex == 1) ? _rawAudioBuffers[0] : _rawAudioBuffers[1];
    TimedRawMicData& nextJob = bufferToUse.push_back();
    nextJob.timestamp = payload.timestamp;
    const auto size = sizeof(audioChunk) / sizeof(audioChunk[0]);
    std::copy(audioChunk, audioChunk + size, nextJob.audioChunk.data());
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
  
  const ssize_t bytesReceived = _udpServer->Recv(receiveArray, kMaxReceiveBytes);
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
      if (_forceRecordClip || _udpServer->GetNumClients() > 0)
      {
        _currentlyStreaming = true;
        streamingAudioIndex = 0;
  
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

  // Send out any messages we have to the engine
  std::vector<std::unique_ptr<RobotInterface::RobotToEngine>> stolenMessages;
  {
    std::lock_guard<std::mutex> lock(_msgsMutex);
    stolenMessages = std::move(_msgsToEngine);
    _msgsToEngine.clear();
  }
  for (const auto& msg : stolenMessages)
  {
    if (msg->tag == RobotInterface::RobotToEngine::Tag_triggerWordDetected)
    {
      RobotInterface::SendMessageToEngine(msg->triggerWordDetected);
    }
    else if (msg->tag == RobotInterface::RobotToEngine::Tag_micDirection)
    {
      RobotInterface::SendMessageToEngine(msg->micDirection);
    }
    else
    {
      DEV_ASSERT_MSG(false, 
                     "MicDataProcessor.Update.UnhandledOutgoingMessageType",
                     "%s", RobotInterface::RobotToEngine::TagToString(msg->tag));
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
