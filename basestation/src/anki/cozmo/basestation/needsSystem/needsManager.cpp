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
#include "anki/cozmo/basestation/components/inventoryComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/console/consoleInterface.h"
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
static const std::string kTimeLastStarAwardedKey = "TimeLastStarAwarded";

static const float kNeedLevelStorageMultiplier = 100000.0f;

static const int kMinimumTimeBetweenDeviceSaves_sec = 60;
static const int kMinimumTimeBetweenRobotSaves_sec = (60 * 10);  // Less frequently than device saves


using namespace std::chrono;
using Time = time_point<system_clock>;


#if ANKI_DEV_CHEATS
namespace {
  NeedsManager* g_DebugNeedsManager = nullptr;
  void DebugFillNeedMeters( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      g_DebugNeedsManager->DebugFillNeedMeters();
    }
  }
  // give stars, push back day
  void DebugGiveStar( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      g_DebugNeedsManager->DebugGiveStar();
    }
  }
  void DebugCompleteDay( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      g_DebugNeedsManager->DebugCompleteDay();
    }
  }
  void DebugResetNeeds( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      g_DebugNeedsManager->DebugResetNeeds();
    }
  }
  void DebugCompleteAction( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* actionName = ConsoleArg_Get_String(context, "actionName");
      g_DebugNeedsManager->DebugCompleteAction(actionName);
    }
  }
  void DebugPauseDecayForNeed( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* needName = ConsoleArg_Get_String(context, "needName");
      g_DebugNeedsManager->DebugPauseDecayForNeed(needName);
    }
  }
  void DebugPauseActionsForNeed( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* needName = ConsoleArg_Get_String(context, "needName");
      g_DebugNeedsManager->DebugPauseActionsForNeed(needName);
    }
  }
  void DebugUnpauseDecayForNeed( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* needName = ConsoleArg_Get_String(context, "needName");
      g_DebugNeedsManager->DebugUnpauseDecayForNeed(needName);
    }
  }
  void DebugUnpauseActionsForNeed( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* needName = ConsoleArg_Get_String(context, "needName");
      g_DebugNeedsManager->DebugUnpauseActionsForNeed(needName);
    }
  }
  void DebugSetRepairLevel( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const float level = ConsoleArg_Get_Float(context, "level");
      g_DebugNeedsManager->DebugSetNeedLevel(NeedId::Repair, level);
    }
  }
  void DebugSetEnergyLevel( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const float level = ConsoleArg_Get_Float(context, "level");
      g_DebugNeedsManager->DebugSetNeedLevel(NeedId::Energy, level);
    }
  }
  void DebugSetPlayLevel( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const float level = ConsoleArg_Get_Float(context, "level");
      g_DebugNeedsManager->DebugSetNeedLevel(NeedId::Play, level);
    }
  }
  void DebugPassTimeMinutes( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const float minutes = ConsoleArg_Get_Float(context, "minutes");
      g_DebugNeedsManager->DebugPassTimeMinutes(minutes);
    }
  }
  CONSOLE_FUNC( DebugFillNeedMeters, "Needs" );
  CONSOLE_FUNC( DebugGiveStar, "Needs" );
  CONSOLE_FUNC( DebugCompleteDay, "Needs" );
  CONSOLE_FUNC( DebugResetNeeds, "Needs" );
  CONSOLE_FUNC( DebugCompleteAction, "Needs", const char* actionName );
  CONSOLE_FUNC( DebugPauseDecayForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugPauseActionsForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugUnpauseDecayForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugUnpauseActionsForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugSetRepairLevel, "Needs", float level );
  CONSOLE_FUNC( DebugSetEnergyLevel, "Needs", float level );
  CONSOLE_FUNC( DebugSetPlayLevel, "Needs", float level );
  CONSOLE_FUNC( DebugPassTimeMinutes, "Needs", float minutes );
};
#endif

namespace {
  CONSOLE_VAR(bool, kUseNeedManager, "Needs", true);
};

