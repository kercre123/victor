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
#include "anki/cozmo/basestation/firmwareUpdater/sha1/sha1.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/debug/messageDebugging.h" // for printing sha1


namespace Anki {
namespace Cozmo {


FirmwareUpdater::FirmwareUpdater(const CozmoContext* context)
  : _context(context)
  , _numFramesInSubState(0)
  , _numBytesWritten(0)
  , _version(0)
  , _state(FirmwareUpdateStage::Wifi)
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
void LoadFirmwareFile(AsyncLoaderData* loaderData)
{
  FILE* fp = fopen(loaderData->GetFilename().c_str(), "rb");
  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    const uint32_t fileSize = Util::numeric_cast<uint32_t>(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    
    loaderData->GetBytes().resize(fileSize + sizeof(fileSize));
    size_t bytesRead = fread(&loaderData->GetBytes()[sizeof(fileSize)], 1, fileSize, fp);
    fclose(fp);
    
    if (bytesRead != fileSize)
    {
      PRINT_NAMED_ERROR("LoadFirmwareFile.ReadMismatch", "BytesRead %zu != fileSize %u", bytesRead, fileSize);
      loaderData->GetBytes().clear();
    }
    else
    {
      // write the fileSize into the header
      memcpy(&loaderData->GetBytes()[0], &fileSize, sizeof(fileSize));
    }
  }
  else
  {
    PRINT_NAMED_ERROR("LoadFirmwareFile.FailedToOpen", "Failed to open '%s' to read", loaderData->GetFilename().c_str());
  }
  
  loaderData->SetComplete();
}
  
  
void GenerateSHA1(const std::vector<uint8_t>& dataBytes, size_t headerSize, std::array<uint8_t, SHA1_BLOCK_SIZE>& outSig)
{
  assert(dataBytes.size() > headerSize);
  
  SHA1_CTX sha1Ctx;
  sha1_init(&sha1Ctx);
  
  sha1_update(&sha1Ctx, &dataBytes[headerSize], dataBytes.size() - headerSize);
  
  uint8_t sha1Hash[SHA1_BLOCK_SIZE];
  sha1_final(&sha1Ctx, sha1Hash);
  
  for (int i = 0; i < SHA1_BLOCK_SIZE; ++i)
  {
    outSig[i] = sha1Hash[i];
  }
}

  
const char* GetFileNameForState(FirmwareUpdateStage state)
{
  switch(state)
  {
    case FirmwareUpdateStage::Wifi:
      return "esp.user.bin";
    case FirmwareUpdateStage::RTIP:
      return "robot.safe";
    case FirmwareUpdateStage::Body:
      return "syscon.safe";
    default:
      assert(0);
      return "";
  }
}

  
Anki::Cozmo::RobotInterface::OTACommand GetOTACommandForState(FirmwareUpdateStage state)
{
  switch(state)
  {
    case FirmwareUpdateStage::Wifi:
      return RobotInterface::OTACommand::OTA_none;
    case FirmwareUpdateStage::RTIP:
      return RobotInterface::OTACommand::OTA_none;
    case FirmwareUpdateStage::Body:
      return RobotInterface::OTACommand::OTA_stage;
    default:
      assert(0);
      return RobotInterface::OTACommand::OTA_none;
  }
}

  
uint32_t GetFlashAddressForState(FirmwareUpdateStage state)
{
  RobotInterface::OTAFlashRegions flashRegion = RobotInterface::OTAFlashRegions::OTA_WiFi_flash_address;
  switch(state)
  {
    case FirmwareUpdateStage::Wifi:
      flashRegion = RobotInterface::OTAFlashRegions::OTA_WiFi_flash_address;
      break;
    case FirmwareUpdateStage::RTIP:
      flashRegion = RobotInterface::OTAFlashRegions::OTA_RTIP_flash_address;
      break;
//    case FirmwareUpdateStage::Body:
//      flashRegion = RobotInterface::OTAFlashRegions::OTA_body_flash_address;
//      break;
    default:
      assert(0);
  }
  
  const uint32_t kSectorSize = 0x1000;
  const uint32_t flashAddress = Util::numeric_cast<uint32_t>(flashRegion);
  if ((flashAddress % kSectorSize) != 0)
  {
    PRINT_NAMED_ERROR("GetFlashAddressForState.BadAddress", "Flash address %u is not a multiple of sector size %u",
                      flashAddress, kSectorSize);
    assert(0);
  }
  
  return flashAddress;
}
  
  
void FirmwareUpdater::SetSubState(const RobotMap& robots, FirmwareUpdateSubStage newState)
{
  PRINT_NAMED_INFO("FirmwareUpdater.SetSubState", "New State %s:%s", EnumToString(_state), EnumToString(newState));
  _subState = newState;
  _numFramesInSubState = 0;
  _numBytesWritten = 0;
  
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
  const bool robotsAreRebooting = (_subState == FirmwareUpdateSubStage::WaitOTAUpgrade) &&
                                  (GetOTACommandForState(_state) != RobotInterface::OTACommand::OTA_none);
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
  
  const std::string& fwSigWifi = GetFwSignature(FirmwareUpdateStage::Wifi);
  const std::string& fwSigRTIP = GetFwSignature(FirmwareUpdateStage::RTIP);
  const std::string& fwSigBody = GetFwSignature(FirmwareUpdateStage::Body);
  
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    auto it = robots.find(robotUpgradeInfo.GetId());
    if (it != robots.end())
    {
      Robot* robot = it->second;
    
      ExternalInterface::FirmwareUpdateProgress message(robot->GetID(), _state, _subState, fwSigWifi, fwSigRTIP, fwSigBody, percentComplete);
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
  const std::string& fwSigWifi = GetFwSignature(FirmwareUpdateStage::Wifi);
  const std::string& fwSigRTIP = GetFwSignature(FirmwareUpdateStage::RTIP);
  const std::string& fwSigBody = GetFwSignature(FirmwareUpdateStage::Body);
  
  for (const RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    auto it = robots.find(robotUpgradeInfo.GetId());
    if (it != robots.end())
    {
      Robot* robot = it->second;
      
      ExternalInterface::FirmwareUpdateComplete message(robot->GetID(), updateResult, fwSigWifi, fwSigRTIP, fwSigBody);
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
      
      std::string relativePath = std::string("config/basestation/firmware/v0/") + GetFileNameForState(_state);
      
      _fileLoaderData.Init( _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, relativePath) );
      _loadingThread = std::thread(LoadFirmwareFile, &_fileLoaderData);
      
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
          const uint32_t flashAddress = GetFlashAddressForState(_state);
          const uint32_t firmwareSize = Util::numeric_cast<uint32_t>(_fileLoaderData.GetBytes().size());
          
          RobotInterface::EngineToRobot msg( RobotInterface::EraseFlash(flashAddress, firmwareSize) );
          
          PRINT_NAMED_INFO("FirmwareUpdater.LoadingFile", "Sending EraseFlash(%u, %u)",
                           flashAddress, firmwareSize);

          if (!SendToAllRobots(robots, msg))
          {
            return;
          }
        
          SetSubState( robots, FirmwareUpdateSubStage::Flash);
        }
      }
      break;
    }
    case FirmwareUpdateSubStage::Flash:
    {
      if (HaveAllRobotsAcked())
      {
        const uint32_t flashAddress = GetFlashAddressForState(_state);
        
        const size_t kChunkSize = 1024;
        const size_t numBytesLeftToSend = _fileLoaderData.GetBytes().size() - _numBytesWritten;
        const size_t numBytesToSendThisMsg = Util::Min(numBytesLeftToSend, kChunkSize);
        
        std::vector<uint8_t> chunk;
        chunk.resize(numBytesToSendThisMsg);
        memcpy(&chunk[0], &_fileLoaderData.GetBytes()[_numBytesWritten], numBytesToSendThisMsg);
        
        RobotInterface::EngineToRobot msg(RobotInterface::WriteFlash(flashAddress+_numBytesWritten, chunk));
        
        //PRINT_NAMED_DEBUG("FirmwareUpdater.Flash", "Sending WriteFlash(%u+%u, %zu bytes)", flashAddress,_numBytesWritten, chunk.size());
        
        if (!SendToAllRobots(robots, msg))
        {
          return;
        }
        
        _numBytesWritten += chunk.size();
        
        if (_numBytesWritten >= _fileLoaderData.GetBytes().size())
        {
          SetSubState( robots, FirmwareUpdateSubStage::SendOTAUpgrade );
        }
        else
        {
          _numFramesInSubState = 0; // only count the time sending a given chunk
          SendProgressToGame( robots, ((float)_numBytesWritten / (float)_fileLoaderData.GetBytes().size()) );
        }
      }
      break;
    }
    case FirmwareUpdateSubStage::SendOTAUpgrade:
    {
      if (HaveAllRobotsAcked())
      {
        const uint32_t flashAddress = GetFlashAddressForState(_state);
        
        std::array<uint8_t, SHA1_BLOCK_SIZE> sig;
        GenerateSHA1(_fileLoaderData.GetBytes(), sizeof(uint32_t), sig);
        Anki::Cozmo::RobotInterface::OTACommand oTACommand = GetOTACommandForState(_state);
        
        GetFwSignature(_state) = Util::ConvertMessageBufferToString(&sig[0], SHA1_BLOCK_SIZE, Util::eBTTT_Hex, false);
        
        RobotInterface::EngineToRobot msg(RobotInterface::OTAUpgrade(flashAddress, _version, sig, oTACommand));
        
        PRINT_NAMED_INFO("FirmwareUpdater.SendOTAUpgrade", "Sending OTAUpgrade(%u, %d, %s, %s)",
                         flashAddress, _version, GetFwSignature(_state).c_str(), EnumToString(oTACommand));
        
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

  
void FirmwareUpdater::HandleFlashWriteAck(RobotID_t robotId, const RobotInterface::FlashWriteAcknowledge& flashWriteAck)
{
  for (RobotUpgradeInfo& robotUpgradeInfo : _robotsToUpgrade)
  {
    if (robotUpgradeInfo.GetId() == robotId)
    {
      if (!robotUpgradeInfo.HasAckPending())
      {
        PRINT_NAMED_WARNING("FirmwareUpdater.HandleFlashWriteAck.NoAckPending", "Robot id %u had no pending ack!?", robotId);
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
  for (auto kv : robots)
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
  _state   = FirmwareUpdateStage::Wifi;
  SetSubState(robots, FirmwareUpdateSubStage::Init);
  
  _eventHandles.clear();
  
  for (auto kv : robots)
  {
    RobotID_t robotId = kv.first;
    Robot* robot = kv.second;
    
    auto handleFlashWriteAck = [this, robotId](const AnkiEvent<RobotInterface::RobotToEngine>& event)
    {
      const RobotInterface::FlashWriteAcknowledge& flashWriteAck = event.GetData().Get_flashWriteAck();
      this->HandleFlashWriteAck(robotId, flashWriteAck);
    };

    _eventHandles.push_back(robot->GetRobotMessageHandler()->Subscribe(robot->GetID(),
                                                                       RobotInterface::RobotToEngineTag::flashWriteAck,
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
    case FirmwareUpdateStage::Wifi:
    case FirmwareUpdateStage::RTIP:
    case FirmwareUpdateStage::Body:
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
