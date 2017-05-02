/**
 * File: needsManager
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: Manages the Needs for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/common/types.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

const char* NeedsManager::kLogChannelName = "NeedsSystem";


static const std::string kNeedsStateFile = "needsState.json";

static const std::string kStateFileVersionKey = "_StateFileVersion";
static const std::string kDateTimeKey = "_DateTime";
static const std::string kSerialNumberKey = "_SerialNumber";

static const std::string kCurNeedLevelKey = "CurNeedLevel";
static const std::string kPartIsDamagedKey = "PartIsDamaged";
static const std::string kCurNeedsUnlockLevelKey = "CurNeedsUnlockLevel";
static const std::string kNumStarsAwardedKey = "NumStarsAwarded";
static const std::string kNumStarsForNextUnlockKey = "NumStarsForNextUnlock";

static const float kNeedLevelStorageMultiplier = 100000.0f;


static const float kConnectedDecayPeriod_s = 3.0f; // 3 seconds for testing; soon to be 60


using Time = std::chrono::time_point<std::chrono::system_clock>;



NeedsManager::NeedsManager(Robot& robot)
: _robot(robot)
, _needsState()
, _needsConfig()
, _needsStateFromRobot()
, _isPaused(false)
, _currentTime_s(0.0f)
, _timeForNextPeriodicDecay_s(0.0f)
, _pausedDurRemainingPeriodicDecay(0.0f)
, kPathToSavedStateFile((_robot.GetContextDataPlatform() != nullptr ? _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, GetNurtureFolder()) : ""))
, _robotStorageState(RobotStorageState::Inactive)
{
}

NeedsManager::~NeedsManager()
{
  _signalHandles.clear();
}


void NeedsManager::Init(const Json::Value& inJson)
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.Init", "Starting Init of NeedsManager");
  _needsConfig.Init(inJson);

  const u32 uninitializedSerialNumber = 0;
  _needsState.Init(_needsConfig, uninitializedSerialNumber);

  // todo:  Init various member vars of NeedsManager as needed
  
  // Subscribe to messages
  if (_robot.HasExternalInterface())
  {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterNeedsActionCompleted>();
  }

  // Subscribe to message from robot to get serial number;
  // when we receive this message we will continue the initialization process
  RobotInterface::MessageHandler *messageHandler = _robot.GetContext()->GetRobotManager()->GetMsgHandler();
  _signalHandles.push_back(messageHandler->Subscribe(_robot.GetID(),
                                                     RobotInterface::RobotToEngineTag::mfgId,
                                                     std::bind(&NeedsManager::HandleMfgID, this, std::placeholders::_1)));
}

void NeedsManager::InitAfterConnection()
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterConnection",
                      "Starting MAIN Init of NeedsManager, with serial number %d", _robot.GetBodySerialNumber());
  _needsState.Init(_needsConfig, _robot.GetBodySerialNumber());

  // todo:  later this logic needs to deal with loading from robot storage, comparing time
  // stamps with device storage, and dealing with multiple device use
  // Also need to deal with multiple robots (even if just for dev purposes)

  Time now = std::chrono::system_clock::now();
  const bool deviceHasNeedsState = DeviceHasNeedsState();
  if (deviceHasNeedsState)
  {
    bool versionUpdated = false;
    if (ReadFromDevice(_needsState, versionUpdated))
    {
      if (versionUpdated)
      {
        // If we've loaded an older verson of the state file, write out the newer version immediately
        _needsState._timeLastWritten = now;
        WriteToDevice(_needsState);
      }
    }
  }
  else
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterConnection", "No needs state data found on device; creating new file");
    _needsState._timeLastWritten = now;
    WriteToDevice(_needsState);
  }

  StartReadFromRobot();
//  if (error)
//  {
//    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterConnection", "No needs state data found on robot; writing to robot");
//    _needsState._timeLastWritten = now;
//    StartWriteToRobot(_needsState);
//  }
//  else
//  {
////    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterConnection",
////                        "robotHasNeedsState is %s", robotHasNeedsState ? "true" : "false");
//  }
}


void NeedsManager::HandleMfgID(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto& payload = message.GetData().Get_mfgId();
  _needsState._robotSerialNumber = payload.esn;
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.HandleMfgID", "HandleMfgID holds esn %d", payload.esn);

  InitAfterConnection();
}



void NeedsManager::Update(const float currentTime_s)
{
//  if (!_robot.GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::NeedsSystem))
//    return;

  _currentTime_s = currentTime_s;

  if (_isPaused)
    return;

  // Don't do anything while reading/writing robot data
  if (_robotStorageState != RobotStorageState::Inactive)
    return;

  if (NEAR_ZERO(_timeForNextPeriodicDecay_s))
  {
    _timeForNextPeriodicDecay_s = currentTime_s + kConnectedDecayPeriod_s;
  }

  if (currentTime_s >= _timeForNextPeriodicDecay_s)
  {
    _timeForNextPeriodicDecay_s += kConnectedDecayPeriod_s;

    _needsState.ApplyDecay(kConnectedDecayPeriod_s, _needsConfig._decayConnected);

    SendNeedsStateToGame(NeedsActionId::Decay);
  }
}


void NeedsManager::SetPaused(bool paused)
{
  if (paused == _isPaused)
  {
    DEV_ASSERT_MSG(paused != _isPaused, "NeedsManager.SetPaused.Redundant",
                   "Setting paused to %s but already in that state",
                   paused ? "true" : "false");
    return;
  }

  _isPaused = paused;

  if (_isPaused)
  {
    // Calculate and record how much time was left until the next decay
    _pausedDurRemainingPeriodicDecay = _timeForNextPeriodicDecay_s - _currentTime_s;

    // Should we send the current needs state to the game as soon as we pause?
    //  (because the periodic decay won't happen during pause)
  }
  else
  {
    // When unpausing, set the next 'time for periodic decay'
    _timeForNextPeriodicDecay_s = _currentTime_s + _pausedDurRemainingPeriodicDecay;
  }

  SendNeedsPauseStateToGame();
}


void NeedsManager::RegisterNeedsActionCompleted(NeedsActionId actionCompleted)
{
  // todo implement this
  
  // todo: Adjust zero or more needs levels based on config data for the completed action
  
  SendNeedsStateToGame(actionCompleted);
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsState& msg)
{
  SendNeedsStateToGame(NeedsActionId::NoAction);
}
  
template<>
void NeedsManager::HandleMessage(const ExternalInterface::SetNeedsPauseState& msg)
{
  SetPaused(msg.pause);
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsPauseState& msg)
{
  SendNeedsPauseStateToGame();
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::RegisterNeedsActionCompleted& msg)
{
  RegisterNeedsActionCompleted(msg.actionCompleted);
}


void NeedsManager::SendNeedsStateToGame(NeedsActionId actionCausingTheUpdate)
{
  _needsState.UpdateCurNeedsBrackets();
  
  std::vector<float> needLevels;
  needLevels.reserve((size_t)NeedId::Count);
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    float level = _needsState.GetNeedLevelByIndex(i);
    needLevels.push_back(level);
  }
  
  std::vector<NeedBracketId> needBrackets;
  needBrackets.reserve((size_t)NeedBracketId::Count);
  for (size_t i = 0; i < (size_t)NeedBracketId::Count; i++)
  {
    NeedBracketId bracketId = _needsState.GetNeedBracketByIndex(i);
    needBrackets.push_back(bracketId);
  }
  
  std::vector<bool> partIsDamaged;
  partIsDamaged.reserve((size_t)RepairablePartId::Count);
  for (size_t i = 0; i < (size_t)RepairablePartId::Count; i++)
  {
    bool isDamaged = _needsState.GetPartIsDamagedByIndex(i);
    partIsDamaged.push_back(isDamaged);
  }
  
  ExternalInterface::NeedsState message(std::move(needLevels), std::move(needBrackets),
                                        std::move(partIsDamaged), _needsState._curNeedsUnlockLevel,
                                        _needsState._numStarsAwarded, _needsState._numStarsForNextUnlock,
                                        actionCausingTheUpdate);
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendNeedsPauseStateToGame()
{
  ExternalInterface::NeedsPauseState message(_isPaused);
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendAllNeedsMetToGame()
{
  // todo
//  message AllNeedsMet {
//    int_32 unlockLevelCompleted,
//    int_32 starsRequiredForNextUnlock,
//    // TODO:  List of rewards unlocked
//  }
  std::vector<NeedsReward> rewards;
  
  ExternalInterface::AllNeedsMet message(0, 1, std::move(rewards)); // todo fill in
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}


bool NeedsManager::DeviceHasNeedsState()
{
  return Util::FileUtils::FileExists(kPathToSavedStateFile + kNeedsStateFile);
}

void NeedsManager::WriteToDevice(const NeedsState& needsState)
{
  Json::Value state;

  state[kStateFileVersionKey] = needsState.kDeviceStorageVersion;

  using namespace std::chrono;
  const auto time_s = duration_cast<seconds>(needsState._timeLastWritten.time_since_epoch()).count();
  state[kDateTimeKey] = Util::numeric_cast<Json::LargestInt>(time_s);

  state[kSerialNumberKey] = needsState._robotSerialNumber;

  state[kCurNeedsUnlockLevelKey] = needsState._curNeedsUnlockLevel;
  state[kNumStarsAwardedKey] = needsState._numStarsAwarded;
  state[kNumStarsForNextUnlockKey] = needsState._numStarsForNextUnlock;

  for (const auto& need : needsState._curNeedsLevels)
  {
    const int levelAsInt = Util::numeric_cast<int>((need.second * kNeedLevelStorageMultiplier) + 0.5f);
    state[kCurNeedLevelKey][EnumToString(need.first)] = levelAsInt;
  }
  for (const auto& part : needsState._partIsDamaged)
  {
    state[kPartIsDamagedKey][EnumToString(part.first)] = part.second;
  }

  if (!_robot.GetContextDataPlatform()->writeAsJson(Util::Data::Scope::Persistent, GetNurtureFolder() + kNeedsStateFile, state))
  {
    PRINT_NAMED_ERROR("NeedsManager.WriteToDevice.WriteStateFailed", "Failed to write needs state file");
  }
//  auto tickEnd = std::chrono::system_clock::now();
//  auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(tickEnd - tickStart);
//  PRINT_CH_INFO(kLogChannelName, "NeedsManager.WriteToDevice", "Write to device took %lld microseconds", microsecs.count());
}

bool NeedsManager::ReadFromDevice(NeedsState& needsState, bool& versionUpdated)
{
  versionUpdated = false;

  Json::Value state;
  if (!_robot.GetContextDataPlatform()->readAsJson(kPathToSavedStateFile + kNeedsStateFile, state))
  {
    PRINT_NAMED_ERROR("NeedsManager.ReadFromDevice.ReadStateFailed", "Failed to read %s", kNeedsStateFile.c_str());
    return false;
  }

  int versionLoaded = state[kStateFileVersionKey].asInt();
  if (versionLoaded > needsState.kDeviceStorageVersion)
  {
    ANKI_VERIFY(versionLoaded <= needsState.kDeviceStorageVersion, "NeedsManager.ReadFromDevice.StateFileVersionIsFuture",
                "Needs state file version read was %d but app thinks latest version is %d",
                versionLoaded, needsState.kDeviceStorageVersion);
    return false;
  }

  using namespace std::chrono;
  const seconds durationSinceEpoch_s(state[kDateTimeKey].asLargestInt());
  needsState._timeLastWritten = time_point<system_clock>(durationSinceEpoch_s);

  needsState._robotSerialNumber = state[kSerialNumberKey].asUInt();

  needsState._curNeedsUnlockLevel = state[kCurNeedsUnlockLevelKey].asInt();
  needsState._numStarsAwarded = state[kNumStarsAwardedKey].asInt();
  needsState._numStarsForNextUnlock = state[kNumStarsForNextUnlockKey].asInt();

  for (auto& need : needsState._curNeedsLevels)
  {
    const int levelAsInt = state[kCurNeedLevelKey][EnumToString(need.first)].asInt();
    need.second = (Util::numeric_cast<float>(levelAsInt) / kNeedLevelStorageMultiplier);
  }
  for (auto& part : needsState._partIsDamaged)
  {
    part.second = state[kPartIsDamagedKey][EnumToString(part.first)].asBool();
  }

  if (versionLoaded < needsState.kDeviceStorageVersion) {
    versionUpdated = true;
    // Sensible defaults go here for new stuff, as needed, when the format changes
  }

  _needsState.UpdateCurNeedsBrackets();

  return true;
}


void NeedsManager::StartWriteToRobot(const NeedsState& needsState)
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Inactive, "NeedsManager.StartWriteToRobot.RobotStorageConflict",
              "Attempting to write to robot but state is %d", _robotStorageState);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Writing to robot...");
  ANKI_CPU_PROFILE("NeedsState::StartWriteToRobot");
  auto tickStart = std::chrono::system_clock::now();

  _robotStorageState = RobotStorageState::Writing;

  using namespace std::chrono;
  const auto time_s = duration_cast<seconds>(needsState._timeLastWritten.time_since_epoch()).count();
  const auto timeLastWritten = Util::numeric_cast<uint64_t>(time_s);

  std::array<int32_t, MAX_NEEDS> curNeedLevels{};
  for (const auto& need : needsState._curNeedsLevels)
  {
    const int32_t levelAsInt = Util::numeric_cast<int>((need.second * kNeedLevelStorageMultiplier) + 0.5f);
    curNeedLevels[(int)need.first] = levelAsInt;
  }

  std::array<bool, MAX_REPAIRABLE_PARTS> partIsDamaged{};
  for (const auto& part : needsState._partIsDamaged)
  {
    partIsDamaged[(int)part.first] = part.second;
  }

  NeedsStateOnRobot stateForRobot(NeedsState::kRobotStorageVersion, timeLastWritten, curNeedLevels,
                                  needsState._curNeedsUnlockLevel, needsState._numStarsAwarded, partIsDamaged);

  std::vector<u8> stateVec(stateForRobot.Size());
  stateForRobot.Pack(stateVec.data(), stateForRobot.Size());
  if (!_robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_NurtureGameData, stateVec.data(), stateVec.size(),
                                           [this, tickStart](NVStorage::NVResult res)
                                           {
                                             FinishWriteToRobot(res);
                                           }))
  {
    PRINT_NAMED_ERROR("NeedsState.StartWriteToRobot.WriteFailed", "Write failed");
    _robotStorageState = RobotStorageState::Inactive;
  }
  auto tickEnd = std::chrono::system_clock::now();
  auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(tickEnd - tickStart);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Write to robot START took %lld microseconds", microsecs.count());
}

void NeedsManager::FinishWriteToRobot(NVStorage::NVResult res)
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Writing, "NeedsManager.FinishWriteToRobot.RobotStorageConflict",
              "Robot storage state should be Writing but instead is %d", _robotStorageState);
  _robotStorageState = RobotStorageState::Inactive;

//  auto tickEnd = std::chrono::system_clock::now();
//  auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(tickEnd - tickStart);
//  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Write to robot AFTER CALLBACK took %lld microseconds", microsecs.count());

  if (res < NVStorage::NVResult::NV_OKAY)
  {
    PRINT_NAMED_ERROR("NeedsState.FinishWriteToRobot.WriteFailed", "Write failed with %s", EnumToString(res));
  }
  else
  {
    // The write was successful
    // xxx send a message to the game to indicate write was completed??
    if (_robot.HasExternalInterface())
    {
      //_robot.GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RequestSetUnlockResult(id, unlocked)));
    }
  }
}


bool NeedsManager::StartReadFromRobot()
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Inactive, "NeedsManager.StartReadFromRobot.RobotStorageConflict",
              "Attempting to read from robot but state is %d", _robotStorageState);
  _robotStorageState = RobotStorageState::Reading;

  if (!_robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_NurtureGameData,
                                          [this](u8* data, size_t size, NVStorage::NVResult res)
                                          {
                                            FinishReadFromRobot(data, size, res);
                                          }))
  {
    PRINT_NAMED_ERROR("NeedsManager.StartReadFromRobot.ReadFailed", "Failed to start read of needs system robot storage");
    _robotStorageState = RobotStorageState::Inactive;
    return false;
  }

  return true;
}

void NeedsManager::FinishReadFromRobot(u8* data, size_t size, NVStorage::NVResult res)
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Reading, "NeedsManager.FinishReadFromRobot.RobotStorageConflict",
              "Robot storage state should be Reading but instead is %d", _robotStorageState);
  _robotStorageState = RobotStorageState::Inactive;

  if (res < NVStorage::NVResult::NV_OKAY)
  {
    // The tag doesn't exist on the robot indicating the robot is new or has been wiped
    if (res == NVStorage::NVResult::NV_NOT_FOUND)
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.FinishReadFromRobot", "No nurture metagame data on robot");
      StartWriteToRobot(_needsState);
    }
    else
    {
      PRINT_NAMED_ERROR("NeedsManager.FinishReadFromRobot.ReadFailedFinish", "Read failed with %s", EnumToString(res));
    }

    return;
  }

  NeedsStateOnRobot stateOnRobot;
  stateOnRobot.Unpack(data, size);

  int versionLoaded = stateOnRobot.version;
  if (versionLoaded > _needsState.kRobotStorageVersion)
  {
    ANKI_VERIFY(versionLoaded <= _needsState.kRobotStorageVersion, "NeedsManager.FinishReadFromRobot.StateFileVersionIsFuture",
                "Needs state robot storage version read was %d but app thinks latest version is %d",
                versionLoaded, _needsState.kRobotStorageVersion);
    return;
  }

  // TODO Compare timeLastWritten with what we have on the device storage.
  // TODO Compare _robotSerialNumber with what we have on the device storage.

  if (true) // xxx if we want to use the robot's saved data instead of the device's saved data:
  {
    using namespace std::chrono;
    const seconds durationSinceEpoch_s(stateOnRobot.timeLastWritten);
    _needsState._timeLastWritten = time_point<system_clock>(durationSinceEpoch_s);

    _needsState._robotSerialNumber = _robot.GetBodySerialNumber();

    _needsState._curNeedsLevels.clear();
    for (size_t i = 0; i < (size_t)NeedId::Count; i++)
    {
      _needsState._curNeedsLevels[static_cast<NeedId>(i)] = Util::numeric_cast<float>(stateOnRobot.curNeedLevel[i]) / kNeedLevelStorageMultiplier;
    }

    _needsState._partIsDamaged.clear();
    for (size_t i = 0; i < (size_t)RepairablePartId::Count; i++)
    {
      _needsState._partIsDamaged[static_cast<RepairablePartId>(i)] = stateOnRobot.partIsDamaged[i];
    }

    _needsState._curNeedsUnlockLevel = stateOnRobot.curNeedsUnlockLevel;
    _needsState._numStarsAwarded = stateOnRobot.numStarsAwarded;
    _needsState._numStarsForNextUnlock = 3;  // TODO fill in _needsState.numStarsForNextUnlock from config data

    if (versionLoaded < _needsState.kRobotStorageVersion)
    {
      // Sensible defaults go here for new stuff, as needed, when the format changes
      // NOTE:  For this robot storage, changing the format is more difficult because the
      // NeedsStateOnRobot structure has to change, meaning that we'd have to maintain prior
      // versions of it to be able to load earlier data.
    }

    _needsState.UpdateCurNeedsBrackets();
  }
}


} // namespace Cozmo
} // namespace Anki

