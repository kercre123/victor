/**
 * File: firmwareUpdater
 *
 * Author: Mark Wesley
 * Created: 03/25/16
 *
 * Description: Handles firmware updating (up or downgrading) of >=1 Cozmo robot(s)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/firmwareUpdater/firmwareUpdater.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/debug/messageDebugging.h" // for printing sha1

#include <cstring>


namespace Anki {
namespace Cozmo {

const char* const FirmwareUpdater::kFirmwareVersionKey = "version";
const char* const FirmwareUpdater::kFirmwareTimeKey = "time";


FirmwareUpdater::FirmwareUpdater(const CozmoContext* context)
  : _context(context)
  , _numFramesInSubState(0)
  , _numBytesWritten(0)
  , _version(0)
  , _state(FirmwareUpdateStage::Flashing)
  , _subState(FirmwareUpdateSubStage::Init)
{
}
  
  
FirmwareUpdater::~FirmwareUpdater()
{
  WaitForLoadingThreadToExit();
}
  

void FirmwareUpdater::WaitForLoadingThreadToExit()
{
  if (_loadingThread.joinable())
  {
    _loadingThread.join();
  }
}


// Note: LoadFirmwareFile runs in its own thread
void LoadFirmwareFile(AsyncLoaderData* loaderData, std::function<void()> callback)
{
  FILE* fp = fopen(loaderData->GetFilename().c_str(), "rb");
  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    const uint32_t fileSize = Util::numeric_cast<uint32_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    
    loaderData->GetBytes().resize(fileSize);
    size_t bytesRead = fread(&loaderData->GetBytes()[0], 1, fileSize, fp);
    fclose(fp);
    
    if (bytesRead != fileSize)
    {
      PRINT_NAMED_ERROR("LoadFirmwareFile.ReadMismatch", "BytesRead %zu != fileSize %u", bytesRead, fileSize);
      loaderData->GetBytes().clear();
    }
  }
  else
  {
    PRINT_NAMED_ERROR("LoadFirmwareFile.FailedToOpen", "Failed to open '%s' to read", loaderData->GetFilename().c_str());
  }
  
  loaderData->SetComplete();
  if (callback)
  {
    callback();
  }
}
  
std::string GetFirmwareFilename()
{
  return "config/basestation/firmware/v0/cozmo.safe";
}

void FirmwareUpdater::LoadHeader(const JsonCallback& callback)
{
  _fileLoaderData.Init(_context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, GetFirmwareFilename()));

  auto loadCallback = [this, callback]
  {
    // after file is loaded, call LoadHeaderData() with the callback we were given
    LoadHeaderData(_fileLoaderData, callback);
  };
  _loadingThread = std::thread(LoadFirmwareFile, &_fileLoaderData, loadCallback);
}
  
void FirmwareUpdater::SetSubState(const RobotMap& robots, FirmwareUpdateSubStage newState)
{
  PRINT_NAMED_INFO("FirmwareUpdater.SetSubState", "New State %s:%s", EnumToString(_state), EnumToString(newState));
  _subState = newState;
  _numFramesInSubState = 0;
  _numBytesWritten = 0;
  _numBytesProcessed = 0;
  _currentPacketNumber = 0;
  
  SendProgressToGame(robots);
}
  
  
void FirmwareUpdater::GotoFailedState(const RobotMap& robots, FirmwareUpdateResult updateResult)
{
  PRINT_NAMED_INFO("FirmwareUpdater.GotoFailedState", "Was State %s:%s", EnumToString(_state), EnumToString(_subState));
  _state = FirmwareUpdateStage::Failed;
  
  SendCompleteResultToGame(robots, updateResult);
}

  
void FirmwareUpdater::AdvanceState(const RobotMap& robots)
{
  PRINT_NAMED_INFO("FirmwareUpdater.AdvanceState", "Was State %s:%s", EnumToString(_state), EnumToString(_subState));
  _state = (FirmwareUpdateStage)((int)_state + 1);
  SetSubState(robots, FirmwareUpdateSubStage::Init);
}
  

void FirmwareUpdater::VerifyActiveRobots(const RobotMap& robots)
{
  const bool shouldRobotsBeRebooting = ShouldRobotsBeRebooting();
  
  // Reverse through list, removing any Ids that aren't still active
  for (size_t i = _robotsToUpgrade.size(); i > 0; )
  {
    --i;
    RobotUpgradeInfo& robotUpgradeInfo = _robotsToUpgrade[i];
    RobotID_t robotId = robotUpgradeInfo.GetId();
    
    auto it = robots.find(robotId);
    
    const bool isConnected = (it != robots.end());
    
    if (shouldRobotsBeRebooting)
    {
      if (isConnected != robotUpgradeInfo.IsConnected())
      {
        robotUpgradeInfo.SetIsConnected(isConnected);
        PRINT_NAMED_INFO("FirmwareUpdater.VerifyActiveRobots.RobotRebooting", "Robot %u has %s-connected", robotId, isConnected ? "re" : "dis");
      }
    }
    else if (!isConnected)
    {
      PRINT_NAMED_WARNING("FirmwareUpdater.VerifyActiveRobots.RobotRemoved", "Robot %u no longer present!", robotId);
      _robotsToUpgrade.erase(_robotsToUpgrade.begin() + i);
    }
  }
  
  if (_robotsToUpgrade.size() == 0)
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.NoRobotsLeft", "");
    GotoFailedState( robots, FirmwareUpdateResult::Failed_LostAllRobots );
  }
}
  
  
bool FirmwareUpdater::SendToAllRobots(const RobotMap& robots, const RobotInterface::EngineToRobot& msg)
{
  for (RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    auto it = robots.find(robotUpgradeInfo.GetId());
    assert(it != robots.end());
    
    Robot* robot = it->second;
    
    //PRINT_NAMED_DEBUG("FirmwareUpdater.SendToAllRobots", "Sending msg to robot %u", robotUpgradeInfo.GetId());
    const Result res = robot->SendMessage( msg );
    
    if(res == RESULT_OK)
    {
      robotUpgradeInfo.RequestAck();
    }
    else
    {
      PRINT_NAMED_ERROR("FirmwareUpdater.SendToAllRobots.Failed", "To robot %u in State %s:%s res=%x - GotoFailedState!",
                        robotUpgradeInfo.GetId(), EnumToString(_state), EnumToString(_subState), res);
      
      GotoFailedState( robots, FirmwareUpdateResult::Failed_SendingData );
      return false;
    }
  }
  
  return true;
}
  

bool FirmwareUpdater::HaveAllRobotsAcked() const
{
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    if (robotUpgradeInfo.HasAckPending())
    {
      return false;
    }
  }
  
  return true;
}
  
  
bool FirmwareUpdater::ShouldRobotsBeRebooting() const
{
  const bool robotsAreRebooting = (_subState == FirmwareUpdateSubStage::WaitOTAUpgrade);
  return robotsAreRebooting;
}
  
  
bool FirmwareUpdater::HaveAllRobotsRebooted() const
{
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    if (!robotUpgradeInfo.HasRebooted())
    {
      return false;
    }
  }
  
  return true;
}

  
bool FirmwareUpdater::AreAllRobotsDisconnected() const
{
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    if (robotUpgradeInfo.IsConnected())
    {
      return false;
    }
  }
  
  return true;
}


void FirmwareUpdater::SendProgressToGame(const RobotMap& robots, float ratioComplete)
{
  const uint8_t percentComplete = Util::Clamp(uint8_t(ratioComplete * 100.0f), uint8_t(0u), uint8_t(100u));
  
  const std::string& fwSig = GetFwSignature(FirmwareUpdateStage::Flashing);
  
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    auto it = robots.find(robotUpgradeInfo.GetId());
    if (it != robots.end())
    {
      Robot* robot = it->second;
    
      ExternalInterface::FirmwareUpdateProgress message(robot->GetID(), _state, _subState, fwSig, percentComplete);
      robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
    }
    else
    {
      if (!ShouldRobotsBeRebooting())
      {
        PRINT_NAMED_ERROR("SendProgressToGame.MissingRobot", "Missing Robot %u", robotUpgradeInfo.GetId());
      }
    }
  }
}
  

void FirmwareUpdater::SendCompleteResultToGame(const RobotMap& robots, FirmwareUpdateResult updateResult)
{
  const std::string& fwSig = GetFwSignature(FirmwareUpdateStage::Flashing);
  
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    auto it = robots.find(robotUpgradeInfo.GetId());
    if (it != robots.end())
    {
      Robot* robot = it->second;
      
      ExternalInterface::FirmwareUpdateComplete message(robot->GetID(), updateResult, fwSig);
      robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
    }
    else
    {
      if (!ShouldRobotsBeRebooting())
      {
        PRINT_NAMED_ERROR("SendCompleteResultToGame.MissingRobot", "Missing Robot %u", robotUpgradeInfo.GetId());
      }
    }
  }
}
  
  
void FirmwareUpdater::UpdateSubState(const RobotMap& robots)
{
  VerifyActiveRobots(robots);
  
  ++_numFramesInSubState;
  
  const uint32_t kMaxFramesInAnyState = 30 * 30; // = ~30 seconds at 30 fps
  if (_numFramesInSubState > kMaxFramesInAnyState)
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.TookTooLong", "Spent %u frames in State %s:%s - GotoFailedState",
                      _numFramesInSubState, EnumToString(_state), EnumToString(_subState));
    GotoFailedState( robots, FirmwareUpdateResult::Failed_NoResponse );
    return;
  }
    
  switch(_subState)
  {
    case FirmwareUpdateSubStage::Init:
    {
      WaitForLoadingThreadToExit();

      _fileLoaderData.Init( _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, GetFirmwareFilename()) );
      _loadingThread = std::thread(LoadFirmwareFile, &_fileLoaderData, nullptr);
      
      PRINT_NAMED_INFO("FirmwareUpdater.Update.Init", "State %s:%s, loading file:'%s'\n",
             EnumToString(_state), EnumToString(_subState), _fileLoaderData.GetFilename().c_str());
      
      SetSubState( robots, FirmwareUpdateSubStage::LoadingFile );
      
      break;
    }
    case FirmwareUpdateSubStage::LoadingFile:
    {
      if (_fileLoaderData.IsComplete())
      {
        WaitForLoadingThreadToExit();
        
        if (_fileLoaderData.GetBytes().size() == 0)
        {
          PRINT_NAMED_ERROR("FirmwareUpdater.LoadFailed", "Load of file '%s' failed in State %s:%s - GotoFailedState!",
                            _fileLoaderData.GetFilename().c_str(), EnumToString(_state), EnumToString(_subState));

          GotoFailedState( robots, FirmwareUpdateResult::Failed_LoadingFile );
          return;
        }
        else
        {
          LoadHeaderData(_fileLoaderData, [this] (const Json::Value& headerData)
          {
            // if success, try to get the values that we want
            std::string versionString;
            if (headerData.isObject() && JsonTools::GetValueOptional(headerData, kFirmwareVersionKey, versionString))
            {
              GetFwSignature(FirmwareUpdateStage::Flashing) = std::move(versionString);
            }
          });
          SetSubState( robots, FirmwareUpdateSubStage::Flash);
        }
      }
      break;
    }
    case FirmwareUpdateSubStage::Flash:
    {
      if (HaveAllRobotsAcked())
      {
        if (SendWriteMessages(robots))
        {
          SetSubState( robots, FirmwareUpdateSubStage::SendFlashEOF );
        }
        else
        {
          _numFramesInSubState = 0; // only count the time sending a given chunk
          SendProgressToGame( robots, ((float)_numBytesWritten / (float)_fileLoaderData.GetBytes().size()) );
        }
      }
      break;
    }
    case FirmwareUpdateSubStage::SendFlashEOF:
    {
      if (HaveAllRobotsAcked())
      {
        RobotInterface::EngineToRobot msg(RobotInterface::OTA::Write{-1, {}});
        
        //PRINT_NAMED_INFO("FirmwareUpdater.SendFlashEOF", "Sending SendFlashEOF(%u, %d, %s)",
        //                 flashAddress, _version, GetFwSignature(_state).c_str());
        
        if (!SendToAllRobots(robots, msg))
        {
          return;
        }
        
        SetSubState(robots, FirmwareUpdateSubStage::WaitOTAUpgrade);
      }
      
      break;
    }
    case FirmwareUpdateSubStage::WaitOTAUpgrade:
    {
      const bool shouldRobotsBeRebooting = ShouldRobotsBeRebooting();
      const bool areAllRobotsDisconnected = AreAllRobotsDisconnected();
      
      // until we can reconnect to the robot after reboot, might as well go directly complete ASAP
      const uint32_t kNumFramesToWaitForOTAUpgrade = shouldRobotsBeRebooting ? 1 : 0;
      
      if ((shouldRobotsBeRebooting && areAllRobotsDisconnected) || (_numFramesInSubState >= kNumFramesToWaitForOTAUpgrade))
      {
        AdvanceState(robots);
      }
      else
      {
        SendProgressToGame( robots, ((float)_numFramesInSubState / (float)kNumFramesToWaitForOTAUpgrade) );
      }
      
      break;
    }
  }
}
  
void FirmwareUpdater::LoadHeaderData(AsyncLoaderData& fileLoaderData, const JsonCallback& callback)
{
  if (!fileLoaderData.IsComplete())
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.LoadHeaderData", "Firmware file not yet loaded");
    return;
  }
  
  constexpr size_t kFirmwareHeaderMaxSize = 1024 * 2;
  const size_t fileSize = fileLoaderData.GetBytes().size();
  if (fileSize < kFirmwareHeaderMaxSize)
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.LoadHeaderData", "Firmware file %zu bytes when expected to be > %zu bytes.", fileSize, kFirmwareHeaderMaxSize);
    return;
  }
  
  const char* stringBegin = (const char*) fileLoaderData.GetBytes().data();
  
  // use std::memchr to find the nullptr at the end of the json string (otherwise assume using the whole 2k)
  const char* stringEnd = (char*) std::memchr(stringBegin, '\0', kFirmwareHeaderMaxSize);
  if (nullptr == stringEnd)
  {
    stringEnd = stringBegin + kFirmwareHeaderMaxSize;
  }
  
  Json::Reader jsonReader;
  Json::Value headerData;
  if (jsonReader.parse(stringBegin, stringEnd, headerData))
  {
    if (callback)
    {
      callback(headerData);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.LoadHeaderData.ReadHeaderFailed", "%s", jsonReader.getFormattedErrorMessages().c_str());
  }
}
  
// Returns true if entire file has been sent, false otherwise
bool FirmwareUpdater::SendWriteMessages(const RobotMap& robots)
{
  RobotInterface::OTA::Write writeMsg;
  int32_t kPacketSize = writeMsg.data.size();
  int32_t kTotalFileSize = Util::numeric_cast<int32_t>(_fileLoaderData.GetBytes().size());
  int32_t numBytesToSendNextMsg = Util::Min(kTotalFileSize - Util::numeric_cast<int32_t>(_numBytesWritten), kPacketSize);
  int32_t emptySpace = (size_t)AnimConstants::KEYFRAME_BUFFER_SIZE - (_numBytesWritten - _numBytesProcessed);
  
  while (_numBytesWritten < kTotalFileSize && (emptySpace - numBytesToSendNextMsg > 0))
  {
    writeMsg = {}; // Using bracket init to zero out message to begin with
    writeMsg.packetNumber = _currentPacketNumber++;
    memcpy(writeMsg.data.data(), &_fileLoaderData.GetBytes()[_numBytesWritten], numBytesToSendNextMsg);
    
    //PRINT_NAMED_DEBUG("FirmwareUpdater::SendWriteMessages",
    //                  "Empty: %d NumSending: %d NumWritten: %d NumProcessed: %d",
    //                  emptySpace, numBytesToSendNextMsg, _numBytesWritten, _numBytesProcessed);
    
    if (!SendToAllRobots(robots, RobotInterface::EngineToRobot(std::move(writeMsg))))
    {
      return false;
    }
    
    _numBytesWritten += numBytesToSendNextMsg;
    emptySpace -= numBytesToSendNextMsg;
    numBytesToSendNextMsg = Util::Min(kTotalFileSize - Util::numeric_cast<int32_t>(_numBytesWritten), kPacketSize);
  }
  
  return _numBytesWritten == kTotalFileSize;
}

  
void FirmwareUpdater::HandleFlashWriteAck(RobotID_t robotId, const RobotInterface::OTA::Ack& flashWriteAck)
{
  for (RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    if (robotUpgradeInfo.GetId() == robotId)
    {
      if (flashWriteAck.result != RobotInterface::OTA::Result::OKAY &&
          flashWriteAck.result != RobotInterface::OTA::Result::SUCCESS)
      {
        PRINT_NAMED_ERROR("FirmwareUpdater.HandleFlashWriteAck.Error",
                          "Robot %u flash ack %d had error %s on %d bytes processed.",
                          robotId, flashWriteAck.packetNumber,
                          RobotInterface::OTA::EnumToString(flashWriteAck.result),
                          flashWriteAck.bytesProcessed);
        // TODO:(lc) This is hacky. Update FirmwareUpdater to have some internal state for handling failureon the next
        // Update() call (from cozmoEngine) rather than going directly to the robot map in the robotmanager here, in order
        // to go to the failed state.
        GotoFailedState( _context->GetRobotManager()->GetRobotMap(), FirmwareUpdateResult::Failed_SendingData );
      }
      else
      {
        _numBytesProcessed = flashWriteAck.bytesProcessed;
      }
      
      robotUpgradeInfo.ConfirmAck();
      return;
    }
  }
  
  PRINT_NAMED_WARNING("FirmwareUpdater.HandleFlashWriteAck.NoRobot", "No Robot entry with id %u", robotId);
}

  
bool FirmwareUpdater::InitUpdate(const RobotMap& robots, int version)
{
  IExternalInterface* externalInterface = _context->GetExternalInterface();
  if (nullptr == externalInterface)
  {
    PRINT_NAMED_ERROR("FirmwareUpdater.InitUpdate.NullExternalInterface", "");
  }
  
  _robotsToUpgrade.clear();
  for (const auto& kv : robots)
  {
    RobotID_t robotId = kv.first;
    
    _robotsToUpgrade.emplace_back(robotId);
    
    if (externalInterface)
    {
      // Cancel animations/sounds
      ExternalInterface::CancelAction cancelActionMsg(RobotActionType::UNKNOWN, robotId);
      externalInterface->Broadcast(ExternalInterface::MessageGameToEngine(std::move(cancelActionMsg)));
    }
  }
  
  for (size_t i = 0; i < kFwSignatureCount; ++i)
  {
    _fwSignatures[i] = "";
  }
  
  if (robots.size() == 0)
  {
    PRINT_NAMED_WARNING("FirmwareUpdater.InitUpdate.NoRobots", "Need >0 robots to update firmware");
    SendCompleteResultToGame(robots, FirmwareUpdateResult::Failed_NoRobots);
    return false;
  }
  
  _version = version;
  _state   = FirmwareUpdateStage::Flashing;
  SetSubState(robots, FirmwareUpdateSubStage::Init);
  
  _eventHandles.clear();
  
  for (const auto& kv : robots)
  {
    RobotID_t robotId = kv.first;
    Robot* robot = kv.second;
    
    auto handleFlashWriteAck = [this, robotId](const AnkiEvent<RobotInterface::RobotToEngine>& event)
    {
      const RobotInterface::OTA::Ack& flashWriteAck = event.GetData().Get_otaAck();
      this->HandleFlashWriteAck(robotId, flashWriteAck);
    };

    _eventHandles.push_back(robot->GetRobotMessageHandler()->Subscribe(robot->GetID(),
                                                                       RobotInterface::RobotToEngineTag::otaAck,
                                                                       handleFlashWriteAck));
   
    PRINT_NAMED_INFO("FirmwareUpdater.InitUpdate", "Init update to version %d for robot %d", version, (int)robot->GetID());
  }

  return true;
}

  
bool FirmwareUpdater::Update(const RobotMap& robots)
{
  bool isComplete = false;
  
  if (!IsComplete())
  {
    VerifyActiveRobots(robots);
  }
  
  switch(_state)
  {
    case FirmwareUpdateStage::Flashing:
      UpdateSubState(robots);
      break;
    case FirmwareUpdateStage::Done:
      SendCompleteResultToGame(robots, FirmwareUpdateResult::Success);
      isComplete = true;
      break;
    case FirmwareUpdateStage::Failed:
      isComplete = true;
      break;
  }
  
  if (isComplete)
  {
    CleanupOnExit();
  }
  
  return isComplete;
}
      
      
bool FirmwareUpdater::IsComplete() const
{
  switch(_state)
  {
    case FirmwareUpdateStage::Done:
    case FirmwareUpdateStage::Failed:
      return true;
    default:
      return false;
  }
}

  
void FirmwareUpdater::CleanupOnExit()
{
  _eventHandles.clear();
}

  
} // namespace Cozmo
} // namespace Anki
