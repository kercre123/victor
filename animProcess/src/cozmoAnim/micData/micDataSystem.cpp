/**
* File: micDataSystem.cpp
*
* Author: Lee Crippen
* Created: 03/26/2018
*
* Description: Handles Updates to mic data processing, streaming collection jobs, and generally acts
*              as messaging/access hub.
*
* Copyright: Anki, Inc. 2018
*
*/

#include "coretech/messaging/shared/LocalUdpServer.h"

#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/beatDetector/beatDetector.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/micData/micDataInfo.h"
#include "cozmoAnim/micData/micDataProcessor.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/showAudioStreamStateManager.h"

#include "audioEngine/plugins/ankiPluginInterface.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "osState/osState.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"

#include <iomanip>
#include <sstream>


namespace {

# define CONSOLE_GROUP "MicData"

#if ANKI_DEV_CHEATS
  std::string _debugMicDataWriteLocation = "";

  void ClearMicData(ConsoleFunctionContextRef context)
  {
    if (!_debugMicDataWriteLocation.empty())
    {
      Anki::Util::FileUtils::RemoveDirectory(_debugMicDataWriteLocation);
    }
  }
  CONSOLE_FUNC(ClearMicData, "zHiddenForSafety");

#endif

#if ANKI_DEV_CHEATS
  CONSOLE_VAR_RANGED(u32, kMicData_ClipRecordTime_ms, CONSOLE_GROUP, 4000, 500, 15000);
  CONSOLE_VAR(bool, kMicData_SaveRawFullIntent_Wakewordless, CONSOLE_GROUP, false);
#endif // ANKI_DEV_CHEATS

# undef CONSOLE_GROUP

}

namespace Anki {
namespace Vector {
namespace MicData {

constexpr auto kCladMicDataTypeSize = sizeof(RobotInterface::MicData::data)/sizeof(RobotInterface::MicData::data[0]);
static_assert(kCladMicDataTypeSize == kRawAudioChunkSize, "Expecting size of MicData::data to match RawAudioChunk");

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


MicDataSystem::MicDataSystem(Util::Data::DataPlatform* dataPlatform,
                             const AnimContext* context)
: _udpServer(new LocalUdpServer())
, _fftResultData(new FFTResultData())
, _context(context)
{
  const std::string& dataWriteLocation = dataPlatform->pathToResource(Util::Data::Scope::Cache, "micdata");
  const std::string& triggerDataDir = dataPlatform->pathToResource(Util::Data::Scope::Resources, "assets");
  _writeLocationDir = dataWriteLocation;
  _micDataProcessor.reset(new MicDataProcessor(_context, this, dataWriteLocation, triggerDataDir));

  if (!_writeLocationDir.empty())
  {
  #if ANKI_DEV_CHEATS
    Util::FileUtils::CreateDirectory(_writeLocationDir);
    _debugMicDataWriteLocation = _writeLocationDir;
  #endif
  }

  const RobotID_t robotID = OSState::getInstance()->GetRobotID();
  const std::string sockName = std::string{LOCAL_SOCKET_PATH} + "mic_sock" + (robotID == 0 ? "" : std::to_string(robotID));
  _udpServer->SetBindClients(false);
  const bool udpSuccess = _udpServer->StartListening(sockName);
  ANKI_VERIFY(udpSuccess,
              "MicDataSystem.Constructor.UdpStartListening",
              "Failed to start listening on socket %s",
              sockName.c_str());
}

void MicDataSystem::Init(const RobotDataLoader& dataLoader)
{
  _micDataProcessor->Init(dataLoader, _locale);
}

MicDataSystem::~MicDataSystem()
{
  // Tear down the mic data processor explicitly first, because it uses functionality owned by MicDataSystem
  _micDataProcessor.reset();

  _udpServer->StopListening();
}

void MicDataSystem::ProcessMicDataPayload(const RobotInterface::MicData& payload)
{
  _micDataProcessor->ProcessMicDataPayload(payload);
}

void MicDataSystem::RecordRawAudio(uint32_t duration_ms, const std::string& path, bool runFFT)
{
  RecordAudioInternal(duration_ms, path, MicDataType::Raw, runFFT);
}

void MicDataSystem::RecordProcessedAudio(uint32_t duration_ms, const std::string& path)
{
  RecordAudioInternal(duration_ms, path, MicDataType::Processed, false);
}

void MicDataSystem::StartWakeWordlessStreaming(CloudMic::StreamType type)
{
  if(HasStreamingJob())
  {
    PRINT_NAMED_WARNING("micDataProcessor.OverlappingStreamRequests",
                        "Received StartWakeWorldlessStreaming message from engine, but micDataSystem is already streaming");
    return;
  }

  ShowAudioStreamStateManager* showStreamState = _context->GetShowAudioStreamStateManager();
  if(!showStreamState->HasValidTriggerResponse())
  {
    PRINT_NAMED_WARNING("MicDataSystem.CantStreamToCloud",
                        "Wakewordless streaming request received, but incapable of opening the cloud stream, so ignoring request");
    return;
  }
  
  showStreamState->StartTriggerResponseWithoutGetIn();

  MicDataInfo* newJob = new MicDataInfo{};
  newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "triggeredCapture"});
  newJob->_writeNameBase = ""; //use autogen names
  newJob->_numMaxFiles = 100;
  newJob->_type = type;
  bool saveToFile = false;
#if ANKI_DEV_CHEATS
  saveToFile = true;
  if(kMicData_SaveRawFullIntent_Wakewordless){
    newJob->EnableDataCollect(MicDataType::Raw, true);
  }
  newJob->_audioSaveCallback = std::bind(&MicDataSystem::AudioSaveCallback, this, std::placeholders::_1);
#endif
  newJob->EnableDataCollect(MicDataType::Processed, saveToFile);
  newJob->SetTimeToRecord(MicDataInfo::kMaxRecordTime_ms);

