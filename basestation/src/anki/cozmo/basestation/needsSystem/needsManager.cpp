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

static const int kMinimumTimeBetweenPeriodicDeviceSaves_sec = 20; // 20 for testing; probably make this 60
static const int kMinimumTimeBetweenPeriodicRobotSaves_sec = (60 * 1);  // 60 secs for testing; probably make this 10 minutes


using namespace std::chrono;
using Time = time_point<system_clock>;



NeedsManager::NeedsManager(Robot& robot)
: _robot(robot)
, _needsState()
, _needsConfig()
, _needsStateFromRobot()
, _timeLastWrittenToRobot()
, _robotHadValidNeedsData(false)
, _deviceHadValidNeedsData(false)
, _robotNeedsVersionUpdate(false)
, _deviceNeedsVersionUpdate(false)
, _isPaused(false)
, _isDecayPausedForNeed()
, _isActionsPausedForNeed()
, _currentTime_s(0.0f)
, _timeForNextPeriodicDecay_s(0.0f)
, _pausedDurRemainingPeriodicDecay(0.0f)
, kPathToSavedStateFile((_robot.GetContextDataPlatform() != nullptr ? _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, GetNurtureFolder()) : ""))
, _robotStorageState(RobotStorageState::Inactive)
{
  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;
  }
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

  // We want to pause the whole system until we've finished reading from robot
  _isPaused = true;

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;
  }

  // Subscribe to messages
  if (_robot.HasExternalInterface())
  {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterNeedsActionCompleted>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetGameBeingPaused>();
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

  // First, see if the device has valid stored state, and if so load it
  _deviceHadValidNeedsData = false;
  _deviceNeedsVersionUpdate = false;
  if (DeviceHasNeedsState())
  {
    _deviceHadValidNeedsData = ReadFromDevice(_needsState, _deviceNeedsVersionUpdate);
  }

  // Second, see if the robot has valid stored state, and if so load it
  _robotHadValidNeedsData = false;
  _robotNeedsVersionUpdate = false;
  if (!StartReadFromRobot())
  {
    // If the read from robot fails immediately, move on to post-robot-read init
    InitAfterReadFromRobotAttempt();
  }
}


void NeedsManager::InitAfterReadFromRobotAttempt()
{
  // First, we can finally un-pause the system now that we've read data from the robot
  _isPaused = false;

  bool needToWriteToDevice = false;
  bool needToWriteToRobot = false;

  if (!_robotHadValidNeedsData && !_deviceHadValidNeedsData)
  {
    // Neither robot nor device has needs data
    needToWriteToDevice = true;
    needToWriteToRobot = true;
  }
  else if (_robotHadValidNeedsData && !_deviceHadValidNeedsData)
  {
    // Robot has needs data, but device doesn't
    // (Use case:  Robot has been used with another device)
    needToWriteToDevice = true;
    // Copy the loaded robot needs state into our device needs state
    _needsState = _needsStateFromRobot;
  }
  else if (!_robotHadValidNeedsData && _deviceHadValidNeedsData)
  {
    // Robot does NOT have needs data, but device does
    // So just go with device data, and write that to robot
    needToWriteToRobot = true;
  }
  else
  {
    // Both robot and device have needs data
    if (_needsState._robotSerialNumber == _needsStateFromRobot._robotSerialNumber)
    {
      // This was the same robot the device had been connected to before.
      if (_needsState._timeLastWritten < _needsStateFromRobot._timeLastWritten)
      {
        // Robot data is newer; possibly someone controlled this robot with another device
        // Go with the robot data
        needToWriteToDevice = true;
        // Copy the loaded robot needs state into our device needs state
        _needsState = _needsStateFromRobot;
      }
      else if (_needsState._timeLastWritten > _needsStateFromRobot._timeLastWritten)
      {
        // Device data is newer; something weird happened
        // Go with the device data
        // TODO:  Add DAS event for analytics purposes
        needToWriteToRobot = true;
      }
      // (else the times are identical, which is the normal case...nothing to do)
    }
    else
    {
      // User has connected to a different robot that has used the needs feature.
      // Use the robot's state; copy it to the device.
      needToWriteToDevice = true;
      // Copy the loaded robot needs state into our device needs state
      _needsState = _needsStateFromRobot;
    }
  }

  const Time now = system_clock::now();

  if (needToWriteToDevice)
  {
    if (_deviceNeedsVersionUpdate)
    {
      _deviceNeedsVersionUpdate = false;
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to device due to storage version update");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to device for the first time");
    }
    _needsState._timeLastWritten = now;
    WriteToDevice(_needsState);
  }

  if (needToWriteToRobot)
  {
    if (_robotNeedsVersionUpdate)
    {
      _robotNeedsVersionUpdate = false;
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to robot due to storage version update");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to robot for the first time");
    }
    _timeLastWrittenToRobot = now;
    _needsState._timeLastWritten = now;
    StartWriteToRobot(_needsState);
  }
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
    _timeForNextPeriodicDecay_s = currentTime_s + _needsConfig._decayPeriod;
  }

  if (currentTime_s >= _timeForNextPeriodicDecay_s)
  {
    _timeForNextPeriodicDecay_s += _needsConfig._decayPeriod;

    _needsState.ApplyDecay(_needsConfig._decayPeriod, _needsConfig._decayConnected);

    SendNeedsStateToGame(NeedsActionId::Decay);

    // Note that we don't want to write to robot at this point, as that
    // can take a long time (300 ms) and can interfere with animations.
    // So we generally only write to robot on actions completed.
    // However, it's quick to write to device, so we [possibly] do that here:
    PossiblyWriteToDevice(_needsState);
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
  if (_isPaused)
    return;

  // todo implement this
  
  // todo: Adjust zero or more needs levels based on config data for the completed action

  // todo: Clamp any changed need level with _needsConfig._minNeedLevel and _maxNeedLevel
  
  SendNeedsStateToGame(actionCompleted);

  PossiblyWriteToDevice(_needsState);
  PossiblyStartWriteToRobot(_needsState);
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
void NeedsManager::HandleMessage(const ExternalInterface::SetNeedsPauseStates& msg)
{
  // TODO:  For each flag, detect when the flag goes from true to false
  // (meaning becoming un-paused), and for decay, apply delayed decay, and
  // for actions, apply decayed actions (for the given need).
  for (int i = 0; i < _isDecayPausedForNeed.size(); i++)
  {
    _isDecayPausedForNeed[i] = msg.decayPause[i];
  }

  for (int i = 0; i < _isActionsPausedForNeed.size(); i++)
  {
    _isActionsPausedForNeed[i] = msg.actionPause[i];
  }
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsPauseStates& msg)
{
  SendNeedsPauseStatesToGame();
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::RegisterNeedsActionCompleted& msg)
{
  RegisterNeedsActionCompleted(msg.actionCompleted);
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::SetGameBeingPaused& msg)
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.HandleSetGameBeingPaused",
                "Game being paused set to %s",
                msg.isPaused ? "TRUE" : "FALSE");

  // When app is backgrounded, we want to also pause the whole needs system
  SetPaused(msg.isPaused);

  if (msg.isPaused)
  {
    const Time now = system_clock::now();
    _needsState._timeLastWritten = now;
    WriteToDevice(_needsState);

    if (_robotStorageState == RobotStorageState::Inactive)
    {
      const Time now = system_clock::now();
      _timeLastWrittenToRobot = now;
      _needsState._timeLastWritten = now;
      StartWriteToRobot(_needsState);
    }
  }
}