NeedsManager::NeedsManager(const CozmoContext* cozmoContext)
: _cozmoContext(cozmoContext)
, _robot(nullptr)
, _needsState()
, _needsStateFromRobot()
, _needsConfig()
, _actionsConfig()
, _starRewardsConfig()
, _savedTimeLastWrittenToDevice()
, _timeLastWrittenToRobot()
, _robotHadValidNeedsData(false)
, _deviceHadValidNeedsData(false)
, _robotNeedsVersionUpdate(false)
, _deviceNeedsVersionUpdate(false)
, _isPausedOverall(false)
, _timeWhenPausedOverall_s(0.0f)
, _isDecayPausedForNeed()
, _isActionsPausedForNeed()
, _lastDecayUpdateTime_s()
, _timeWhenPaused_s()
, _queuedNeedDeltas()
, _currentTime_s(0.0f)
, _timeForNextPeriodicDecay_s(0.0f)
, _pausedDurRemainingPeriodicDecay(0.0f)
, _signalHandles()
, kPathToSavedStateFile((cozmoContext->GetDataPlatform() != nullptr ? cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Persistent, GetNurtureFolder()) : ""))
, _robotStorageState(RobotStorageState::Inactive)
{
  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;

    _lastDecayUpdateTime_s[i] = 0.0f;
    _timeWhenPaused_s[i] = 0.0f;

    _queuedNeedDeltas[i].clear();
  }
}

NeedsManager::~NeedsManager()
{
  _signalHandles.clear();
#if ANKI_DEV_CHEATS
  g_DebugNeedsManager = nullptr;
#endif
}


void NeedsManager::Init(const float currentTime_s, const Json::Value& inJson,
                        const Json::Value& inStarsJson, const Json::Value& inActionsJson,
                        const Json::Value& inDecayJson)
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.Init", "Starting Init of NeedsManager");

  _needsConfig.Init(inJson);
  _needsConfig.InitDecay(inDecayJson);
  
  _starRewardsConfig = std::make_shared<StarRewardsConfig>();
  _starRewardsConfig->Init(inStarsJson);

  _actionsConfig.Init(inActionsJson);

  const u32 uninitializedSerialNumber = 0;
  _needsState.Init(_needsConfig, uninitializedSerialNumber, _starRewardsConfig, _cozmoContext->GetRandom());

  _timeForNextPeriodicDecay_s = currentTime_s + _needsConfig._decayPeriod;

  if (!kUseNeedManager)
    return;

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _lastDecayUpdateTime_s[i] = _currentTime_s;

    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;
  }

  if (_cozmoContext->GetExternalInterface() != nullptr)
  {
    auto helper = MakeAnkiEventUtil(*_cozmoContext->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterNeedsActionCompleted>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetGameBeingPaused>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableDroneMode>();
  }

  // Read needs data from device storage, if it exists
  _deviceHadValidNeedsData = false;
  _deviceNeedsVersionUpdate = false;
  bool appliedDecay = false;

  if (DeviceHasNeedsState())
  {
    _deviceHadValidNeedsData = ReadFromDevice(_needsState, _deviceNeedsVersionUpdate);

    if (_deviceHadValidNeedsData)
    {
      // Save the time this save was made, for later comparison in InitAfterReadFromRobotAttempt
      _savedTimeLastWrittenToDevice = _needsState._timeLastWritten;

      // Calculate time elapsed since last connection
      const Time now = system_clock::now();
      const float elasped_s = std::chrono::duration_cast<std::chrono::seconds>(now - _needsState._timeLastWritten).count();

      // Now apply decay according to unconnected config, and elapsed time
      // First, however, we set the timers as if that much time had elapsed:
      for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
      {
        _lastDecayUpdateTime_s[i] -= elasped_s;
      }
      ApplyDecayAllNeeds();

      appliedDecay = true;
    }
  }

  SendNeedsStateToGame(appliedDecay ? NeedsActionId::Decay : NeedsActionId::NoAction);

  // Save to device, because we've either applied a bunch of unconnected decay,
  // or we never had valid data on this device yet
  WriteToDevice(_needsState);

#if ANKI_DEV_CHEATS
  g_DebugNeedsManager = this;
#endif
}


void NeedsManager::InitAfterConnection()
{
  _robot = _cozmoContext->GetRobotManager()->GetFirstRobot();
}


void NeedsManager::InitAfterSerialNumberAcquired(u32 serialNumber)
{
  if (!kUseNeedManager)
    return;

  _needsState._robotSerialNumber = serialNumber;

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterSerialNumberAcquired",
                      "Starting MAIN Init of NeedsManager, with serial number %d", serialNumber);

  // See if the robot has valid needs state, and if so load it
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
  bool needToWriteToDevice = false;
  bool needToWriteToRobot = _robotNeedsVersionUpdate;

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
      if (_savedTimeLastWrittenToDevice < _needsStateFromRobot._timeLastWritten)
      {
        // Robot data is newer; possibly someone controlled this robot with another device
        // Go with the robot data
        needToWriteToDevice = true;
        // Copy the loaded robot needs state into our device needs state
        _needsState = _needsStateFromRobot;
      }
      else if (_savedTimeLastWrittenToDevice > _needsStateFromRobot._timeLastWritten)
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
    else if (!_deviceHadValidNeedsData)
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to device for the first time");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to device");
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
    else if (!_robotHadValidNeedsData)
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to robot for the first time");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "InitAfterReadFromRobotAttempt", "Writing needs data to robot");
    }
    _timeLastWrittenToRobot = now;
    _needsState._timeLastWritten = now;
    StartWriteToRobot(_needsState);
  }
}