  const bool isStreamingJob = true;
  AddMicDataJob(std::shared_ptr<MicDataInfo>(newJob), isStreamingJob);

  PRINT_NAMED_INFO("MicDataSystem.StartStreaming",
                   "Starting Wake Wordless streaming");
}

void MicDataSystem::FakeTriggerWordDetection()
{
  _micDataProcessor->FakeTriggerWordDetection();
}

void MicDataSystem::RecordAudioInternal(uint32_t duration_ms, const std::string& path, MicDataType type, bool runFFT)
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
  newJob->_audioSaveCallback = std::bind(&MicDataSystem::AudioSaveCallback, this, std::placeholders::_1);

  newJob->EnableDataCollect(type, true);
  newJob->SetTimeToRecord(duration_ms);
  newJob->_doFFTProcess = runFFT;
  if (runFFT)
  {
    auto weakptr = std::weak_ptr<FFTResultData>(_fftResultData);
    newJob->_rawAudioFFTCallback = [weakdata = std::move(weakptr)] (std::vector<uint32>&& result) {
      if (auto resultdata = weakdata.lock())
      {
        std::lock_guard<std::mutex> _lock(resultdata->_fftResultMutex);
        resultdata->_fftResultList.push_back(std::move(result));
      }
    };
  }

  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
  }
}