void NeedsManager::SendNeedsStateToGame(NeedsActionId actionCausingTheUpdate)
{
  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);
  
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

void NeedsManager::SendNeedsPauseStatesToGame()
{
  std::vector<bool> decayPause;
  decayPause.reserve(_isDecayPausedForNeed.size());
  for (int i = 0; i < _isDecayPausedForNeed.size(); i++)
  {
    decayPause.push_back(_isDecayPausedForNeed[i]);
  }

  std::vector<bool> actionPause;
  actionPause.reserve(_isActionsPausedForNeed.size());
  for (int i = 0; i < _isActionsPausedForNeed.size(); i++)
  {
    actionPause.push_back(_isActionsPausedForNeed[i]);
  }

  ExternalInterface::NeedsPauseStates message(decayPause, actionPause);
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

void NeedsManager::PossiblyWriteToDevice(NeedsState& needsState)
{
  const Time now = system_clock::now();
  const auto elapsed = now - needsState._timeLastWritten;
  const auto secsSinceLastSave = duration_cast<seconds>(elapsed).count();
  if (secsSinceLastSave > kMinimumTimeBetweenPeriodicDeviceSaves_sec)
  {
    needsState._timeLastWritten = now;
    WriteToDevice(needsState);
  }
}

void NeedsManager::WriteToDevice(const NeedsState& needsState)
{
  const auto startTime = std::chrono::system_clock::now();
  Json::Value state;

  state[kStateFileVersionKey] = needsState.kDeviceStorageVersion;

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

  const auto midTime = std::chrono::system_clock::now();
  if (!_robot.GetContextDataPlatform()->writeAsJson(Util::Data::Scope::Persistent, GetNurtureFolder() + kNeedsStateFile, state))
  {
    PRINT_NAMED_ERROR("NeedsManager.WriteToDevice.WriteStateFailed", "Failed to write needs state file");
  }
  const auto endTime = std::chrono::system_clock::now();
  const auto microsecsMid =std::chrono::duration_cast<std::chrono::microseconds>(endTime - midTime);
  const auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.WriteToDevice",
                "Write to device took %lld microseconds total; %lld microseconds for the actual write",
                microsecs.count(), microsecsMid.count());
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

  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  return true;
}