void NeedsManager::OnRobotDisconnected()
{
  WriteToDevice(_needsState);

  _robot = nullptr;
}


// This is called whether we are connected to a robot or not
void NeedsManager::Update(const float currentTime_s)
{
  _currentTime_s = currentTime_s;

  if (!kUseNeedManager)
    return;

  if (_isPausedOverall)
    return;

  // Handle periodic decay:
  if (currentTime_s >= _timeForNextPeriodicDecay_s)
  {
    _timeForNextPeriodicDecay_s += _needsConfig._decayPeriod;

    ApplyDecayAllNeeds();

    SendNeedsStateToGame(NeedsActionId::Decay);

    // Note that we don't want to write to robot at this point, as that
    // can take a long time (300 ms) and can interfere with animations.
    // So we generally only write to robot on actions completed.

    // However, it's quick to write to device, so we [possibly] do that here:
    PossiblyWriteToDevice(_needsState);
  }
}


void NeedsManager::SetPaused(const bool paused)
{
  if (!kUseNeedManager)
    return;

  if (paused == _isPausedOverall)
  {
    DEV_ASSERT_MSG(paused != _isPausedOverall, "NeedsManager.SetPaused.Redundant",
                   "Setting paused to %s but already in that state",
                   paused ? "true" : "false");
    return;
  }

  _isPausedOverall = paused;

  if (_isPausedOverall)
  {
    // Calculate and record how much time was left until the next decay
    _pausedDurRemainingPeriodicDecay = _timeForNextPeriodicDecay_s - _currentTime_s;

    _timeWhenPausedOverall_s = _currentTime_s;

    // Send the current needs state to the game as soon as we pause
    //  (because the periodic decay won't happen during pause)
    SendNeedsStateToGame();
  }
  else
  {
    // When unpausing, set the next 'time for periodic decay'
    _timeForNextPeriodicDecay_s = _currentTime_s + _pausedDurRemainingPeriodicDecay;

    // Then calculate how long we were paused
    const float durationOfPause = _currentTime_s - _timeWhenPausedOverall_s;

    // Adjust some timers accordingly, so that the overall pause is excluded from
    // things like decay time, and individual needs pausing
    for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
    {
      _lastDecayUpdateTime_s[i] += durationOfPause;
      _timeWhenPaused_s[i] += durationOfPause;
    }
  }

  SendNeedsPauseStateToGame();
}

  
NeedsState& NeedsManager::GetCurNeedsStateMutable()
{
  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);
  return _needsState;
}


const NeedsState& NeedsManager::GetCurNeedsState()
{
  return GetCurNeedsStateMutable();
}