void MicDataSystem::Update(BaseStationTime_t currTime_nanosec)
{
  _fftResultData->_fftResultMutex.lock();
  while (_fftResultData->_fftResultList.size() > 0)
  {
    auto result = std::move(_fftResultData->_fftResultList.front());
    _fftResultData->_fftResultList.pop_front();
    _fftResultData->_fftResultMutex.unlock();

    // Populate the fft result message
    auto msg = RobotInterface::AudioFFTResult();

    for(uint8_t i = 0; i < result.size(); ++i)
    {
      msg.result[i] = result[i];
    }
    RobotInterface::SendAnimToEngine(std::move(msg));


    _fftResultData->_fftResultMutex.lock();
  }
  _fftResultData->_fftResultMutex.unlock();

  bool receivedStopMessage = false;
  static constexpr size_t kMaxReceiveBytes = 2000;
  uint8_t receiveArray[kMaxReceiveBytes];

  const ssize_t bytesReceived = _udpServer->Recv((char*)receiveArray, kMaxReceiveBytes);
  if (bytesReceived > 0) {
    CloudMic::Message msg{receiveArray, (size_t)bytesReceived};
    switch (msg.GetTag()) {

      case CloudMic::MessageTag::stopSignal:
        PRINT_NAMED_INFO("MicDataSystem.Update.RecvCloudProcess.StopSignal", "");
        receivedStopMessage = true;
        break;

      #if ANKI_DEV_CHEATS
      case CloudMic::MessageTag::testStarted:
      {
        PRINT_NAMED_INFO("MicDataSystem.Update.RecvCloudProcess.FakeTrigger", "");
        _fakeStreamingState = true;

        // Set up a message to send out about the triggerword
        RobotInterface::TriggerWordDetected twDetectedMessage;
        const auto mostRecentTimestamp_ms = static_cast<TimeStamp_t>(currTime_nanosec / (1000 * 1000));
        twDetectedMessage.timestamp = mostRecentTimestamp_ms;
        twDetectedMessage.direction = kFirstIndex;
        auto engineMessage = std::make_unique<RobotInterface::RobotToEngine>(std::move(twDetectedMessage));
        {
          std::lock_guard<std::mutex> lock(_msgsMutex);
          _msgsToEngine.push_back(std::move(engineMessage));
        }
        break;
      }
      #endif

      default:
        PRINT_NAMED_INFO("MicDataSystem.Update.RecvCloudProcess.UnexpectedSignal", "0x%x 0x%x", receiveArray[0], receiveArray[1]);
        receivedStopMessage = true;
        break;
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

  if (_forceRecordClip && nullptr == _saveJob)
  {
    MicDataInfo* newJob = new MicDataInfo{};
    newJob->_writeLocationDir = Util::FileUtils::FullFilePath({_writeLocationDir, "debugCapture"});
    newJob->_writeNameBase = ""; // Use the autogen names in this subfolder
    newJob->_numMaxFiles = 100;
    newJob->EnableDataCollect(MicDataType::Processed, true);
    newJob->EnableDataCollect(MicDataType::Raw, true);
    newJob->SetTimeToRecord(kMicData_ClipRecordTime_ms);

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
    if (!_currentlyStreaming && HasStreamingJob()
      #if ANKI_DEV_CHEATS
        && !_forceRecordClip
      #endif
   )
    {
      #if ANKI_DEV_CHEATS
        endTriggerDispTime_ns = currTime_nanosec + kTriggerDisplayTime_ns;
      #endif
      if (_udpServer->HasClient())
      {
        _currentlyStreaming = true;
        _streamingAudioIndex = 0;

        // Send out the message announcing the trigger word has been detected
        auto hw = CloudMic::Hotword{CloudMic::StreamType::Normal, _locale.ToString()};
        if (_currentStreamingJob != nullptr) {
          hw.mode = _currentStreamingJob->_type;
        }
        SendUdpMessage(CloudMic::Message::Createhotword(std::move(hw)));
        PRINT_NAMED_INFO("MicDataSystem.Update.StreamingStart", "");
      }
      else
      {
        ClearCurrentStreamingJob();
        PRINT_NAMED_INFO("MicDataSystem.Update.StreamingStartIgnored", "Ignoring stream start as no clients connected.");
      }
    }

    if (_currentlyStreaming)
    {
      // Are we done with what we want to stream?
      static constexpr size_t kMaxRecordNumChunks = (kStreamingTimeout_ms / kTimePerSEBlock_ms) + 1;
      const bool didTimeout = _streamingAudioIndex >= kMaxRecordNumChunks;
      if (receivedStopMessage || didTimeout)
      {
        ClearCurrentStreamingJob();
        if (didTimeout)
        {
          SendUdpMessage(CloudMic::Message::CreateaudioDone({}));
        }
        PRINT_NAMED_INFO("MicDataSystem.Update.StreamingEnd", "%zu ms", _streamingAudioIndex * kTimePerSEBlock_ms);
        #if ANKI_DEV_CHEATS
          _fakeStreamingState = false;
        #endif
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
              SendUdpMessage(CloudMic::Message::Createaudio(CloudMic::AudioData{audioChunk}));
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
    bool updatedMicDirection = false;
  #endif
  for (const auto& msg : stolenMessages)
  {
    if (msg->tag == RobotInterface::RobotToEngine::Tag_triggerWordDetected)
    {
      RobotInterface::SendAnimToEngine(msg->triggerWordDetected);

      ShowAudioStreamStateManager* showStreamState = _context->GetShowAudioStreamStateManager();
      const bool willStream = HasStreamingJob() && showStreamState->ShouldStreamAfterTriggerWordResponse();
      for(auto func : _triggerWordDetectedCallbacks)
      {
        if(func != nullptr)
        {
          func(willStream);
        }
      }
    }
    else if (msg->tag == RobotInterface::RobotToEngine::Tag_micDirection)
    {
      _latestMicDirectionMsg = msg->micDirection;
      #if ANKI_DEV_CHEATS
        updatedMicDirection = true;
      #endif
      RobotInterface::SendAnimToEngine(msg->micDirection);
    }
    else if (msg->tag == RobotInterface::RobotToEngine::Tag_beatDetectorState)
    {
      RobotInterface::SendAnimToEngine(msg->beatDetectorState);
    }
    else
    {
      DEV_ASSERT_MSG(false,
                     "MicDataSystem.Update.UnhandledOutgoingMessageType",
                     "%s", RobotInterface::RobotToEngine::TagToString(msg->tag));
    }
  }

  const auto& rawBufferFullness = GetIncomingMicDataPercentUsed();
  RobotInterface::MicDataState micDataState{};
  micDataState.rawBufferFullness = rawBufferFullness;
  RobotInterface::SendAnimToEngine(micDataState);

  #if ANKI_DEV_CHEATS
    if (updatedMicDirection || recordingSecondsRemaining != 0)
    {
      FaceInfoScreenManager::getInstance()->DrawConfidenceClock(
        _latestMicDirectionMsg,
        rawBufferFullness,
        recordingSecondsRemaining,
        endTriggerDispTime_ns != 0 || _currentlyStreaming);
    }
  #endif

  // Try to retrieve the speaker latency from the AkAlsaSink plugin. We
  // only need to actually call into the plugin once (or until we get a
  // nonzero latency), since the latency does not change during runtime.
#ifndef SIMULATOR
  if (_speakerLatency_ms == 0) {
    if (_context != nullptr) {
      const auto* audioController = _context->GetAudioController();
      if (audioController != nullptr) {
        const auto* pluginInterface = audioController->GetPluginInterface();
        if (pluginInterface != nullptr) {
          _speakerLatency_ms = pluginInterface->AkAlsaSinkGetSpeakerLatency_ms();
          if (_speakerLatency_ms != 0) {
            PRINT_NAMED_INFO("MicDataSystem.Update.SpeakerLatency",
                             "AkAlsaSink plugin reporting a max speaker latency of %u",
                             (uint32_t) _speakerLatency_ms);
          }
        }
      }
    }
  }
#endif
}

void MicDataSystem::ClearCurrentStreamingJob()
{
  {
    std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
    _currentlyStreaming = false;
    if (_currentStreamingJob != nullptr)
    {
      _currentStreamingJob->SetTimeToRecord(0);
      _currentStreamingJob = nullptr;

      for(auto func : _streamUpdatedCallbacks)
      {
        if(func != nullptr)
        {
          func(false);
        }
      }
    }
  }

  ResetMicListenDirection();
}

void MicDataSystem::ResetMicListenDirection()
{
  _micDataProcessor->ResetMicListenDirection();
}

float MicDataSystem::GetIncomingMicDataPercentUsed()
{
  return _micDataProcessor->GetIncomingMicDataPercentUsed();
}

void MicDataSystem::SendMessageToEngine(std::unique_ptr<RobotInterface::RobotToEngine> msgPtr)
{
  std::lock_guard<std::mutex> lock(_msgsMutex);
  _msgsToEngine.push_back(std::move(msgPtr));
}

bool MicDataSystem::HasStreamingJob() const
{
  std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
  return (_currentStreamingJob != nullptr
  #if ANKI_DEV_CHEATS
    || _fakeStreamingState || _forceRecordClip
  #endif
  );
}

void MicDataSystem::AddMicDataJob(std::shared_ptr<MicDataInfo> newJob, bool isStreamingJob)
{
  std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
  _micProcessingJobs.push_back(std::shared_ptr<MicDataInfo>(newJob));
  if (isStreamingJob)
  {
    _currentStreamingJob = _micProcessingJobs.back();

    for(auto func : _streamUpdatedCallbacks)
    {
      if(func != nullptr)
      {
        func(true);
      }
    }
  }
}

std::deque<std::shared_ptr<MicDataInfo>> MicDataSystem::GetMicDataJobs() const
{
  std::lock_guard<std::recursive_mutex> lock(_dataRecordJobMutex);
  return _micProcessingJobs;
}

void MicDataSystem::UpdateMicJobs()
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

void MicDataSystem::AudioSaveCallback(const std::string& dest)
{
  if (_udpServer->HasClient())
  {
    SendUdpMessage(CloudMic::Message::CreatedebugFile(CloudMic::Filename{dest}));
  }

  // let the world know our recording is now complete
  {
    // we have a buffer length of 255 and our path needs to fit into this buffer
    // shouldn't be a problem, but if we ever hit this we'll need to find another solution
    DEV_ASSERT( dest.length() <= 255, "MicDataSystem::AudioSaveCallback path is too long for MicRecordingComplete message" );

    RobotInterface::MicRecordingComplete event;

    memcpy( event.path, dest.c_str(), dest.length() );
    event.path_length = dest.length();

    AnimProcessMessages::SendAnimToEngine( std::move( event ) );
  }
}

BeatInfo MicDataSystem::GetLatestBeatInfo()
{
  return _micDataProcessor->GetBeatDetector().GetLatestBeat();
}

void MicDataSystem::ResetBeatDetector()
{
  _micDataProcessor->GetBeatDetector().Start();
}

void MicDataSystem::SendUdpMessage(const CloudMic::Message& msg)
{
  std::vector<uint8_t> buf(msg.Size());
  msg.Pack(buf.data(), buf.size());
  _udpServer->Send((const char*)buf.data(), buf.size());
}

void MicDataSystem::UpdateLocale(const Util::Locale& newLocale)
{
  _locale = newLocale;
  _micDataProcessor->UpdateTriggerForLocale(newLocale);
}

bool MicDataSystem::IsSpeakerPlayingAudio() const
{
  if (_context != nullptr) {
    const auto* audioController = _context->GetAudioController();
    if (audioController != nullptr) {
      const auto* pluginInterface = audioController->GetPluginInterface();
      if (pluginInterface != nullptr) {
        return pluginInterface->AkAlsaSinkIsUsingSpeaker();
      }
    }
  }

  return false;
}

bool MicDataSystem::HasConnectionToCloud() const
{
  return _udpServer->HasClient();
}

} // namespace MicData
} // namespace Vector
} // namespace Anki