void NeedsManager::PossiblyStartWriteToRobot(NeedsState& needsState)
{
  if (_robotStorageState != RobotStorageState::Inactive)
    return;

  const Time now = system_clock::now();
  const auto elapsed = now - _timeLastWrittenToRobot;
  const auto secsSinceLastSave = duration_cast<seconds>(elapsed).count();
  if (secsSinceLastSave > kMinimumTimeBetweenPeriodicRobotSaves_sec)
  {
    _timeLastWrittenToRobot = now;
    needsState._timeLastWritten = now;
    StartWriteToRobot(needsState);
  }
}

void NeedsManager::StartWriteToRobot(const NeedsState& needsState)
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Inactive, "NeedsManager.StartWriteToRobot.RobotStorageConflict",
              "Attempting to write to robot but state is %d", _robotStorageState);

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Writing to robot...");
  const auto startTime = std::chrono::system_clock::now();

  _robotStorageState = RobotStorageState::Writing;

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
                                           [this, startTime](NVStorage::NVResult res)
                                           {
                                             FinishWriteToRobot(res, startTime);
                                           }))
  {
    PRINT_NAMED_ERROR("NeedsState.StartWriteToRobot.WriteFailed", "Write failed");
    _robotStorageState = RobotStorageState::Inactive;
  }
  auto endTime = std::chrono::system_clock::now();
  auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Write to robot START took %lld microseconds", microsecs.count());
}

void NeedsManager::FinishWriteToRobot(NVStorage::NVResult res, const Time startTime)
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Writing, "NeedsManager.FinishWriteToRobot.RobotStorageConflict",
              "Robot storage state should be Writing but instead is %d", _robotStorageState);
  _robotStorageState = RobotStorageState::Inactive;

  auto endTime = std::chrono::system_clock::now();
  auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.FinishWriteToRobot", "Write to robot AFTER CALLBACK took %lld microseconds", microsecs.count());

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
                                            _robotHadValidNeedsData = FinishReadFromRobot(data, size, res);

                                            InitAfterReadFromRobotAttempt();
                                          }))
  {
    PRINT_NAMED_ERROR("NeedsManager.StartReadFromRobot.ReadFailed", "Failed to start read of needs system robot storage");
    _robotStorageState = RobotStorageState::Inactive;
    return false;
  }

  return true;
}

bool NeedsManager::FinishReadFromRobot(u8* data, size_t size, NVStorage::NVResult res)
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
    }
    else
    {
      PRINT_NAMED_ERROR("NeedsManager.FinishReadFromRobot.ReadFailedFinish", "Read failed with %s", EnumToString(res));
    }

    return false;
  }

  NeedsStateOnRobot stateOnRobot;
  stateOnRobot.Unpack(data, size);

  int versionLoaded = stateOnRobot.version;
  if (versionLoaded > _needsStateFromRobot.kRobotStorageVersion)
  {
    ANKI_VERIFY(versionLoaded <= _needsStateFromRobot.kRobotStorageVersion, "NeedsManager.FinishReadFromRobot.StateFileVersionIsFuture",
                "Needs state robot storage version read was %d but app thinks latest version is %d",
                versionLoaded, _needsStateFromRobot.kRobotStorageVersion);
    return false;
  }

  // Now initialize _needsStateFromRobot from the loaded NeedsStateOnRobot:

  const seconds durationSinceEpoch_s(stateOnRobot.timeLastWritten);
  _needsStateFromRobot._timeLastWritten = time_point<system_clock>(durationSinceEpoch_s);

  _needsStateFromRobot._robotSerialNumber = _robot.GetBodySerialNumber();

  _needsStateFromRobot._curNeedsUnlockLevel = stateOnRobot.curNeedsUnlockLevel;
  _needsStateFromRobot._numStarsAwarded = stateOnRobot.numStarsAwarded;
  _needsStateFromRobot._numStarsForNextUnlock = 3;  // todo fill in from config data and curNeedsUnlockLevel

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    const auto& needId = static_cast<NeedId>(i);
    _needsStateFromRobot._curNeedsLevels[needId] = Util::numeric_cast<float>(stateOnRobot.curNeedLevel[i]) / kNeedLevelStorageMultiplier;
  }

  for (int i = 0; i < static_cast<int>(RepairablePartId::Count); i++)
  {
    const auto& pardId = static_cast<RepairablePartId>(i);
    _needsStateFromRobot._partIsDamaged[pardId] = stateOnRobot.partIsDamaged[i];
  }

  _needsStateFromRobot._needsConfig = &_needsConfig;

  if (versionLoaded < _needsStateFromRobot.kRobotStorageVersion)
  {
    _robotNeedsVersionUpdate = true;
    // Sensible defaults go here for new stuff, as needed, when the format changes
  }

  _needsStateFromRobot.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  return true;
}



} // namespace Cozmo
} // namespace Anki