void NeedsManager::RegisterNeedsActionCompleted(const NeedsActionId actionCompleted)
{
  if (!kUseNeedManager)
    return;

  if (_isPausedOverall)
    return;

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.RegisterNeedsActionCompleted", "Completed %s", NeedsActionIdToString(actionCompleted));
  const int actionIndex = static_cast<int>(actionCompleted);
  const auto& actionDelta = _actionsConfig._actionDeltas[actionIndex];

  switch (actionCompleted)
  {
    case NeedsActionId::RepairHead:
    {
      _needsState._partIsDamaged[RepairablePartId::Head] = false;
      break;
    }
    case NeedsActionId::RepairLift:
    {
      _needsState._partIsDamaged[RepairablePartId::Lift] = false;
      break;
    }
    case NeedsActionId::RepairTreads:
    {
      _needsState._partIsDamaged[RepairablePartId::Treads] = false;
      break;
    }
    default:
      break;
  }

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    if (_isActionsPausedForNeed[i])
    {
      _queuedNeedDeltas[i].push_back(actionDelta._needDeltas[i]);
    }
    else
    {
      _needsState.ApplyDelta(static_cast<NeedId>(i), actionDelta._needDeltas[i]);
    }
  }

  switch (actionCompleted)
  {
    case NeedsActionId::RepairHead:
    case NeedsActionId::RepairLift:
    case NeedsActionId::RepairTreads:
    {
      if (_needsState.NumDamagedParts() == 0)
      {
        // If this was a 'repair' action and there are no more broken parts,
        // set Repair level to 100%
        _needsState._curNeedsLevels[NeedId::Repair] = _needsConfig._maxNeedLevel;
        _needsState.SetNeedsBracketsDirty();
      }
      break;
    }
    default:
      break;
  }

  SendNeedsStateToGame(actionCompleted);

  UpdateStarsState();

  PossiblyWriteToDevice(_needsState);
  PossiblyStartWriteToRobot(_needsState);
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsState& msg)
{
  SendNeedsStateToGame();
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
  if (_isPausedOverall)
  {
    PRINT_CH_DEBUG(kLogChannelName, "NeedsManager.HandleSetNeedsPauseStates",
                  "SetNeedsPauseStates message received but ignored because overall needs manager is paused");
    return;
  }

  // Pause/unpause for decay
  NeedsMultipliers multipliers;
  bool multipliersSet = false;
  bool needToSendNeedsStateToGame = false;

  for (int needIndex = 0; needIndex < _isDecayPausedForNeed.size(); needIndex++)
  {
    if (!_isDecayPausedForNeed[needIndex] && msg.decayPause[needIndex])
    {
      // If pausing this need for decay, record the time we are pausing
      _timeWhenPaused_s[needIndex] = _currentTime_s;
    }
    else if (_isDecayPausedForNeed[needIndex] && !msg.decayPause[needIndex])
    {
      // If un-pausing this need for decay, OPTIONALLY apply queued decay for this need
      if (msg.decayDiscardAfterUnpause[needIndex])
      {
        // Throw away the decay for the period the need was paused
        // But we don't want to throw away (a) the time between the last decay and when
        // the pause started, and (b) the time between now and when the next periodic
        // decay will occur.  So set the 'time of last decay' to account for this:
        // (A key point here is that we want the periodic decay for needs to happen all
        // at the same time.)
        const float durationA_s = _timeWhenPaused_s[needIndex] - _lastDecayUpdateTime_s[needIndex];
        const float durationB_s = _timeForNextPeriodicDecay_s - _currentTime_s;
        _lastDecayUpdateTime_s[needIndex] = _timeForNextPeriodicDecay_s - (durationA_s + durationB_s);
      }
      else
      {
        // Apply the decay for the period the need was paused
        if (!multipliersSet)
        {
          // Set the multipliers only once even if we're applying decay to mulitiple needs at
          // once.  This is to make it "fair", as multipliers are set according to need levels
          multipliersSet = true;
          _needsState.SetDecayMultipliers(_needsConfig._decayConnected, multipliers);
        }
        const float duration_s = _currentTime_s - _lastDecayUpdateTime_s[needIndex];
        _needsState.ApplyDecay(_needsConfig._decayConnected, needIndex, duration_s, multipliers);
        _lastDecayUpdateTime_s[needIndex] = _currentTime_s;
        needToSendNeedsStateToGame = true;
      }
    }

    _isDecayPausedForNeed[needIndex] = msg.decayPause[needIndex];
  }

  // Pause/unpause for actions
  for (int needIndex = 0; needIndex < _isActionsPausedForNeed.size(); needIndex++)
  {
    if (_isActionsPausedForNeed[needIndex] && !msg.actionPause[needIndex])
    {
      // If un-pausing this need for actions, apply all queued actions for this need
      auto& queuedDeltas = _queuedNeedDeltas[needIndex];
      for (int j = 0; j < queuedDeltas.size(); j++)
      {
        _needsState.ApplyDelta(static_cast<NeedId>(needIndex), queuedDeltas[j]);
        needToSendNeedsStateToGame = true;
      }
      queuedDeltas.clear();
    }

    _isActionsPausedForNeed[needIndex] = msg.actionPause[needIndex];
  }

  if (needToSendNeedsStateToGame)
  {
    SendNeedsStateToGame();

    UpdateStarsState();
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
      if (_robot != nullptr)
      {
        _timeLastWrittenToRobot = now;
        StartWriteToRobot(_needsState);
      }
    }
  }
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::EnableDroneMode& msg)
{
  // Pause the needs system during explorer mode
  SetPaused(msg.isStarted);
}


void NeedsManager::SendNeedsStateToGame(const NeedsActionId actionCausingTheUpdate /* = NeedsActionId::NoAction */)
{
  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);
  
  std::vector<float> needLevels;
  needLevels.reserve((size_t)NeedId::Count);
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const float level = _needsState.GetNeedLevelByIndex(i);
    needLevels.push_back(level);
  }
  
  std::vector<NeedBracketId> needBrackets;
  needBrackets.reserve((size_t)NeedBracketId::Count);
  for (size_t i = 0; i < (size_t)NeedBracketId::Count; i++)
  {
    const NeedBracketId bracketId = _needsState.GetNeedBracketByIndex(i);
    needBrackets.push_back(bracketId);
  }
  
  std::vector<bool> partIsDamaged;
  partIsDamaged.reserve((size_t)RepairablePartId::Count);
  for (size_t i = 0; i < (size_t)RepairablePartId::Count; i++)
  {
    const bool isDamaged = _needsState.GetPartIsDamagedByIndex(i);
    partIsDamaged.push_back(isDamaged);
  }

  ExternalInterface::NeedsState message(std::move(needLevels), std::move(needBrackets),
                                        std::move(partIsDamaged), _needsState._curNeedsUnlockLevel,
                                        _needsState._numStarsAwarded, _needsState._numStarsForNextUnlock,
                                        actionCausingTheUpdate);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendNeedsPauseStateToGame()
{
  ExternalInterface::NeedsPauseState message(_isPausedOverall);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
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
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}


void NeedsManager::ApplyDecayAllNeeds()
{
  const DecayConfig& config = _robot == nullptr ? _needsConfig._decayUnconnected : _needsConfig._decayConnected;

  NeedsMultipliers multipliers;
  _needsState.SetDecayMultipliers(config, multipliers);

  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    if (!_isDecayPausedForNeed[needIndex])
    {
      const float duration_s = _currentTime_s - _lastDecayUpdateTime_s[needIndex];
      _needsState.ApplyDecay(config, needIndex, duration_s, multipliers);
      _lastDecayUpdateTime_s[needIndex] = _currentTime_s;
    }
  }
}


void NeedsManager::UpdateStarsState()
{
  // The only weakness of this check is:  If the player gets the star awarded near midnight,
  // and then a few minutes later, after midnight, this function gets called again and the Play
  // level is still within the "full" range, they would be awarded a second star (erroneously).
  // Currently, however, Production is OK with this hole.  --PTerry 2017/06/02

  // If "play stat" is now full, and they haven't gotten a star "today" send the star ready message
  if (_needsState.GetNeedBracketByIndex((size_t)NeedId::Play) == NeedBracketId::Full)
  {
    Time nowTime = std::chrono::system_clock::now();
    // Has it past midnight our time...
    std::time_t lastStarTime = std::chrono::system_clock::to_time_t( _needsState._timeLastStarAwarded );
    std::tm lastLocalTime;
    localtime_r(&lastStarTime, &lastLocalTime);
    std::time_t nowTimeT = std::chrono::system_clock::to_time_t( nowTime );
    std::tm nowLocalTime;
    localtime_r(&nowTimeT, &nowLocalTime);
    
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.UpdateStarsState",
                  "Local time gmt offset %ld",
                  nowLocalTime.tm_gmtoff);
    
    // Past midnight from lasttime
    if (nowLocalTime.tm_yday != lastLocalTime.tm_yday || nowLocalTime.tm_year != lastLocalTime.tm_year)
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.UpdateStarsState",
                    "now: %d, lastsave: %d",
                    nowLocalTime.tm_yday, lastLocalTime.tm_yday);
      
      _needsState._timeLastStarAwarded = nowTime;
      _needsState._numStarsAwarded++;
      // StarUnlocked message
      SendSingleStarAddedToGame();
      
      // Completed a set
      if (_needsState._numStarsAwarded >= _needsState._numStarsForNextUnlock)
      {
        // resets the stars
        SendStarLevelCompletedToGame();
      }

      // Save that we've issued a star today.
      PossiblyStartWriteToRobot(_needsState, true);
    }
  }
}

void NeedsManager::SendStarLevelCompletedToGame()
{
  // Get rewards for current level first
  std::vector<NeedsReward> rewards;
  _starRewardsConfig->GetRewardsForLevel(_needsState._curNeedsUnlockLevel, rewards);
  // level up
  _needsState.SetStarLevel(_needsState._curNeedsUnlockLevel + 1);
  
  ExternalInterface::StarLevelCompleted message(_needsState._curNeedsUnlockLevel,
                                                _needsState._numStarsForNextUnlock,
                                                std::move(rewards));
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  
  // Issue rewards in inventory
  for( int i = 0; i < rewards.size(); ++i )
  {
    switch(rewards[i].rewardType)
    {
      case NeedsRewardType::Unlock:
      {
        UnlockId id = UnlockIdFromString(rewards[i].data);
        _robot->GetProgressionUnlockComponent().SetUnlock(id, true);
        break;
      }
      case NeedsRewardType::Sparks:
      {
        int sparksAdded = std::stoi(rewards[i].data);
        _robot->GetInventoryComponent().AddInventoryAmount(InventoryType::Sparks,sparksAdded);
        break;
      }
      default:
        // TODO: support songs and memories
        break;
    }
  }
  
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.SendStarLevelCompletedToGame","CurrUnlockLevel: %d, stars for next: %d, currStars: %d",
                      _needsState._curNeedsUnlockLevel,_needsState._numStarsForNextUnlock, _needsState._numStarsAwarded);
  
  // Save is forced after this function is called.
}

void NeedsManager::SendSingleStarAddedToGame()
{
  ExternalInterface::StarUnlocked message(_needsState._curNeedsUnlockLevel,
                                          _needsState._numStarsForNextUnlock,
                                          _needsState._numStarsAwarded);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
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
  if (secsSinceLastSave > kMinimumTimeBetweenDeviceSaves_sec)
  {
    needsState._timeLastWritten = now;
    WriteToDevice(needsState);
  }
}

void NeedsManager::WriteToDevice(const NeedsState& needsState)
{
  const auto startTime = std::chrono::system_clock::now();
  Json::Value state;

  state[kStateFileVersionKey] = NeedsState::kDeviceStorageVersion;

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

  const auto timeStarAwarded_s = duration_cast<seconds>(needsState._timeLastStarAwarded.time_since_epoch()).count();
  state[kTimeLastStarAwardedKey] = Util::numeric_cast<Json::LargestInt>(timeStarAwarded_s);

  const auto midTime = std::chrono::system_clock::now();
  if (!_cozmoContext->GetDataPlatform()->writeAsJson(Util::Data::Scope::Persistent, GetNurtureFolder() + kNeedsStateFile, state))
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
  if (!_cozmoContext->GetDataPlatform()->readAsJson(kPathToSavedStateFile + kNeedsStateFile, state))
  {
    PRINT_NAMED_ERROR("NeedsManager.ReadFromDevice.ReadStateFailed", "Failed to read %s", kNeedsStateFile.c_str());
    return false;
  }

  int versionLoaded = state[kStateFileVersionKey].asInt();
  if (versionLoaded > NeedsState::kDeviceStorageVersion)
  {
    ANKI_VERIFY(versionLoaded <= NeedsState::kDeviceStorageVersion, "NeedsManager.ReadFromDevice.StateFileVersionIsFuture",
                "Needs state file version read was %d but app thinks latest version is %d",
                versionLoaded, NeedsState::kDeviceStorageVersion);
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

  if (versionLoaded >= 2)
  {
    const seconds durationSinceEpochLastStar_s(state[kTimeLastStarAwardedKey].asLargestInt());
    needsState._timeLastStarAwarded = time_point<system_clock>(durationSinceEpochLastStar_s);
  }
  else
  {
    needsState._timeLastStarAwarded = Time(); // For older versions, a sensible default
  }

  if (versionLoaded < NeedsState::kDeviceStorageVersion)
  {
    versionUpdated = true;
  }

  _needsState.SetNeedsBracketsDirty();
  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  return true;
}


void NeedsManager::PossiblyStartWriteToRobot(NeedsState& needsState, bool ignoreCooldown)
{
  if (_robotStorageState != RobotStorageState::Inactive)
    return;

  if (_robot == nullptr)
    return;

  const Time now = system_clock::now();
  const auto elapsed = now - _timeLastWrittenToRobot;
  const auto secsSinceLastSave = duration_cast<seconds>(elapsed).count();
  if (ignoreCooldown || secsSinceLastSave > kMinimumTimeBetweenRobotSaves_sec)
  {
    _timeLastWrittenToRobot = now;
    needsState._timeLastWritten = now;
    StartWriteToRobot(needsState);
  }
}

void NeedsManager::StartWriteToRobot(const NeedsState& needsState)
{
  if (_robot == nullptr)
    return;

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

  const auto timeLastStarAwarded_s = duration_cast<seconds>(needsState._timeLastStarAwarded.time_since_epoch()).count();
  const auto timeLastStarAwarded = Util::numeric_cast<uint64_t>(timeLastStarAwarded_s);

  NeedsStateOnRobot stateForRobot(NeedsState::kRobotStorageVersion, timeLastWritten, curNeedLevels,
                                  needsState._curNeedsUnlockLevel, needsState._numStarsAwarded, partIsDamaged, timeLastStarAwarded);

  std::vector<u8> stateVec(stateForRobot.Size());
  stateForRobot.Pack(stateVec.data(), stateForRobot.Size());
  if (!_robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_NurtureGameData, stateVec.data(), stateVec.size(),
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

void NeedsManager::FinishWriteToRobot(const NVStorage::NVResult res, const Time startTime)
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
    // Send a message to the game to indicate write was completed??
    //const auto& extInt = _cozmoContext->GetExternalInterface();
    //extInt->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RequestSetUnlockResult(id, unlocked)));
  }
}


bool NeedsManager::StartReadFromRobot()
{
  ANKI_VERIFY(_robotStorageState == RobotStorageState::Inactive, "NeedsManager.StartReadFromRobot.RobotStorageConflict",
              "Attempting to read from robot but state is %d", _robotStorageState);
  _robotStorageState = RobotStorageState::Reading;

  if (!_robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_NurtureGameData,
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

bool NeedsManager::FinishReadFromRobot(const u8* data, const size_t size, const NVStorage::NVResult res)
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

  // Read first 4 bytes of data as int32_t; this is the save format version
  const int32_t versionLoaded = static_cast<int32_t>(*data);

  if (versionLoaded > NeedsState::kRobotStorageVersion)
  {
    ANKI_VERIFY(versionLoaded <= NeedsState::kRobotStorageVersion, "NeedsManager.FinishReadFromRobot.StateFileVersionIsFuture",
                "Needs state robot storage version read was %d but app thinks latest version is %d",
                versionLoaded, NeedsState::kRobotStorageVersion);
    return false;
  }

  NeedsStateOnRobot stateOnRobot;

  if (versionLoaded == NeedsState::kRobotStorageVersion)
  {
    stateOnRobot.Unpack(data, size);
  }
  else
  {
    // This is an older version of the robot storage, so the data must be
    // migrated to the new format
    _robotNeedsVersionUpdate = true;

    switch (versionLoaded)
    {
      case 1:
      {
        NeedsStateOnRobot_v01 stateOnRobot_v01;
        stateOnRobot_v01.Unpack(data, size);

        stateOnRobot.version = NeedsState::kRobotStorageVersion;
        stateOnRobot.timeLastWritten = stateOnRobot_v01.timeLastWritten;
        for (int i = 0; i < MAX_NEEDS; i++)
          stateOnRobot.curNeedLevel[i] = stateOnRobot_v01.curNeedLevel[i];
        stateOnRobot.curNeedsUnlockLevel = stateOnRobot_v01.curNeedsUnlockLevel;
        stateOnRobot.numStarsAwarded = stateOnRobot_v01.numStarsAwarded;
        for (int i = 0; i < MAX_REPAIRABLE_PARTS; i++)
          stateOnRobot.partIsDamaged[i] = stateOnRobot_v01.partIsDamaged[i];

        // Version 2 added this variable:
        stateOnRobot.timeLastStarAwarded = 0;
        break;
      }
      default:
      {
        PRINT_CH_DEBUG(kLogChannelName, "NeedsManager.FinishReadFromRobot.UnsupportedOldRobotStorageVersion",
                       "Version %d found on robot but not supported", versionLoaded);
        break;
      }
    }
  }

  // Now initialize _needsStateFromRobot from the loaded NeedsStateOnRobot:

  const seconds durationSinceEpoch_s(stateOnRobot.timeLastWritten);
  _needsStateFromRobot._timeLastWritten = time_point<system_clock>(durationSinceEpoch_s);

  _needsStateFromRobot._curNeedsUnlockLevel = stateOnRobot.curNeedsUnlockLevel;
  _needsStateFromRobot._numStarsAwarded = stateOnRobot.numStarsAwarded;
  _needsStateFromRobot._numStarsForNextUnlock = _starRewardsConfig->GetMaxStarsForLevel(stateOnRobot.curNeedsUnlockLevel);

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

  const seconds durationSinceEpochLastStar_s(stateOnRobot.timeLastStarAwarded);
  _needsStateFromRobot._timeLastStarAwarded = time_point<system_clock>(durationSinceEpochLastStar_s);

  // Other initialization for things that do not come from storage:
  _needsStateFromRobot._robotSerialNumber = _robot->GetBodySerialNumber();
  _needsStateFromRobot._needsConfig = &_needsConfig;
  _needsStateFromRobot._starRewardsConfig = _starRewardsConfig;
  _needsStateFromRobot._rng = _cozmoContext->GetRandom();
  _needsStateFromRobot.SetNeedsBracketsDirty();
  _needsStateFromRobot.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  return true;
}

#if ANKI_DEV_CHEATS
void NeedsManager::DebugFillNeedMeters()
{
  _needsState.DebugFillNeedMeters();
  SendNeedsStateToGame();
  UpdateStarsState();
}

void NeedsManager::DebugGiveStar()
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.DebugGiveStar","");
  DebugCompleteDay();
  DebugFillNeedMeters();
}

void NeedsManager::DebugCompleteDay()
{
  // Push the last given star back 24 hours
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t yesterdayTime = std::chrono::system_clock::to_time_t(now - std::chrono::hours(25));
  
  
  std::tm yesterdayTimeLocal;
  localtime_r(&yesterdayTime, &yesterdayTimeLocal);
  std::time_t nowTime = std::chrono::system_clock::to_time_t( now );
  std::tm nowLocalTime;
  localtime_r(&nowTime, &nowLocalTime);
  
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.DebugCompleteDay","");
  _needsState._timeLastStarAwarded = std::chrono::system_clock::from_time_t(yesterdayTime);

}

void NeedsManager::DebugResetNeeds()
{
  if (_robot != nullptr)
  {
    _needsState.Init(_needsConfig, _robot->GetBodySerialNumber(), _starRewardsConfig, _cozmoContext->GetRandom());
    _robotHadValidNeedsData = false;
    _deviceHadValidNeedsData = false;
    InitAfterReadFromRobotAttempt();
  }
}

void NeedsManager::DebugCompleteAction(const char* actionName)
{
  const NeedsActionId actionId = NeedsActionIdFromString(actionName);
  RegisterNeedsActionCompleted(actionId);
}

void NeedsManager::DebugPauseDecayForNeed(const char* needName)
{
  DebugImplPausing(needName, true, true);
}

void NeedsManager::DebugPauseActionsForNeed(const char *needName)
{
  DebugImplPausing(needName, false, true);
}

void NeedsManager::DebugUnpauseDecayForNeed(const char *needName)
{
  DebugImplPausing(needName, true, false);
}

void NeedsManager::DebugUnpauseActionsForNeed(const char *needName)
{
  DebugImplPausing(needName, false, false);
}

void NeedsManager::DebugImplPausing(const char* needName, const bool isDecay, const bool isPaused)
{
  // First, make a copy of all the current pause flags
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

  // Now set or clear the single flag in question
  const NeedId needId = NeedIdFromString(needName);
  const int needIndex = static_cast<int>(needId);
  if (isDecay)
    decayPause[needIndex] = isPaused;
  else
    actionPause[needIndex] = isPaused;

  // Finally, set the flags for whether to discard decay
  // Note:  Just hard coding for now
  std::vector<bool> decayDiscardAfterUnpause;
  const auto numNeeds = static_cast<size_t>(NeedId::Count);
  decayDiscardAfterUnpause.reserve(numNeeds);
  for (int i = 0; i < numNeeds; i++)
  {
    decayDiscardAfterUnpause.push_back(true);
  }

  ExternalInterface::SetNeedsPauseStates m(std::move(decayPause),
                                           std::move(decayDiscardAfterUnpause),
                                           std::move(actionPause));
  HandleMessage(m);
}

void NeedsManager::DebugSetNeedLevel(const NeedId needId, const float level)
{
  const float delta = level - _needsState._curNeedsLevels[needId];
  NeedDelta needDelta(delta, 0.0f);
  _needsState.ApplyDelta(needId, needDelta);

  SendNeedsStateToGame();

  UpdateStarsState();

  PossiblyWriteToDevice(_needsState);
  PossiblyStartWriteToRobot(_needsState);
}

void NeedsManager::DebugPassTimeMinutes(const float minutes)
{
  const float timeElaspsed_s = minutes * 60.0f;
  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    if (!_isDecayPausedForNeed[needIndex])
    {
      _lastDecayUpdateTime_s[needIndex] -= timeElaspsed_s;
    }
  }

  ApplyDecayAllNeeds();

  SendNeedsStateToGame(NeedsActionId::Decay);

  WriteToDevice(_needsState);
}
#endif

} // namespace Cozmo
} // namespace Anki

