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


#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/types.h"
#include "engine/ankiEventUtil.h"
#include "engine/components/desiredFaceDistortionComponent.h"
#include "engine/components/inventoryComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsConfig.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/viz/vizManager.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/ankiLab/extLabInterface.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

const char* NeedsManager::kLogChannelName = "NeedsSystem";


static const std::string kNeedsStateFileBase = "needsState";
static const std::string kNeedsStateFile = kNeedsStateFileBase + ".json";

static const std::string kStateFileVersionKey = "_StateFileVersion";
static const std::string kDateTimeKey = "_DateTime";
static const std::string kSerialNumberKey = "_SerialNumber";

static const std::string kCurNeedLevelKey = "CurNeedLevel";
static const std::string kPartIsDamagedKey = "PartIsDamaged";
static const std::string kCurNeedsUnlockLevelKey = "CurNeedsUnlockLevel";
static const std::string kNumStarsAwardedKey = "NumStarsAwarded";
static const std::string kNumStarsForNextUnlockKey = "NumStarsForNextUnlock";
static const std::string kTimeCreatedKey = "TimeCreated";
static const std::string kTimeLastStarAwardedKey = "TimeLastStarAwarded";
static const std::string kTimeLastDisconnectKey = "TimeLastDisconnect";
static const std::string kTimeLastAppBackgroundedKey = "TimeLastAppBackgrounded";
static const std::string kOpenAppAfterDisconnectKey = "OpenAppAfterDisconnect";
static const std::string kForceNextSongKey = "ForceNextSong";

// This key is used as the base of the string for when sparks are sometimes rewarded
// for freeplay activities, that tells how many sparks were awarded and what Cozmo
// was doing (we append the NeedsActionId enum string to the end of this string)
static const std::string kFreeplaySparksRewardStringKey = "needs.FreeplaySparksReward";

static const float kNeedLevelStorageMultiplier = 100000.0f;

static const int kMinimumTimeBetweenDeviceSaves_sec = 60;
static const int kMinimumTimeBetweenRobotSaves_sec = (60 * 10);  // Less frequently than device saves

static const std::string kUnconnectedDecayRatesExperimentKey = "unconnected_decay_rates";

// Note:  We don't use zero for 'uninitialized serial number' because
// running in webots gives us zero as the robot serial number
static const u32 kUninitializedSerialNumber = UINT_MAX;


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
  void DebugPredictActionResult( ConsoleFunctionContextRef context )
  {
    if( g_DebugNeedsManager != nullptr)
    {
      const char* actionName = ConsoleArg_Get_String(context, "actionName");
      g_DebugNeedsManager->DebugPredictActionResult(actionName);
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
  void ForceDecayVariation( ConsoleFunctionContextRef context )
  {
    const char* experimentKey = kUnconnectedDecayRatesExperimentKey.c_str();
    const char* variationKey = ConsoleArg_Get_String(context, "variationKey");
    Util::AnkiLab::AddABTestingForcedAssignment(experimentKey, variationKey);
    context->channel->WriteLog("ForceAssignExperiment %s => %s", experimentKey, variationKey);

    g_DebugNeedsManager->GetNeedsConfigMutable().SetUnconnectedDecayTestVariation(g_DebugNeedsManager->GetDecayConfigBaseFilename(),
                                                                                  variationKey,
                                                                                  Util::AnkiLab::AssignmentStatus::ForceAssigned);
  }
  CONSOLE_FUNC( DebugFillNeedMeters, "Needs" );
  CONSOLE_FUNC( DebugGiveStar, "Needs" );
  CONSOLE_FUNC( DebugCompleteDay, "Needs" );
  CONSOLE_FUNC( DebugResetNeeds, "Needs" );
  CONSOLE_FUNC( DebugCompleteAction, "Needs", const char* actionName );
  CONSOLE_FUNC( DebugPredictActionResult, "Needs", const char* actionName );
  CONSOLE_FUNC( DebugPauseDecayForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugPauseActionsForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugUnpauseDecayForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugUnpauseActionsForNeed, "Needs", const char* needName );
  CONSOLE_FUNC( DebugSetRepairLevel, "Needs", float level );
  CONSOLE_FUNC( DebugSetEnergyLevel, "Needs", float level );
  CONSOLE_FUNC( DebugSetPlayLevel, "Needs", float level );
  CONSOLE_FUNC( DebugPassTimeMinutes, "Needs", float minutes );
  CONSOLE_FUNC( ForceDecayVariation, "A/B Testing", const char* variationKey );
};
#endif

NeedsManager::NeedsManager(const CozmoContext* cozmoContext)
: _cozmoContext(cozmoContext)
, _robot(nullptr)
, _needsState()
, _needsStateFromRobot()
, _needsConfig(cozmoContext)
, _actionsConfig()
, _starRewardsConfig()
, _localNotifications(new LocalNotifications(cozmoContext, *this))
, _savedTimeLastWrittenToDevice()
, _timeLastWrittenToRobot()
, _robotHadValidNeedsData(false)
, _deviceHadValidNeedsData(false)
, _robotNeedsVersionUpdate(false)
, _deviceNeedsVersionUpdate(false)
, _previousRobotSerialNumber(0)
, _robotOnboardingStageCompleted(0)
, _connectionOccurredThisAppRun(false)
, _isPausedOverall(false)
, _timeWhenPausedOverall_s(0.0f)
, _isDecayPausedForNeed()
, _isActionsPausedForNeed()
, _lastDecayUpdateTime_s()
, _timeWhenPaused_s()
, _timeWhenCooldownStarted_s()
, _timeWhenCooldownOver_s()
, _timeWhenBracketChanged_s()
, _queuedNeedDeltas()
, _actionCooldown_s()
, _onlyWhiteListedActionsEnabled(false)
, _currentTime_s(0.0f)
, _timeForNextPeriodicDecay_s(0.0f)
, _pausedDurRemainingPeriodicDecay(0.0f)
, _signalHandles()
, kPathToSavedStateFile((cozmoContext->GetDataPlatform() != nullptr ? cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Persistent, GetNurtureFolder()) : ""))
, _pendingReadFromRobot(false)
, _faceDistortionComponent(new DesiredFaceDistortionComponent(*this))
, _pendingSparksRewardMsg(false)
, _sparksRewardMsg()
{
  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;

    _lastDecayUpdateTime_s[i] = 0.0f;
    _timeWhenPaused_s[i] = 0.0f;
    _timeWhenCooldownStarted_s[i] = 0.0f;
    _timeWhenCooldownOver_s[i] = 0.0f;
    _timeWhenBracketChanged_s[i] = 0.0f;

    _queuedNeedDeltas[i].clear();
  }

  for (int i = 0; i < static_cast<int>(NeedsActionId::Count); i++)
  {
    _actionCooldown_s[i] = 0.0f;
  }
}

NeedsManager::~NeedsManager()
{
  _signalHandles.clear();
#if ANKI_DEV_CHEATS
  g_DebugNeedsManager = nullptr;
#endif
}


void NeedsManager::Init(const float currentTime_s,      const Json::Value& inJson,
                        const Json::Value& inStarsJson, const Json::Value& inActionsJson,
                        const Json::Value& inDecayJson, const Json::Value& inHandlersJson,
                        const Json::Value& inLocalNotificationJson)
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.Init", "Starting Init of NeedsManager");

  _needsConfig.Init(inJson);
  _needsConfig.InitDecay(inDecayJson);
  
  _starRewardsConfig = std::make_shared<StarRewardsConfig>();
  _starRewardsConfig->Init(inStarsJson);

  _actionsConfig.Init(inActionsJson);

  if( ANKI_VERIFY(_cozmoContext->GetRandom() != nullptr,
                  "NeedsManager.Init.NoRNG",
                  "Can't create needs handler for face glitches because there is no RNG in cozmo context") ) {
    _faceDistortionComponent->Init(inHandlersJson, _cozmoContext->GetRandom());
  }

  _localNotifications->Init(inLocalNotificationJson, _cozmoContext->GetRandom(), currentTime_s);

  _connectionOccurredThisAppRun = false;

  if (_cozmoContext->GetExternalInterface() != nullptr)
  {
    auto helper = MakeAnkiEventUtil(*_cozmoContext->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ForceSetDamagedParts>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ForceSetNeedsLevels>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetNeedsState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetSongsList>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetWantsNeedsOnboarding>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterNeedsActionCompleted>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RegisterOnboardingComplete>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetGameBeingPaused>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsActionWhitelist>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetNeedsPauseStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::WipeDeviceNeedsData>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::WipeRobotGameData>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::WipeRobotNeedsData>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::GetStarStatus>();
  }

  InitInternal(currentTime_s);
}


void NeedsManager::InitReset(const float currentTime_s, const u32 serialNumber)
{
  _needsState.Init(_needsConfig, serialNumber, _starRewardsConfig, _cozmoContext->GetRandom());

  _timeForNextPeriodicDecay_s = currentTime_s + _needsConfig._decayPeriod;

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _lastDecayUpdateTime_s[i] = _currentTime_s;
    _timeWhenCooldownStarted_s[i] = 0.0f;
    _timeWhenCooldownOver_s[i] = 0.0f;
    _timeWhenBracketChanged_s[i] = _currentTime_s;

    _isDecayPausedForNeed[i] = false;
    _isActionsPausedForNeed[i] = false;
  }

  for (int i = 0; i < static_cast<int>(NeedsActionId::Count); i++)
  {
    _actionCooldown_s[i] = 0.0f;
  }
}


void NeedsManager::InitInternal(const float currentTime_s)
{
  InitReset(currentTime_s, kUninitializedSerialNumber);

  // Read needs data from device storage, if it exists
  _deviceHadValidNeedsData = false;
  _deviceNeedsVersionUpdate = false;

  _deviceHadValidNeedsData = AttemptReadFromDevice(kNeedsStateFile, _deviceNeedsVersionUpdate);

  if (!_deviceHadValidNeedsData)
  {
    // If there was no device save, we still want to send needs state to show the initial state
    SendNeedsStateToGame(NeedsActionId::NoAction);
  }

  // Save to device, because we've either applied a bunch of unconnected decay,
  // or we never had valid needs data on this device yet
  WriteToDevice();

  SendNeedsLevelsDasEvent("app_start");

  // Generate all appropriate notifications now (see function comments). Note that on
  // app startup, this is actually too early (because the Game side is not quite ready
  // to accept the messages that Generate sends).  But we keep this here because this
  // function (InitInternal) is also called when wiping the device or robot save.
  _localNotifications->Generate();

#if ANKI_DEV_CHEATS
  g_DebugNeedsManager = this;
#endif
}


void NeedsManager::InitAfterConnection()
{
  _robot = _cozmoContext->GetRobotManager()->GetFirstRobot();

  _pendingReadFromRobot = true;

  _connectionOccurredThisAppRun = true;
}


void NeedsManager::InitAfterSerialNumberAcquired(u32 serialNumber)
{
  _previousRobotSerialNumber = _needsState._robotSerialNumber;
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
  // By this time we've finished reading the stored lab assignments from the
  // robot, and immediately after that, the needs state.  So now we can
  // attempt to activate the experiment
  AttemptActivateDecayExperiment(_needsState._robotSerialNumber);

  bool needToWriteToDevice = false;
  bool needToWriteToRobot = _robotNeedsVersionUpdate;

  // DAS Event: "needs.resolve_on_connection"
  // s_val: Whether device had valid needs data (1 or 0), and whether robot
  //        had valid needs data, separated by a colon
  // data: Serial number extracted from device storage, and serial number on
  //       robot, separated by colon
  std::ostringstream stream;
  stream << (_deviceHadValidNeedsData ? "1" : "0") << ":" << (_robotHadValidNeedsData ? "1" : "0");
  const std::string serialNumbers = std::to_string(_previousRobotSerialNumber) + ":" +
                                    std::to_string(_needsStateFromRobot._robotSerialNumber);
  Anki::Util::sEvent("needs.resolve_on_connection",
                     {{DDATA, serialNumbers.c_str()}},
                     stream.str().c_str());

  bool useStateFromRobot = false;

  if (!_robotHadValidNeedsData && !_deviceHadValidNeedsData)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Neither robot nor device has needs data");
    // Neither robot nor device has needs data
    needToWriteToDevice = true;
    needToWriteToRobot = true;
  }
  else if (_robotHadValidNeedsData && !_deviceHadValidNeedsData)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Robot has needs data, but device doesn't");
    // Robot has needs data, but device doesn't
    // (Use case:  Robot has been used with another device)
    needToWriteToDevice = true;
    useStateFromRobot = true;
    // Reset the needs state's 'time of last disconnect', since that doesn't apply
    // any more (and it's not stored on robot); look below to see where we preserve
    // this field when overriding needs state with needs state from robot
    _needsState._timeLastDisconnect = Time();
  }
  else if (!_robotHadValidNeedsData && _deviceHadValidNeedsData)
  {
    // Robot does NOT have needs data, but device does
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt",
                  "Robot does NOT have needs data, but device does");

    // Compare the device save's robot serial number with the actual robot
    // serial number; if different, we look for another device save that has
    // the actual serial number in the file name (a file we created when
    // backgrounding the app).
    if (_previousRobotSerialNumber != _needsState._robotSerialNumber)
    {
      const std::string filename = NeedsFilenameFromSerialNumber(_needsState._robotSerialNumber);

      if (AttemptReadFromDevice(filename, _deviceNeedsVersionUpdate))
      {
        needToWriteToDevice = true;
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt",
                      "Attempted to read alternate save file %s; file missing or read failed; could be because of brand new robot", filename.c_str());
      }
    }

    // Either way, we definitely want to write to robot
    needToWriteToRobot = true;
  }
  else
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt",
                  "Both robot and device have needs data...Serial numbers %x and %x",
                  _previousRobotSerialNumber, _needsStateFromRobot._robotSerialNumber);

    // Both robot and device have needs data
    if (_previousRobotSerialNumber == _needsStateFromRobot._robotSerialNumber)
    {
      // DAS Event: "needs.resolve_on_connection_matched"
      // s_val: 0 if timestamps matched; -1 if device storage was newer; 1 if
      //        robot storage was newer
      // data: Unused
      std::string comparison = (_savedTimeLastWrittenToDevice < _needsStateFromRobot._timeLastWritten ? "1" :
                               (_savedTimeLastWrittenToDevice > _needsStateFromRobot._timeLastWritten ? "-1" : "0"));
      Anki::Util::sEvent("needs.resolve_on_connection_matched", {}, comparison.c_str());

      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "...and serial numbers MATCH");
      // This was the same robot the device had been connected to before
      if (_savedTimeLastWrittenToDevice < _needsStateFromRobot._timeLastWritten)
      {
        PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Robot data is newer");
        // Robot data is newer; possibly someone controlled this robot with another device
        // Go with the robot data
        needToWriteToDevice = true;
        useStateFromRobot = true;
      }
      else if (_savedTimeLastWrittenToDevice > _needsStateFromRobot._timeLastWritten)
      {
        PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Device data is newer");
        // Device data is newer; go with the device data
        needToWriteToRobot = true;
      }
      else
      {
        // (else the times are identical, which is the normal case...nothing to do)
        PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Timestamps are IDENTICAL");
      }
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "...and serial numbers DON'T match");
      // User has connected to a different robot that has used the needs feature.
      // Use the robot's state; copy it to the device.
      needToWriteToDevice = true;
      useStateFromRobot = true;
      // Reset the needs state's 'time of last disconnect', since that doesn't apply
      // any more (and it's not stored on robot); look below to see where we preserve
      // this field when overriding needs state with needs state from robot
      _needsState._timeLastDisconnect = Time();
      // Similar for time since last app backgrounded; let's not confuse things
      _needsState._timeLastAppBackgrounded = Time();
    }
  }

  if (useStateFromRobot)
  {
    const Time savedTimeLastDisconnect = _needsState._timeLastDisconnect;
    const Time savedTimeLastAppBackgrounded = _needsState._timeLastAppBackgrounded;

    // Copy the loaded robot needs state into our device needs state
    _needsState = _needsStateFromRobot;

    _needsState._timeLastDisconnect = savedTimeLastDisconnect;
    _needsState._timeLastAppBackgrounded = savedTimeLastAppBackgrounded;

    // Now apply decay for the unconnected time for THIS robot
    // (We did it earlier, in Init, but that was for a different robot)
    static const bool connected = false;
    ApplyDecayForTimeSinceLastDeviceWrite(connected);

    SendNeedsStateToGame(NeedsActionId::Decay);
  }

  // Update Game on Robot's last state
  SendNeedsOnboardingToGame();

  if (_needsState._timeLastDisconnect != Time()) // (don't send if we've never disconnected)
  {
    // DAS Event: "needs.disconnect_time"
    // s_val: The number of seconds since the last disconnection
    // data: Unused
    const Time now = system_clock::now();
    const auto elapsed = now - _needsState._timeLastDisconnect;
    const auto secsSinceLastDisconnect = duration_cast<seconds>(elapsed).count();
    Anki::Util::sEvent("needs.disconnect_time", {},
                       std::to_string(secsSinceLastDisconnect).c_str());
  }

  const Time now = system_clock::now();

  if (needToWriteToDevice)
  {
    if (_deviceNeedsVersionUpdate)
    {
      _deviceNeedsVersionUpdate = false;
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to device due to storage version update");
    }
    else if (!_deviceHadValidNeedsData)
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to device for the first time");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to device");
    }
    // Instead of having WriteToDevice do the time-stamping, we do it externally here
    // so that we can use the exact same timestamp in StartWriteToRobot below
    _needsState._timeLastWritten = now;
    WriteToDevice(false);

    // Set this flag to true because we now have valid needs data on the device,
    // and we're done using the flag.  If we later disconnect and then reconnect
    // (in the same app run), we want this flag to be accurate
    _deviceHadValidNeedsData = true;
  }

  if (needToWriteToRobot)
  {
    if (_robotNeedsVersionUpdate)
    {
      _robotNeedsVersionUpdate = false;
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to robot due to storage version update");
    }
    else if (!_robotHadValidNeedsData)
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to robot for the first time");
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitAfterReadFromRobotAttempt", "Writing needs data to robot");
    }
    StartWriteToRobot(now);
  }

  _localNotifications->Generate();
}


void NeedsManager::AttemptActivateDecayExperiment(u32 robotSerialNumber)
{
  // If we don't yet have a valid robot serial number, don't try to make
  // an experiment assignment, since it's based on robot serial number
  if (robotSerialNumber == kUninitializedSerialNumber)
  {
    return;
  }

  // Call this first to set the audience tags based on current data.
  // Note that in ActivateExperiment, if there was already an assigned
  // variation, we'll use that assignment and ignore the audience tags
  _cozmoContext->GetExperiments()->GetAudienceTags().CalculateQualifiedTags();

  Util::AnkiLab::ActivateExperimentRequest request(kUnconnectedDecayRatesExperimentKey,
                                                   std::to_string(robotSerialNumber));
  std::string outVariationKey;
  const auto assignmentStatus = _cozmoContext->GetExperiments()->ActivateExperiment(request, outVariationKey);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.InitInternal",
                "Unconnected decay experiment activation assignment status is: %s; based on serial number %d",
                EnumToString(assignmentStatus), robotSerialNumber);
  if (assignmentStatus == Util::AnkiLab::AssignmentStatus::Assigned ||
      assignmentStatus == Util::AnkiLab::AssignmentStatus::OverrideAssigned ||
      assignmentStatus == Util::AnkiLab::AssignmentStatus::ForceAssigned)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.AttemptActivateDecayExperiment",
                  "Experiment %s: assigned to variation: %s",
                  kUnconnectedDecayRatesExperimentKey.c_str(),
                  outVariationKey.c_str());

    _needsConfig.SetUnconnectedDecayTestVariation(GetDecayConfigBaseFilename(),
                                                  outVariationKey,
                                                  assignmentStatus);
  }
  else
  {
    _needsConfig.SetUnconnectedDecayTestVariation(GetDecayConfigBaseFilename(),
                                                  _needsConfig.kABTestDecayConfigControlKey,
                                                  assignmentStatus);
  }
}


void NeedsManager::OnRobotDisconnected()
{
  _needsState._timeLastDisconnect = system_clock::now();
  _needsState._timesOpenedSinceLastDisconnect = 0;

  // Write latest needs state to device, but not if needs system is paused.
  // If we're paused, we've already written needs state to device when we
  // paused.  And we don't want to write again, because if the robot gets
  // disconnected while app is backgrounded (and thus paused), we don't
  // want to update the 'time last written' because when we un-background,
  // we use that time to apply accumulated decay.
  if (!_isPausedOverall)
  {
    WriteToDevice();
  }

  _savedTimeLastWrittenToDevice = _needsState._timeLastWritten;

  _robot = nullptr;

  static const bool forceSend = true;
  DetectBracketChangeForDas(forceSend);

  SendNeedsLevelsDasEvent("disconnect");
}


// This is called whether we are connected to a robot or not
void NeedsManager::Update(const float currentTime_s)
{
  _currentTime_s = currentTime_s;

  if (_isPausedOverall)
    return;

  _localNotifications->Update(currentTime_s);

  // Handle periodic decay:
  if (currentTime_s >= _timeForNextPeriodicDecay_s)
  {
    _timeForNextPeriodicDecay_s += _needsConfig._decayPeriod;

    const bool connected = _robot != nullptr;
    ApplyDecayAllNeeds(connected);

    SendNeedsStateToGame(NeedsActionId::Decay);

    // Note that we don't want to write to robot at this point, as that
    // can take a long time (300 ms) and can interfere with animations.
    // So we generally only write to robot on actions completed.

    // However, it's quick to write to device, so we [possibly] do that here:
    PossiblyWriteToDevice();
  }
}


void NeedsManager::SetPaused(const bool paused)
{
  if (paused == _isPausedOverall)
  {
    // This can happen for several reasons, e.g. being in explorer mode (which
    // pauses the needs system), and then backgrounding the app (which also pauses
    // the needs system)
    PRINT_CH_DEBUG(kLogChannelName, "NeedsManager.SetPaused.Redundant",
                   "Setting paused to %s but already in that state",
                   paused ? "true" : "false");
    return;
  }

  _isPausedOverall = paused;

  if (_isPausedOverall)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.SetPaused.Pausing",
                  "Pausing Needs system");
    // Calculate and record how much time was left until the next decay
    _pausedDurRemainingPeriodicDecay = _timeForNextPeriodicDecay_s - _currentTime_s;

    _timeWhenPausedOverall_s = _currentTime_s;

    // Send the current needs state to the game as soon as we pause
    //  (because the periodic decay won't happen during pause)
    SendNeedsStateToGame();

    // Now is a good time to save needs state; for example, in SDK mode we
    // will eventually disconnect when exiting SDK mode
    WriteToDevice();
  }
  else
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.SetPaused.UnPausing",
                  "Un-Pausing Needs system");
    // When unpausing, set the next 'time for periodic decay'
    _timeForNextPeriodicDecay_s = _currentTime_s + _pausedDurRemainingPeriodicDecay;

    // Then calculate how long we were paused
    const float durationOfPause = _currentTime_s - _timeWhenPausedOverall_s;

    // Adjust some timers accordingly, so that the overall pause is excluded from
    // things like decay time, and individual needs pausing
    for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
    {
      _lastDecayUpdateTime_s[needIndex] += durationOfPause;
      _timeWhenPaused_s[needIndex] += durationOfPause;

      if (_timeWhenCooldownOver_s[needIndex] != 0.0f)
      {
        _timeWhenCooldownOver_s[needIndex] += durationOfPause;
        _timeWhenCooldownStarted_s[needIndex] += durationOfPause;
      }

      _timeWhenBracketChanged_s[needIndex] += durationOfPause;
    }
  }

  _localNotifications->SetPaused(paused);

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
  if (_isPausedOverall)
  {
    return;
  }

  // Don't do anything if no robot connected.  This can happen during shutdown,
  // since we have so much code being executed from the robot class destructor
  if (_robot == nullptr)
  {
    return;
  }

  // Only accept certain types of events
  if (_onlyWhiteListedActionsEnabled)
  {
    if (_whiteListedActions.find(actionCompleted) == _whiteListedActions.end())
    {
      return;
    }
  }

  if (actionCompleted == NeedsActionId::CozmoSings)
  {
    _needsState._forceNextSong = UnlockId::Invalid;
  }

  const int actionIndex = static_cast<int>(actionCompleted);
  if (_currentTime_s < _actionCooldown_s[actionIndex])
  {
    // DAS Event: "needs.action_completed_ignored"
    // s_val: The needs action being completed
    // data: Unused
    Anki::Util::sEvent("needs.action_completed_ignored", {},
                       NeedsActionIdToString(actionCompleted));
    return;
  }
  const auto& actionDelta = _actionsConfig._actionDeltas[actionIndex];
  if (!Util::IsNearZero(actionDelta._cooldown_s))
  {
    _actionCooldown_s[actionIndex] = _currentTime_s + actionDelta._cooldown_s;
  }

  NeedsState::CurNeedsMap prevNeedsLevels = _needsState._curNeedsLevels;
  _needsState.SetPrevNeedsBrackets();

  RegisterNeedsActionCompletedInternal(actionCompleted, _needsState, false);

  // DAS Event: "needs.action_completed"
  // s_val: The needs action being completed
  // data: The needs levels before the completion, followed by the needs levels after
  //       the completion, all colon-separated (e.g. "1.0000:0.6000:0.7242:0.6000:0.5990:0.7202"
  std::ostringstream stream;
  FormatStringOldAndNewLevels(stream, prevNeedsLevels);
  Anki::Util::sEvent("needs.action_completed",
                     {{DDATA, stream.str().c_str()}},
                     NeedsActionIdToString(actionCompleted));

  SendNeedsStateToGame(actionCompleted);

  const bool starAwarded = UpdateStarsState();

  // If no daily star was awarded, possibly award sparks for freeplay activities
  if (!starAwarded)
  {
    // Also, generate the updated local notifications here (if a star was awarded, we've
    // already done this in UpdateStarsState)
    _localNotifications->Generate();

    if (!Util::IsNearZero(actionDelta._freeplaySparksRewardWeight))
    {
      if (ShouldRewardSparksForFreeplay())
      {
        const int sparksAwarded = RewardSparksForFreeplay();

        if (_pendingSparksRewardMsg)
        {
          PRINT_CH_INFO(kLogChannelName, "NeedsManager.RegisterNeedsActionCompleted",
                        "About to create freeplay sparks reward message but there was a pending one, so sending that one now");

          SparksRewardCommunicatedToUser();
        }
        _pendingSparksRewardMsg = true;

        // Fill in the message that we will send later when the reward behavior completes
        _sparksRewardMsg.sparksAwarded = sparksAwarded;
        _sparksRewardMsg.sparksAwardedDisplayKey = kFreeplaySparksRewardStringKey + "." +
                                                   NeedsActionIdToString(actionCompleted);

        // DAS Event: "needs.freeplay_sparks_awarded"
        // s_val: The number of sparks awarded
        // data: The need action ID that triggered this
        Anki::Util::sEvent("needs.freeplay_sparks_awarded",
                           {{DDATA, NeedsActionIdToString(actionCompleted)}},
                           std::to_string(sparksAwarded).c_str());
      }
    }
  }

  DetectBracketChangeForDas();

  PossiblyWriteToDevice();
  PossiblyStartWriteToRobot();
}


void NeedsManager::PredictNeedsActionResult(const NeedsActionId actionCompleted, NeedsState& outNeedsState)
{
  outNeedsState = _needsState;

  if (_isPausedOverall) {
    return;
  }

  // NOTE: Since an action's deltas can have a 'random uniform distribution', this code
  // is not fully accurate when that applies, since when the 'real' call is made, we'll
  // generate the random range again.  (We don't seem to have a random number generator
  // that allows us to extract the current seed, save it, and later restore it.)
  RegisterNeedsActionCompletedInternal(actionCompleted, outNeedsState, true);
}


void NeedsManager::RegisterNeedsActionCompletedInternal(const NeedsActionId actionCompleted,
                                                        NeedsState& needsState,
                                                        const bool predictionOnly)
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.RegisterNeedsActionCompletedInternal",
                "%s %s", predictionOnly ? "Predicted" : "Completed",
                NeedsActionIdToString(actionCompleted));
  const int actionIndex = static_cast<int>(actionCompleted);
  const auto& actionDelta = _actionsConfig._actionDeltas[actionIndex];

  switch (actionCompleted)
  {
    case NeedsActionId::RepairHead:
    {
      const auto partId = RepairablePartId::Head;
      needsState._partIsDamaged[partId] = false;
      if (!predictionOnly)
      {
        SendRepairDasEvent(needsState, actionCompleted, partId);
      }
      break;
    }
    case NeedsActionId::RepairLift:
    {
      const auto partId = RepairablePartId::Lift;
      needsState._partIsDamaged[partId] = false;
      if (!predictionOnly)
      {
        SendRepairDasEvent(needsState, actionCompleted, partId);
      }
      break;
    }
    case NeedsActionId::RepairTreads:
    {
      const auto partId = RepairablePartId::Treads;
      needsState._partIsDamaged[partId] = false;
      if (!predictionOnly)
      {
        SendRepairDasEvent(needsState, actionCompleted, partId);
      }
      break;
    }
    default:
      break;
  }

  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    if (_isActionsPausedForNeed[needIndex])
    {
      if (!predictionOnly)
      {
        NeedDelta deltaToSave = actionDelta._needDeltas[needIndex];
        deltaToSave._cause = actionCompleted;
        _queuedNeedDeltas[needIndex].push_back(deltaToSave);
      }
    }
    else
    {
      const NeedId needId = static_cast<NeedId>(needIndex);
      if (needsState.ApplyDelta(needId,
                                actionDelta._needDeltas[needIndex],
                                actionCompleted))
      {
        if (!predictionOnly)
        {
          StartFullnessCooldownForNeed(needId);
        }
      }
    }
  }

  switch (actionCompleted)
  {
    case NeedsActionId::RepairHead:
    case NeedsActionId::RepairLift:
    case NeedsActionId::RepairTreads:
    {
      if (needsState.NumDamagedParts() == 0)
      {
        // If this was a 'repair' action and there are no more broken parts,
        // set Repair level to 100%
        needsState._curNeedsLevels[NeedId::Repair] = _needsConfig._maxNeedLevel;
        needsState.SetNeedsBracketsDirty();
      }
      break;
    }
    default:
      break;
  }
}


bool NeedsManager::ShouldRewardSparksForFreeplay()
{
  auto& ic = _robot->GetInventoryComponent();
  const int curSparks = ic.GetInventoryAmount(InventoryType::Sparks);
  const int level = _needsState._curNeedsUnlockLevel;
  const float targetRatio = (Util::numeric_cast<float>(curSparks) /
                             _starRewardsConfig->GetFreeplayTargetSparksTotalForLevel(level));
  float rewardChancePct = 1.0f - targetRatio;
  const float minPct = _starRewardsConfig->GetFreeplayMinSparksRewardPctForLevel(level);
  if (rewardChancePct < minPct)
  {
    rewardChancePct = minPct;
  }

  return (_cozmoContext->GetRandom()->RandDbl() < rewardChancePct);
}


int NeedsManager::RewardSparksForFreeplay()
{
  const int level = _needsState._curNeedsUnlockLevel;
  const int sparksAdded = AwardSparks(_starRewardsConfig->GetFreeplayTargetSparksTotalForLevel(level),
                                      _starRewardsConfig->GetFreeplayMinSparksPctForLevel(level),
                                      _starRewardsConfig->GetFreeplayMaxSparksPctForLevel(level),
                                      _starRewardsConfig->GetFreeplayMinSparksForLevel(level),
                                      _starRewardsConfig->GetFreeplayMinMaxSparksForLevel(level));

  return sparksAdded;
}


int NeedsManager::AwardSparks(const int targetSparks, const float minPct, const float maxPct,
                              const int minSparks, const int minMaxSparks)
{
  auto& ic = _robot->GetInventoryComponent();
  const int curSparks = ic.GetInventoryAmount(InventoryType::Sparks);
  const int delta = targetSparks - curSparks;
  int min = ((delta * minPct) + 0.5f);
  int max = ((delta * maxPct) + 0.5f);

  if (min < minSparks)
  {
    min = minSparks;
  }
  if (max < minMaxSparks)
  {
    max = minMaxSparks;
  }
  const int sparksAdded = _cozmoContext->GetRandom()->RandIntInRange(min, max);

  ic.AddInventoryAmount(InventoryType::Sparks, sparksAdded);

  return sparksAdded;
}


void NeedsManager::SparksRewardCommunicatedToUser()
{
  DEV_ASSERT_MSG(_pendingSparksRewardMsg == true,
                 "NeedsManager.OnSparksRewardAnimComplete",
                 "About to send sparks reward message but it's already been sent");
  _pendingSparksRewardMsg = false;

  // Tell game that sparks were awarded, and how many, and what cozmo was doing
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame
                    (std::move(_sparksRewardMsg)));
}


void NeedsManager::SendRepairDasEvent(const NeedsState& needsState,
                                      const NeedsActionId cause,
                                      const RepairablePartId part)
{
  // DAS Event: "needs.part_repaired"
  // s_val: The name of the part repaired (RepairablePartId)
  // data: New number of damaged parts, followed by a colon, followed
  //       by the cause of repair (NeedsActionId)
  std::string data = std::to_string(needsState.NumDamagedParts()) + ":" +
                     NeedsActionIdToString(cause);
  Anki::Util::sEvent("needs.part_repaired",
                     {{DDATA, data.c_str()}},
                     RepairablePartIdToString(part));
}


void NeedsManager::FormatStringOldAndNewLevels(std::ostringstream& stream,
                                               NeedsState::CurNeedsMap& prevNeedsLevels)
{
  stream.precision(5);
  stream << std::fixed;
  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    if (needIndex > 0)
    {
      stream << ":";
    }
    stream << prevNeedsLevels[static_cast<NeedId>(needIndex)];
  }
  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    stream << ":" << _needsState.GetNeedLevelByIndex(needIndex);
  }
}


void NeedsManager::SendNeedsLevelsDasEvent(const char * whenTag)
{
  // DAS Event: "needs.needs_levels"
  // s_val: The 'when' tag ("app_start", "app_background", "app_unbackground", "disconnect")
  // data: The needs levels at that time, colon-separated
  //       (e.g. "1.0000:0.6000:0.7242")
  std::ostringstream stream;
  stream.precision(5);
  stream << std::fixed;
  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    if (needIndex > 0)
    {
      stream << ":";
    }
    stream << _needsState.GetNeedLevelByIndex(needIndex);
  }
  Anki::Util::sEvent("needs.needs_levels", {{DDATA, whenTag}}, stream.str().c_str());
}


void NeedsManager::SendTimeSinceBackgroundedDasEvent()
{
  if (_needsState._timeLastAppBackgrounded != Time()) // (don't send if we've never backgrounded)
  {
    // DAS Event: "needs.app_backgrounded_time"
    // s_val: The number of seconds since the last time the user backgrounded
    //        the app
    // data: The number of times the user has opened or unbackgrounded the app
    //       since the last robot disconnection
    const Time now = system_clock::now();
    const auto elapsed = now - _needsState._timeLastAppBackgrounded;
    const auto secsSinceLastBackgrounded = duration_cast<seconds>(elapsed).count();
    Anki::Util::sEvent("needs.app_backgrounded_time",
                       {{DDATA, std::to_string(_needsState._timesOpenedSinceLastDisconnect).c_str()}},
                       std::to_string(secsSinceLastBackgrounded).c_str());
  }
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetStarStatus& msg)
{
  bool givenStarToday = true;
  // Now see if they've already received the star award today:
  const std::time_t lastStarTime = system_clock::to_time_t( _needsState._timeLastStarAwarded );
  std::tm lastLocalTime;
  localtime_r(&lastStarTime, &lastLocalTime);
  
  const std::time_t nowTimeT = system_clock::to_time_t( system_clock::now() );
  std::tm nowLocalTime;
  localtime_r(&nowTimeT, &nowLocalTime);
  
  // Is it past midnight (a different day-of-year (0-365), or a different year)
  if (nowLocalTime.tm_yday != lastLocalTime.tm_yday || nowLocalTime.tm_year != lastLocalTime.tm_year)
  {
    givenStarToday = false;
  }
  
  std::tm t;
  localtime_r(&nowTimeT, &t);
  // Todays time, reset to last midnight
  t.tm_sec = t.tm_min = t.tm_hour = 0;
  time_t lastMidnightTimeT = mktime(&t);
  system_clock::time_point lastMidnightTimePoint = system_clock::from_time_t(lastMidnightTimeT);
  // We need midnight tomorrow, not tonight.
  time_t timeToNextDay = system_clock::to_time_t(lastMidnightTimePoint + hours(24));
  
  time_t timeRemainingToNextDay_s = timeToNextDay - nowTimeT;
  
  // Send the time to game...
  ExternalInterface::StarStatus message((int)(timeRemainingToNextDay_s),givenStarToday);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsState& msg)
{
  SendNeedsStateToGame();
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetSongsList& msg)
{
  std::vector<ExternalInterface::SongUnlockStatus> songUnlockStatuses;

  // Begin the list with all of the default-unlocked songs
  const auto& defaultUnlocks = ProgressionUnlockComponent::GetDefaultUnlocks(_cozmoContext);
  for (const auto& defaultUnlock : defaultUnlocks)
  {
    std::string defaultUnlockStr = UnlockIdToString(defaultUnlock);
    static const std::string prefix = "Singing_";
    if (defaultUnlockStr.compare(0, prefix.length(), prefix) == 0)
    {
      ExternalInterface::SongUnlockStatus songUnlockStatus(true, defaultUnlockStr.c_str());
      songUnlockStatuses.emplace_back(songUnlockStatus);
    }
  }

  // Add songs from the level rewards list, in order, checking unlock status for each.
  // By going through the level rewards list, we ensure we are not including songs that
  // are in the app but not yet 'released' (and therefore can never be unlocked)
  auto& puc = _robot->GetProgressionUnlockComponent();
  const int numLevels = _starRewardsConfig->GetNumLevels();
  std::vector<NeedsReward> rewards;
  for (int level = 0; level < numLevels; level++)
  {
    _starRewardsConfig->GetRewardsForLevel(level, rewards);
    for (const auto& reward : rewards)
    {
      if (reward.rewardType == NeedsRewardType::Song)
      {
        const UnlockId unlockId = UnlockIdFromString(reward.data);
        const bool isUnlocked = puc.IsUnlocked(unlockId);
        ExternalInterface::SongUnlockStatus songUnlockStatus(isUnlocked, reward.data.c_str());
        songUnlockStatuses.emplace_back(songUnlockStatus);
      }
    }
  }

  ExternalInterface::SongsList message(std::move(songUnlockStatuses));
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}


template<>
void NeedsManager::HandleMessage(const ExternalInterface::ForceSetNeedsLevels& msg)
{
  NeedsState::CurNeedsMap prevNeedsLevels = _needsState._curNeedsLevels;

  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    float newLevel = msg.newNeedLevel[needIndex];
    newLevel = Util::Clamp(newLevel, _needsConfig._minNeedLevel, _needsConfig._maxNeedLevel);
    _needsState._curNeedsLevels[static_cast<NeedId>(needIndex)] = newLevel;
  }

  _needsState.SetNeedsBracketsDirty();

  // Note that we don't set the appropriate number of broken parts here, because we're
  // just using this to fake needs levels during onboarding, and we will fully initialize
  // after onboarding completes.  The ForceSetDamagedParts message below can be used to
  // set whether each part is damaged.

  SendNeedsStateToGame();

  // DAS Event: "needs.force_set_needs_levels"
  // s_val: The needs levels before the completion, followed by the needs levels after
  //        the completion, all colon-separated (e.g. "1.0000:0.6000:0.7242:0.6000:0.5990:0.7202"
  // data: Unused
  std::ostringstream stream;
  FormatStringOldAndNewLevels(stream, prevNeedsLevels);
  Anki::Util::sEvent("needs.force_set_needs_levels", {}, stream.str().c_str());
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::ForceSetDamagedParts& msg)
{
  for (size_t i = 0; i < RepairablePartIdNumEntries; i++)
  {
    _needsState._partIsDamaged[static_cast<RepairablePartId>(i)] = msg.partIsDamaged[i];
  }

  SendNeedsStateToGame();

  // DAS Event: "needs.force_set_damaged_parts"
  // s_val: Colon-separated list of bools (expressed as 1 or 0) for whether each
  //        repairable part is damaged
  // data: Unused
  std::ostringstream stream;
  for (size_t i = 0; i < RepairablePartIdNumEntries; i++)
  {
    if (i > 0)
    {
      stream << ":";
    }
    stream << (msg.partIsDamaged[i] ? "1" : "0");
  }
  Anki::Util::sEvent("needs.force_set_damaged_parts", {}, stream.str().c_str());
}
  
template<>
void NeedsManager::HandleMessage(const ExternalInterface::SetNeedsActionWhitelist& msg)
{
  _onlyWhiteListedActionsEnabled = msg.enable;
  _whiteListedActions.clear();
  if( _onlyWhiteListedActionsEnabled )
  {
    std::copy(msg.whitelistedActions.begin(), msg.whitelistedActions.end(),
              std::inserter(_whiteListedActions, _whiteListedActions.end()));
  }
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::RegisterOnboardingComplete& msg)
{
  if( msg.onboardingStage < _robotOnboardingStageCompleted )
  {
    // only complete resets are allowed, however if connecting to a new robot from an old
    // app we do allow people to restart onboarding.
    PRINT_NAMED_INFO("NeedsManager.HandleMessage.RegisterOnboardingComplete",
                        "Negative onboarding progress %d -> %d is ignored",
                        _robotOnboardingStageCompleted, msg.onboardingStage);
  }
  else
  {
    _robotOnboardingStageCompleted = msg.onboardingStage;
  }
  PRINT_CH_INFO(kLogChannelName, "RegisterOnboardingComplete",
                "OnboardingStageCompleted: %d, finalStage: %d",
                _robotOnboardingStageCompleted, msg.finalStage);

  // phase 1 is just the first part showing the needs hub.
  if( msg.finalStage )
  {
    // Reset cozmo's need levels to their starting points, and reset timers
    InitReset(_currentTime_s, _needsState._robotSerialNumber);

    // onboarding unlocks one star.
    _needsState._numStarsAwarded = 1;
    const Time nowTime = system_clock::now();
    _needsState._timeLastStarAwarded = nowTime;

    // Un-pause the needs system if we are not already
    if (_isPausedOverall)
    {
      SetPaused(false);
    }

    SendNeedsStateToGame();

    // Re-generate the notifications since we have a different number of stars
    _localNotifications->Generate();

    // DAS Event: "needs.onboarding_completed"
    // s_val: Unused
    // data: Unused
    Anki::Util::sEvent("needs.onboarding_completed", {}, "");
  }

  // onboarding phases are very important for not having to repeat view the charging screen so force write.
  // we also know that they take several seconds to complete so won't be spammy.
  PossiblyStartWriteToRobot(true);
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

  _needsState.SetPrevNeedsBrackets();

  // Pause/unpause for decay
  NeedsMultipliers multipliers;
  bool multipliersSet = false;
  bool needsLevelsChanged = false;

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
        // (But do nothing if we're in a 'fullness cooldown')
        if (_timeWhenCooldownOver_s[needIndex] == 0.0f)
        {
          // Apply the decay for the period the need was paused
          if (!multipliersSet)
          {
            // Set the multipliers only once even if we're applying decay to mulitiple needs at
            // once.  This is to make it "fair", as multipliers are set according to need levels
            multipliersSet = true;
            _needsState.GetDecayMultipliers(_needsConfig._decayConnected, multipliers);
          }
          const float duration_s = _currentTime_s - _lastDecayUpdateTime_s[needIndex];
          _needsState.ApplyDecay(_needsConfig._decayConnected, needIndex, duration_s, multipliers);
          _lastDecayUpdateTime_s[needIndex] = _currentTime_s;
          needsLevelsChanged = true;
        }
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
        const NeedId needId = static_cast<NeedId>(needIndex);
        if (_needsState.ApplyDelta(needId, queuedDeltas[j], queuedDeltas[j]._cause))
        {
          StartFullnessCooldownForNeed(needId);
        }
        needsLevelsChanged = true;
      }
      queuedDeltas.clear();
    }

    _isActionsPausedForNeed[needIndex] = msg.actionPause[needIndex];
  }

  if (needsLevelsChanged)
  {
    SendNeedsStateToGame();

    if (_robot != nullptr)
    {
      UpdateStarsState();
    }

    DetectBracketChangeForDas();

    _localNotifications->Generate();
  }
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetNeedsPauseStates& msg)
{
  SendNeedsPauseStatesToGame();
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::GetWantsNeedsOnboarding& msg)
{
  SendNeedsOnboardingToGame();
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::WipeDeviceNeedsData& msg)
{
  Util::FileUtils::DeleteFile(kPathToSavedStateFile + kNeedsStateFile);

  if (msg.reinitializeNeeds)
  {
    InitInternal(_currentTime_s);
  }
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::WipeRobotGameData& msg)
{
  // When the debug 'erase everything' button is pressed, or the user-facing
  // "ERASE COZMO" button is pressed, that means we also need to re-initialize
  // the needs system

  Util::FileUtils::DeleteFile(kPathToSavedStateFile + kNeedsStateFile);
  // ensures onboarding starts from the beginning & w/o option to skip
  _robotOnboardingStageCompleted = 0;

  InitInternal(_currentTime_s);
}

template<>
void NeedsManager::HandleMessage(const ExternalInterface::WipeRobotNeedsData& msg)
{
  if (nullptr == _robot) {
    PRINT_NAMED_WARNING("NeedsManager.WipeRobotNeedsData.NoRobot", "No active robot");
    return;
  }

  if (!_robot->GetNVStorageComponent().Erase(NVStorage::NVEntryTag::NVEntry_NurtureGameData,
                                             [this](NVStorage::NVResult res)
                                             {
                                               bool success;
                                               if(res < NVStorage::NVResult::NV_OKAY)
                                               {
                                                 success = false;
                                                 PRINT_NAMED_WARNING("NeedsManager.WipeRobotNeedsData",
                                                                     "Erase of needs data failed with %s",
                                                                     EnumToString(res));
                                               }
                                               else
                                               {
                                                 success = true;
                                                 PRINT_NAMED_INFO("NeedsManager.WipeRobotNeedsData",
                                                                  "Erase of needs complete!");
                                               }
                                               const auto& extInt = _cozmoContext->GetExternalInterface();
                                               extInt->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RestoreRobotStatus(true, success)));

                                               // ensures onboarding starts from the beginning & w/o option to skip
                                               _robotOnboardingStageCompleted = 0;
                                               InitInternal(_currentTime_s);
                                             }))
  {
    PRINT_NAMED_ERROR("NeedsManager.WipeRobotNeedsData.EraseFailed", "Erase failed");
  }
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

  if (msg.isPaused)
  {
    _needsState._timeLastAppBackgrounded = system_clock::now();
  }

  // During app backgrounding, we want to pause the whole needs system
  SetPaused(msg.isPaused);

  if (msg.isPaused)
  {
    // When backgrounding, we'll also write to robot if connected
    if (_robot != nullptr)
    {
      // We pass in the time that was just set in SetPaused (above), when we
      // wrote to device; this way, we're setting the robot save's 'time last
      // written' to the EXACT SAME time
      StartWriteToRobot(_needsState._timeLastWritten);
    }

    SendNeedsLevelsDasEvent("app_background");

    if (_needsState._robotSerialNumber != kUninitializedSerialNumber)
    {
      // If we have a valid robot serial number, make a copy of the save file
      // we just wrote out (in the call to SetPaused above), with the serial
      // number as part of the filename.
      // We do this to fix the case of multiple robots on one device, and the
      // user corrupts the robot's needs save by killing the app right after
      // backgrounding, before the robot save completes.

#if ENABLE_NEEDS_READ_WRITE_PROFILING
      const auto startTime = system_clock::now();
#endif

      const std::string destFile = kPathToSavedStateFile +
                                   NeedsFilenameFromSerialNumber(_needsState._robotSerialNumber);

      const std::string srcFile = kPathToSavedStateFile + kNeedsStateFile;
#if ENABLE_NEEDS_READ_WRITE_PROFILING
      const auto midTime = system_clock::now();
#endif
      if (!Util::FileUtils::CopyFile(destFile, srcFile))
      {
        PRINT_NAMED_ERROR("NeedsManager.SetGameBeingPaused",
                          "Error copying %s to %s",
                          srcFile.c_str(), destFile.c_str());
      }
#if ENABLE_NEEDS_READ_WRITE_PROFILING
      const auto endTime = system_clock::now();
      const auto microsecsMid = duration_cast<microseconds>(endTime - midTime);
      const auto microsecs = duration_cast<microseconds>(endTime - startTime);
      PRINT_CH_INFO(kLogChannelName, "NeedsManager.CopyFile",
                    "Copying needs state file took %lld microseconds total; %lld microseconds for the copy",
                    microsecs.count(), microsecsMid.count());
#endif
    }

    // It's too late to generate local notifications, which is why we
    // generate them peridically and on certain events
  }
  else
  {
    // When un-backgrounding, apply decay for the time we were backgrounded

    // Attempt to set the experiment variation for unconnected decay; this
    // is to cover the case where the user backgrounds the app, and then
    // the experiment's end date passes; in that case we want to switch
    // back to the normal unconnected decay rates
    AttemptActivateDecayExperiment(_needsState._robotSerialNumber);

    const bool connected = (_robot == nullptr ? false : true);
    ApplyDecayForTimeSinceLastDeviceWrite(connected);

    _needsState._timesOpenedSinceLastDisconnect++;

    SendNeedsStateToGame(NeedsActionId::Decay);

    SendNeedsLevelsDasEvent("app_unbackground");

    SendTimeSinceBackgroundedDasEvent();

    _needsState._timeLastAppUnBackgrounded = system_clock::now();
  }
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
  needBrackets.reserve((size_t)NeedId::Count);
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const NeedBracketId bracketId = _needsState.GetNeedBracketByIndex(i);
    needBrackets.push_back(bracketId);
  }
  
  std::vector<bool> partIsDamaged;
  partIsDamaged.reserve(RepairablePartIdNumEntries);
  for (size_t i = 0; i < RepairablePartIdNumEntries; i++)
  {
    const bool isDamaged = _needsState.GetPartIsDamagedByIndex(i);
    partIsDamaged.push_back(isDamaged);
  }

  const std::string ABTestString = kUnconnectedDecayRatesExperimentKey + ": " +
                                   _needsConfig.GetUnconnectedDecayTestVariation();

  ExternalInterface::NeedsState message(std::move(needLevels), std::move(needBrackets),
                                        std::move(partIsDamaged), _needsState._curNeedsUnlockLevel,
                                        _needsState._numStarsAwarded, _needsState._numStarsForNextUnlock,
                                        actionCausingTheUpdate, std::move(ABTestString));
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));

  SendNeedsDebugVizString(actionCausingTheUpdate);
}

void NeedsManager::SendNeedsDebugVizString(const NeedsActionId actionCausingTheUpdate)
{
#if ANKI_DEV_CHEATS

  // Example string:
  // Eng:0.31-Warn Play:1.00-Full Repr:0.05-Crit HiccupsEndGood

  _cozmoContext->GetVizManager()->SetText(
    VizManager::NEEDS_STATE, NamedColors::ORANGE,
    "Eng:%04.2f-%.4s Play:%04.2f-%.4s Repr:%04.2f-%.4s %s",
    _needsState.GetNeedLevel(NeedId::Energy),
    NeedBracketIdToString(_needsState.GetNeedBracket(NeedId::Energy)),
    _needsState.GetNeedLevel(NeedId::Play),
    NeedBracketIdToString(_needsState.GetNeedBracket(NeedId::Play)),
    _needsState.GetNeedLevel(NeedId::Repair),
    NeedBracketIdToString(_needsState.GetNeedBracket(NeedId::Repair)),
    NeedsActionIdToString(actionCausingTheUpdate));
  
#endif

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


void NeedsManager::ApplyDecayAllNeeds(const bool connected)
{
  const DecayConfig& config = connected ? _needsConfig._decayConnected : _needsConfig._decayUnconnected;

  _needsState.SetPrevNeedsBrackets();

  NeedsMultipliers multipliers;
  _needsState.GetDecayMultipliers(config, multipliers);

  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    if (!_isDecayPausedForNeed[needIndex])
    {
      if (_timeWhenCooldownOver_s[needIndex] != 0.0f &&
          _currentTime_s > _timeWhenCooldownOver_s[needIndex])
      {
        // There was a 'fullness cooldown' for this need, and it has expired;
        // calculate the amount of decay time we need to adjust for
        const float cooldownDuration = _timeWhenCooldownOver_s[needIndex] -
                                       _timeWhenCooldownStarted_s[needIndex];
        _lastDecayUpdateTime_s[needIndex] += cooldownDuration;

        _timeWhenCooldownOver_s[needIndex] = 0.0f;
        _timeWhenCooldownStarted_s[needIndex] = 0.0f;
      }

      if (_timeWhenCooldownOver_s[needIndex] == 0.0f)
      {
        const float duration_s = _currentTime_s - _lastDecayUpdateTime_s[needIndex];
        _needsState.ApplyDecay(config, needIndex, duration_s, multipliers);
        _lastDecayUpdateTime_s[needIndex] = _currentTime_s;
      }
    }
  }

  DetectBracketChangeForDas();
}


void NeedsManager::ApplyDecayForTimeSinceLastDeviceWrite(const bool connected)
{
  // Calculate time elapsed since last connection
  const Time now = system_clock::now();
  const float elasped_s = duration_cast<seconds>(now - _needsState._timeLastWritten).count();

  // Now apply decay according to appropriate config, and elapsed time
  // First, however, we set the timers as if that much time had elapsed:
  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    _lastDecayUpdateTime_s[needIndex] = _currentTime_s - elasped_s;

    if (_timeWhenCooldownOver_s[needIndex] != 0.0f)
    {
      // Note: There is a very small chance that the following subtraction
      // could result in a value of exactly zero, which means 'no cooldown'.
      // In that case ApplyDecayAllNeeds would not account for the remaining
      // fullness cooldown period to not apply decay.  A real fix for this
      // would be to have another array of bools for '_cooldownActive'
      // (instead of double-use of this _timeWhenCooldownOver_s)
      _timeWhenCooldownOver_s[needIndex] -= elasped_s;
      _timeWhenCooldownStarted_s[needIndex] -= elasped_s;
    }
  }

  ApplyDecayAllNeeds(connected);
}


void NeedsManager::StartFullnessCooldownForNeed(const NeedId needId)
{
  const int needIndex = static_cast<int>(needId);

  _timeWhenCooldownOver_s[needIndex] = _currentTime_s +
                                       _needsConfig._fullnessDecayCooldownTimes_s[needId];

  if (_timeWhenCooldownStarted_s[needIndex] == 0.0f)
  {
    _timeWhenCooldownStarted_s[needIndex] = _currentTime_s;
  }
}

  
bool NeedsManager::UpdateStarsState(const bool cheatGiveStar)
{
  bool starAwarded = false;

  // If "Play" level has transitioned to Full
  if (((_needsState.GetPrevNeedBracketByIndex((size_t)NeedId::Play) != NeedBracketId::Full) &&
       (_needsState.GetNeedBracketByIndex((size_t)NeedId::Play) == NeedBracketId::Full)) ||
       (cheatGiveStar))
  {
    // Now see if they've already received the star award today:
    const std::time_t lastStarTime = system_clock::to_time_t( _needsState._timeLastStarAwarded );
    std::tm lastLocalTime;
    localtime_r(&lastStarTime, &lastLocalTime);

    const Time nowTime = system_clock::now();
    const std::time_t nowTimeT = system_clock::to_time_t( nowTime );
    std::tm nowLocalTime;
    localtime_r(&nowTimeT, &nowLocalTime);
    
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.UpdateStarsState",
                  "Local time gmt offset %ld",
                  nowLocalTime.tm_gmtoff);

    // Is it past midnight (a different day-of-year (0-365), or a different year)
    if (nowLocalTime.tm_yday != lastLocalTime.tm_yday || nowLocalTime.tm_year != lastLocalTime.tm_year)
    {
      starAwarded = true;

      PRINT_CH_INFO(kLogChannelName, "NeedsManager.UpdateStarsState",
                    "now day: %d, lastsave day: %d, level: %d",
                    nowLocalTime.tm_yday, lastLocalTime.tm_yday,
                    _needsState._curNeedsUnlockLevel);
      
      _needsState._timeLastStarAwarded = nowTime;
      _needsState._numStarsAwarded++;
      SendStarUnlockedToGame();
      
      // Completed a set
      if (_needsState._numStarsAwarded >= _needsState._numStarsForNextUnlock)
      {
        // resets the stars
        SendStarLevelCompletedToGame();
      }

      // Save that we've issued a star today
      PossiblyStartWriteToRobot(true);

      // Re-generate the notifications since we have a different number of stars
      _localNotifications->Generate();
    }

    // DAS Event: "needs.play_need_filled"
    // s_val: Whether a daily star was awarded (1 or 0)
    // data: New current level
    Anki::Util::sEvent("needs.play_need_filled",
                       {{DDATA, std::to_string(_needsState._curNeedsUnlockLevel).c_str()}},
                       starAwarded ? "1" : "0");
  }

  return starAwarded;
}


void NeedsManager::SendStarLevelCompletedToGame()
{
  // Since the rewards config can be changed after this feature is launched,
  // we want to be able to give users the unlocks they might have missed if
  // they are past a level that has been modified to have an unlock that they
  // don't have.  But we also limit the number of these 'prior level unlocks'
  // so we don't overwhelm them with a ton on any single level unlock.

  std::vector<NeedsReward> rewards;

  // First, see about any prior level unlocks that have not occurred due to a
  // change in the rewards config as described above:
  int allowedPriorUnlocks = _starRewardsConfig->GetMaxPriorUnlocksForLevel(_needsState._curNeedsUnlockLevel);
  static const bool unlocksOnly = true;
  for (int level = 0; level < _needsState._curNeedsUnlockLevel; level++)
  {
    if (allowedPriorUnlocks <= 0)
    {
      break;
    }
    ProcessLevelRewards(level, rewards, unlocksOnly, &allowedPriorUnlocks);
  }

  // Now get the rewards for the level they are unlocking
  ProcessLevelRewards(_needsState._curNeedsUnlockLevel, rewards);

  // level up
  _needsState.SetStarLevel(_needsState._curNeedsUnlockLevel + 1);

  ExternalInterface::StarLevelCompleted message(_needsState._curNeedsUnlockLevel,
                                                _needsState._numStarsForNextUnlock,
                                                std::move(rewards));
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.SendStarLevelCompletedToGame",
                "CurrUnlockLevel: %d, stars for next: %d, currStars: %d",
                _needsState._curNeedsUnlockLevel, _needsState._numStarsForNextUnlock,
                _needsState._numStarsAwarded);
}


void NeedsManager::ProcessLevelRewards(const int level, std::vector<NeedsReward>& rewards,
                                       const bool unlocksOnly, int* allowedPriorUnlocks)
{
  std::vector<NeedsReward> rewardsThisLevel;
  _starRewardsConfig->GetRewardsForLevel(level, rewardsThisLevel);

  // Issue rewards in inventory
  for (int rewardIndex = 0; rewardIndex < rewardsThisLevel.size(); ++rewardIndex)
  {
    const auto& rewardType = rewardsThisLevel[rewardIndex].rewardType;

    switch(rewardType)
    {
      case NeedsRewardType::Sparks:
      {
        if (unlocksOnly)
        {
          continue;
        }

        const int sparksSpaceRemaining = _robot->GetInventoryComponent().GetInventorySpaceRemaining(InventoryType::Sparks);
        const int numAttemptedSparksAwarded = AwardSparks(_starRewardsConfig->GetTargetSparksTotalForLevel(level),
                                            _starRewardsConfig->GetMinSparksPctForLevel(level),
                                            _starRewardsConfig->GetMaxSparksPctForLevel(level),
                                            _starRewardsConfig->GetMinSparksForLevel(level),
                                            _starRewardsConfig->GetMinMaxSparksForLevel(level));

        rewards.push_back(rewardsThisLevel[rewardIndex]);

        // Put the attempted number of sparks awarded into the rewards data that we're about
        // to send to the game, so game will know how many sparks were attempted
        rewards.back().data = std::to_string(numAttemptedSparksAwarded);
        
        rewards.back().inventoryIsFull = sparksSpaceRemaining != InventoryComponent::kInfinity
                                         && sparksSpaceRemaining < numAttemptedSparksAwarded;
        
        // DAS Event: "needs.level_sparks_awarded"
        // s_val: The number of sparks awarded
        // data: Which level was just unlocked
        Anki::Util::sEvent("needs.level_sparks_awarded",
                           {{DDATA, std::to_string(_needsState._curNeedsUnlockLevel).c_str()}},
                           std::to_string(numAttemptedSparksAwarded).c_str());
        break;
      }
      // Songs are treated exactly the same as any other unlock
      case NeedsRewardType::Unlock:
      case NeedsRewardType::Song:
      {
        const UnlockId id = UnlockIdFromString(rewardsThisLevel[rewardIndex].data);
        if (id != UnlockId::Invalid)
        {
          auto& puc = _robot->GetProgressionUnlockComponent();
          if (!puc.IsUnlocked(id))
          {
            _robot->GetProgressionUnlockComponent().SetUnlock(id, true);
            rewards.push_back(rewardsThisLevel[rewardIndex]);
            if (rewardType == NeedsRewardType::Song)
            {
              _needsState._forceNextSong = id;
            }
            if (allowedPriorUnlocks != nullptr)
            {
              (*allowedPriorUnlocks)--;
              if (*allowedPriorUnlocks <= 0)
              {
                break;
              }
            }
          }
          else
          {
            // This is probably not an error case, because of post-launch 'prior level'
            // unlocks that can occur if/when we change the reward level unlock config
            if (!unlocksOnly)
            {
              PRINT_NAMED_WARNING("NeedsManager.ProcessLevelRewards",
                                  "Level reward is already unlocked: %s",
                                  UnlockIdToString(id));
            }
          }
        }
        else
        {
          PRINT_NAMED_ERROR("NeedsManager.ProcessLevelRewards",
                            "Level reward has invalid ID: %s",
                            rewardsThisLevel[rewardIndex].data.c_str());
        }
        break;
      }
      case NeedsRewardType::MemoryBadge:
      {
        // TODO: support memory badges in the future
        rewards.push_back(rewardsThisLevel[rewardIndex]);
        break;
      }
      default:
        break;
    }
  }
}


void NeedsManager::SendStarUnlockedToGame()
{
  ExternalInterface::StarUnlocked message(_needsState._curNeedsUnlockLevel,
                                          _needsState._numStarsForNextUnlock,
                                          _needsState._numStarsAwarded);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::SendNeedsOnboardingToGame()
{
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.SendNeedsOnboardingToGame",
                "OnboardingStageCompleted %d",
                _robotOnboardingStageCompleted);
  ExternalInterface::WantsNeedsOnboarding message(_robotOnboardingStageCompleted);
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

void NeedsManager::DetectBracketChangeForDas(const bool forceSend)
{
  for (int needIndex = 0; needIndex < static_cast<int>(NeedId::Count); needIndex++)
  {
    const auto oldBracket = _needsState.GetPrevNeedBracketByIndex(needIndex);
    const auto newBracket = _needsState.GetNeedBracketByIndex(needIndex);

    if (forceSend || (oldBracket != newBracket))
    {
      // DAS Event: "needs.bracket_changed"
      // s_val: The need whose bracket is changing (e.g. "Play")
      // data: Old bracket name, followed by new bracket name, followed by the
      //       number of seconds we've been in the old bracket (while
      //       connected), separated by colons, e.g. "Normal:Full:287"
      //       (Note that when we send this on robot disconnect, the old and
      //       new brackets will be the same)
      const int elapsed_s = Util::numeric_cast<int>(_currentTime_s -
                                                    _timeWhenBracketChanged_s[needIndex]);
      std::string data = std::string(NeedBracketIdToString(oldBracket)) + ":" +
                         std::string(NeedBracketIdToString(newBracket)) + ":" +
                         std::to_string(elapsed_s);
      Anki::Util::sEvent("needs.bracket_changed",
                         {{DDATA, data.c_str()}},
                         NeedIdToString(static_cast<NeedId>(needIndex)));

      if (!forceSend)
      {
        _timeWhenBracketChanged_s[needIndex] = _currentTime_s;
      }
    }
  }
}


bool NeedsManager::DeviceHasNeedsState(const std::string& filename)
{
  return Util::FileUtils::FileExists(kPathToSavedStateFile + filename);
}

void NeedsManager::PossiblyWriteToDevice()
{
  const Time now = system_clock::now();
  const auto elapsed = now - _needsState._timeLastWritten;
  const auto secsSinceLastSave = duration_cast<seconds>(elapsed).count();
  if (secsSinceLastSave > kMinimumTimeBetweenDeviceSaves_sec)
  {
    _needsState._timeLastWritten = now;
    WriteToDevice(false);
  }
}

void NeedsManager::WriteToDevice(bool stampWithNowTime /* = true */)
{
  const auto startTime = system_clock::now();

  if (stampWithNowTime)
  {
    _needsState._timeLastWritten = startTime;
  }

  Json::Value state;

  state[kStateFileVersionKey] = NeedsState::kDeviceStorageVersion;

  const auto time_s = duration_cast<seconds>(_needsState._timeLastWritten.time_since_epoch()).count();
  state[kDateTimeKey] = Util::numeric_cast<Json::LargestInt>(time_s);

  const auto timeCreated_s = duration_cast<seconds>(_needsState._timeCreated.time_since_epoch()).count();
  state[kTimeCreatedKey] = Util::numeric_cast<Json::LargestInt>(timeCreated_s);

  const auto timeLastDisconnect_s = duration_cast<seconds>(_needsState._timeLastDisconnect.time_since_epoch()).count();
  state[kTimeLastDisconnectKey] = Util::numeric_cast<Json::LargestInt>(timeLastDisconnect_s);

  const auto timeLastAppBackgrounded_s = duration_cast<seconds>(_needsState._timeLastAppBackgrounded.time_since_epoch()).count();
  state[kTimeLastAppBackgroundedKey] = Util::numeric_cast<Json::LargestInt>(timeLastAppBackgrounded_s);

  state[kOpenAppAfterDisconnectKey] = _needsState._timesOpenedSinceLastDisconnect;

  state[kSerialNumberKey] = _needsState._robotSerialNumber;

  state[kCurNeedsUnlockLevelKey] = _needsState._curNeedsUnlockLevel;
  state[kNumStarsAwardedKey] = _needsState._numStarsAwarded;
  state[kNumStarsForNextUnlockKey] = _needsState._numStarsForNextUnlock;

  for (const auto& need : _needsState._curNeedsLevels)
  {
    const int levelAsInt = Util::numeric_cast<int>((need.second * kNeedLevelStorageMultiplier) + 0.5f);
    state[kCurNeedLevelKey][EnumToString(need.first)] = levelAsInt;
  }
  for (const auto& part : _needsState._partIsDamaged)
  {
    state[kPartIsDamagedKey][EnumToString(part.first)] = part.second;
  }

  const auto timeStarAwarded_s = duration_cast<seconds>(_needsState._timeLastStarAwarded.time_since_epoch()).count();
  state[kTimeLastStarAwardedKey] = Util::numeric_cast<Json::LargestInt>(timeStarAwarded_s);

  state[kForceNextSongKey] = EnumToString(_needsState._forceNextSong);

#if ENABLE_NEEDS_READ_WRITE_PROFILING
  const auto midTime = system_clock::now();
#endif
  if (!_cozmoContext->GetDataPlatform()->writeAsJson(Util::Data::Scope::Persistent, GetNurtureFolder() + kNeedsStateFile, state))
  {
    PRINT_NAMED_ERROR("NeedsManager.WriteToDevice.WriteStateFailed", "Failed to write needs state file");
  }

#if ENABLE_NEEDS_READ_WRITE_PROFILING
  const auto endTime = system_clock::now();
  const auto microsecsMid = duration_cast<microseconds>(endTime - midTime);
  const auto microsecs = duration_cast<microseconds>(endTime - startTime);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.WriteToDevice",
                "Write to device took %lld microseconds total; %lld microseconds for the actual write",
                microsecs.count(), microsecsMid.count());
#endif
}


bool NeedsManager::AttemptReadFromDevice(const std::string& filename, bool& versionUpdated)
{
  bool valid = false;

  if (DeviceHasNeedsState(filename))
  {
    if (ReadFromDevice(filename, versionUpdated))
    {
      valid = true;

      // Save the time this save was made, for later comparison in InitAfterReadFromRobotAttempt
      _savedTimeLastWrittenToDevice = _needsState._timeLastWritten;

      static const bool connected = false;
      ApplyDecayForTimeSinceLastDeviceWrite(connected);

      SendNeedsStateToGame(NeedsActionId::Decay);

      _needsState._timesOpenedSinceLastDisconnect++;

      SendTimeSinceBackgroundedDasEvent();
    }
  }

  return valid;
}


bool NeedsManager::ReadFromDevice(const std::string& filename, bool& versionUpdated)
{
  versionUpdated = false;

  Json::Value state;
  if (!_cozmoContext->GetDataPlatform()->readAsJson(kPathToSavedStateFile + filename, state))
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
  _needsState._timeLastWritten = time_point<system_clock>(durationSinceEpoch_s);

  if (versionLoaded >= 3)
  {
    const seconds durationSinceEpochLastDisconnect_s(state[kTimeLastDisconnectKey].asLargestInt());
    _needsState._timeLastDisconnect = time_point<system_clock>(durationSinceEpochLastDisconnect_s);

    const seconds durationSinceEopchLastAppBackgrounded_s(state[kTimeLastAppBackgroundedKey].asLargestInt());
    _needsState._timeLastAppBackgrounded = time_point<system_clock>(durationSinceEopchLastAppBackgrounded_s);

    _needsState._timesOpenedSinceLastDisconnect = state[kOpenAppAfterDisconnectKey].asInt();
  }
  else
  {
    // For older versions, sensible defaults
    _needsState._timeLastDisconnect = Time();
    _needsState._timeLastAppBackgrounded = Time();
    _needsState._timesOpenedSinceLastDisconnect = 0;
  }

  _needsState._robotSerialNumber = state[kSerialNumberKey].asUInt();

  _needsState._curNeedsUnlockLevel = state[kCurNeedsUnlockLevelKey].asInt();
  _needsState._numStarsAwarded = state[kNumStarsAwardedKey].asInt();
  _needsState._numStarsForNextUnlock = state[kNumStarsForNextUnlockKey].asInt();

  for (auto& need : _needsState._curNeedsLevels)
  {
    const int levelAsInt = state[kCurNeedLevelKey][EnumToString(need.first)].asInt();
    need.second = (Util::numeric_cast<float>(levelAsInt) / kNeedLevelStorageMultiplier);
  }
  for (auto& part : _needsState._partIsDamaged)
  {
    part.second = state[kPartIsDamagedKey][EnumToString(part.first)].asBool();
  }

  if (versionLoaded >= 2)
  {
    const seconds durationSinceEpochLastStar_s(state[kTimeLastStarAwardedKey].asLargestInt());
    _needsState._timeLastStarAwarded = time_point<system_clock>(durationSinceEpochLastStar_s);
  }
  else
  {
    _needsState._timeLastStarAwarded = Time(); // For older versions, a sensible default
  }

  if (versionLoaded >= 4)
  {
    _needsState._forceNextSong = UnlockIdFromString(state[kForceNextSongKey].asString());
  }
  else
  {
    _needsState._forceNextSong = UnlockId::Invalid;
  }

  if (versionLoaded >= 5)
  {
    const seconds durationSinceEpochCreated_s(state[kTimeCreatedKey].asLargestInt());
    _needsState._timeCreated = time_point<system_clock>(durationSinceEpochCreated_s);
  }
  else
  {
    // For older versions, sensible default
    _needsState._timeCreated = Time();
  }

  if (versionLoaded < NeedsState::kDeviceStorageVersion)
  {
    versionUpdated = true;
  }

  _needsState.SetNeedsBracketsDirty();
  _needsState.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  return true;
}


std::string NeedsManager::NeedsFilenameFromSerialNumber(const u32 serialNumber)
{
  const std::string filename = kNeedsStateFileBase + "_" +
                               std::to_string(_needsState._robotSerialNumber) +
                               ".json";

  return filename;
}


void NeedsManager::PossiblyStartWriteToRobot(bool ignoreCooldown /* = false */)
{
  if (_robot == nullptr)
    return;

  const Time now = system_clock::now();
  const auto elapsed = now - _timeLastWrittenToRobot;
  const auto secsSinceLastSave = duration_cast<seconds>(elapsed).count();
  if (ignoreCooldown || secsSinceLastSave > kMinimumTimeBetweenRobotSaves_sec)
  {
    StartWriteToRobot(now);
  }
}

void NeedsManager::StartWriteToRobot(const Time time)
{
  if (_robot == nullptr)
  {
    return;
  }

  // If we're reading from the robot, don't start a write operation, because
  // the data we're writing will soon be replaced.  This can happen, e.g.,
  // when robot has just connected (and thus we start reading), and user
  // backgrounds the app
  if (_pendingReadFromRobot)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot",
                  "Aborting writing needs state to robot, because we are reading needs state from robot");
    return;
  }

  _timeLastWrittenToRobot = time;

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.StartWriteToRobot", "Writing to robot...");
  const auto startTime = system_clock::now();

  const auto time_s = duration_cast<seconds>(_timeLastWrittenToRobot.time_since_epoch()).count();
  const auto timeLastWritten = Util::numeric_cast<uint64_t>(time_s);

  std::array<int32_t, MAX_NEEDS> curNeedLevels{};
  for (const auto& need : _needsState._curNeedsLevels)
  {
    const int32_t levelAsInt = Util::numeric_cast<int>((need.second * kNeedLevelStorageMultiplier) + 0.5f);
    curNeedLevels[(int)need.first] = levelAsInt;
  }

  std::array<bool, MAX_REPAIRABLE_PARTS> partIsDamaged{};
  for (const auto& part : _needsState._partIsDamaged)
  {
    partIsDamaged[(int)part.first] = part.second;
  }

  const auto timeLastStarAwarded_s = duration_cast<seconds>(_needsState._timeLastStarAwarded.time_since_epoch()).count();
  const auto timeLastStarAwarded = Util::numeric_cast<uint64_t>(timeLastStarAwarded_s);

  const auto timeCreated_s = duration_cast<seconds>(_needsState._timeCreated.time_since_epoch()).count();
  const auto timeCreated = Util::numeric_cast<uint64_t>(timeCreated_s);

  NeedsStateOnRobot stateForRobot(NeedsState::kRobotStorageVersion, timeLastWritten, curNeedLevels,
                                  _needsState._curNeedsUnlockLevel, _needsState._numStarsAwarded,
                                  partIsDamaged, timeLastStarAwarded, _robotOnboardingStageCompleted,
                                  _needsState._forceNextSong, timeCreated);

  std::vector<u8> stateVec(stateForRobot.Size());
  stateForRobot.Pack(stateVec.data(), stateForRobot.Size());
  if (!_robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_NurtureGameData, stateVec.data(), stateVec.size(),
                                           [this, startTime](NVStorage::NVResult res)
                                           {
                                             FinishWriteToRobot(res, startTime);
                                           }))
  {
    PRINT_NAMED_ERROR("NeedsManager.StartWriteToRobot.WriteFailed", "Write failed");
  }
}


void NeedsManager::FinishWriteToRobot(const NVStorage::NVResult res, const Time startTime)
{
#if ENABLE_NEEDS_READ_WRITE_PROFILING
  const auto endTime = system_clock::now();
  const auto microsecs = duration_cast<microseconds>(endTime - startTime);
  PRINT_CH_INFO(kLogChannelName, "NeedsManager.FinishWriteToRobot", "Write to robot AFTER CALLBACK took %lld microseconds", microsecs.count());
#endif

  if (res < NVStorage::NVResult::NV_OKAY)
  {
    PRINT_NAMED_ERROR("NeedsManager.FinishWriteToRobot.WriteFailed", "Write failed with %s", EnumToString(res));
  }
}


bool NeedsManager::StartReadFromRobot()
{
  if (!_robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_NurtureGameData,
                                          [this](u8* data, size_t size, NVStorage::NVResult res)
                                          {
                                            _pendingReadFromRobot = false;

                                            _robotHadValidNeedsData = FinishReadFromRobot(data, size, res);

                                            InitAfterReadFromRobotAttempt();
                                          }))
  {
    PRINT_NAMED_ERROR("NeedsManager.StartReadFromRobot.ReadFailed", "Failed to start read of needs system robot storage");

    _pendingReadFromRobot = false;
    return false;
  }

  return true;
}


bool NeedsManager::FinishReadFromRobot(const u8* data, const size_t size, const NVStorage::NVResult res)
{
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
        break;
      }
      case 2:
      {
        NeedsStateOnRobot_v02 stateOnRobot_v02;
        stateOnRobot_v02.Unpack(data, size);
        
        stateOnRobot.version = NeedsState::kRobotStorageVersion;
        stateOnRobot.timeLastWritten = stateOnRobot_v02.timeLastWritten;
        for (int i = 0; i < MAX_NEEDS; i++)
          stateOnRobot.curNeedLevel[i] = stateOnRobot_v02.curNeedLevel[i];
        stateOnRobot.curNeedsUnlockLevel = stateOnRobot_v02.curNeedsUnlockLevel;
        stateOnRobot.numStarsAwarded = stateOnRobot_v02.numStarsAwarded;
        for (int i = 0; i < MAX_REPAIRABLE_PARTS; i++)
          stateOnRobot.partIsDamaged[i] = stateOnRobot_v02.partIsDamaged[i];
        stateOnRobot.timeLastStarAwarded = stateOnRobot_v02.timeLastStarAwarded;
        break;
      }
      case 3:
      {
        NeedsStateOnRobot_v03 stateOnRobot_v03;
        stateOnRobot_v03.Unpack(data, size);

        stateOnRobot.version = NeedsState::kRobotStorageVersion;
        stateOnRobot.timeLastWritten = stateOnRobot_v03.timeLastWritten;
        for (int i = 0; i < MAX_NEEDS; i++)
          stateOnRobot.curNeedLevel[i] = stateOnRobot_v03.curNeedLevel[i];
        stateOnRobot.curNeedsUnlockLevel = stateOnRobot_v03.curNeedsUnlockLevel;
        stateOnRobot.numStarsAwarded = stateOnRobot_v03.numStarsAwarded;
        for (int i = 0; i < MAX_REPAIRABLE_PARTS; i++)
          stateOnRobot.partIsDamaged[i] = stateOnRobot_v03.partIsDamaged[i];
        stateOnRobot.timeLastStarAwarded = stateOnRobot_v03.timeLastStarAwarded;
        stateOnRobot.onboardingStageCompleted = stateOnRobot_v03.onboardingStageCompleted;
        break;
      }
      case 4: // (First version shipped to the wild, in Cozmo 2.0.0)
      {
        NeedsStateOnRobot_v04 stateOnRobot_v04;
        stateOnRobot_v04.Unpack(data, size);

        stateOnRobot.version = NeedsState::kRobotStorageVersion;
        stateOnRobot.timeLastWritten = stateOnRobot_v04.timeLastWritten;
        for (int i = 0; i < MAX_NEEDS; i++)
          stateOnRobot.curNeedLevel[i] = stateOnRobot_v04.curNeedLevel[i];
        stateOnRobot.curNeedsUnlockLevel = stateOnRobot_v04.curNeedsUnlockLevel;
        stateOnRobot.numStarsAwarded = stateOnRobot_v04.numStarsAwarded;
        for (int i = 0; i < MAX_REPAIRABLE_PARTS; i++)
          stateOnRobot.partIsDamaged[i] = stateOnRobot_v04.partIsDamaged[i];
        stateOnRobot.timeLastStarAwarded = stateOnRobot_v04.timeLastStarAwarded;
        stateOnRobot.onboardingStageCompleted = stateOnRobot_v04.onboardingStageCompleted;
        stateOnRobot.forceNextSong = stateOnRobot_v04.forceNextSong;
        break;
      }
      default:
      {
        PRINT_CH_DEBUG(kLogChannelName, "NeedsManager.FinishReadFromRobot.UnsupportedOldRobotStorageVersion",
                       "Version %d found on robot but not supported", versionLoaded);
        break;
      }
    }
    if (versionLoaded < 2)
    {
      // Version 2 added this variable:
      stateOnRobot.timeLastStarAwarded = 0;
    }
    if (versionLoaded < 3)
    {
      // Version 3 added this variable
      stateOnRobot.onboardingStageCompleted = 0;
    }
    if (versionLoaded < 4)
    {
      // Version 4 added this variable
      stateOnRobot.forceNextSong = UnlockId::Invalid;
    }
    if (versionLoaded < 5)
    {
      // Version 5 added this variable
      stateOnRobot.timeCreated = 0;
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

  for (int i = 0; i < RepairablePartIdNumEntries; i++)
  {
    const auto& pardId = static_cast<RepairablePartId>(i);
    _needsStateFromRobot._partIsDamaged[pardId] = stateOnRobot.partIsDamaged[i];
  }

  const seconds durationSinceEpochLastStar_s(stateOnRobot.timeLastStarAwarded);
  _needsStateFromRobot._timeLastStarAwarded = time_point<system_clock>(durationSinceEpochLastStar_s);

  _needsStateFromRobot._forceNextSong = stateOnRobot.forceNextSong;

  const seconds durationSinceEpochCreated_s(stateOnRobot.timeCreated);
  _needsStateFromRobot._timeCreated = time_point<system_clock>(durationSinceEpochCreated_s);

  // Other initialization for things that do not come from storage:
  _needsStateFromRobot._robotSerialNumber = _robot->GetBodySerialNumber();
  _needsStateFromRobot._needsConfig = &_needsConfig;
  _needsStateFromRobot._starRewardsConfig = _starRewardsConfig;
  _needsStateFromRobot._rng = _cozmoContext->GetRandom();
  _needsStateFromRobot.SetNeedsBracketsDirty();
  _needsStateFromRobot.UpdateCurNeedsBrackets(_needsConfig._needsBrackets);

  _robotOnboardingStageCompleted = stateOnRobot.onboardingStageCompleted;
  
  return true;
}

#if ANKI_DEV_CHEATS
void NeedsManager::DebugFillNeedMeters()
{
  _needsState.SetPrevNeedsBrackets();

  _needsState.DebugFillNeedMeters();
  SendNeedsStateToGame();
  if (_robot != nullptr)
  {
    UpdateStarsState();
  }
}

void NeedsManager::DebugGiveStar()
{
  if (_robot != nullptr)
  {
    PRINT_CH_INFO(kLogChannelName, "NeedsManager.DebugGiveStar","");
    DebugCompleteDay();
    UpdateStarsState(true);
  }
}

void NeedsManager::DebugCompleteDay()
{
  // Push the last given star back 24 hours
  system_clock::time_point now = system_clock::now();
  std::time_t yesterdayTime = system_clock::to_time_t(now - hours(25));
  _needsState._timeLastStarAwarded = system_clock::from_time_t(yesterdayTime);

  PRINT_CH_INFO(kLogChannelName, "NeedsManager.DebugCompleteDay","");
}

void NeedsManager::DebugResetNeeds()
{
  if (_robot != nullptr)
  {
    _needsState.Init(_needsConfig, _robot->GetBodySerialNumber(), _starRewardsConfig, _cozmoContext->GetRandom());
    _robotHadValidNeedsData = false;
    _deviceHadValidNeedsData = false;
    InitAfterReadFromRobotAttempt();
    SendNeedsStateToGame();
  }
}

void NeedsManager::DebugCompleteAction(const char* actionName)
{
  const NeedsActionId actionId = NeedsActionIdFromString(actionName);
  RegisterNeedsActionCompleted(actionId);
}

void NeedsManager::DebugPredictActionResult(const char* actionName)
{
  const NeedsActionId actionId = NeedsActionIdFromString(actionName);
  NeedsState state;
  PredictNeedsActionResult(actionId, state);
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
  _needsState.SetPrevNeedsBrackets();

  const float delta = level - _needsState._curNeedsLevels[needId];

  if ((needId == NeedId::Repair) && (delta > 0.0f))
  {
    // For the repair need, if we're going UP, we also need to repair enough
    // parts as needed so that the new level will be within the correct
    // threshold for 'number of broken parts'.
    // We don't need to do this when going DOWN because ApplyDelta will
    // break parts for us.
    int numDamagedParts = _needsState.NumDamagedParts();
    int newNumDamagedParts = _needsState.NumDamagedPartsForRepairLevel(level);
    while (newNumDamagedParts < numDamagedParts)
    {
      _needsState._partIsDamaged[_needsState.PickPartToRepair()] = false;
      numDamagedParts--;
    }
  }

  NeedDelta needDelta(delta, 0.0f, NeedsActionId::NoAction);
  if (_needsState.ApplyDelta(needId, needDelta, NeedsActionId::NoAction))
  {
    StartFullnessCooldownForNeed(needId);
  }

  SendNeedsStateToGame();

  // Don't award daily stars when no robot connected, because that could lead
  // to unlocks which have to be stored on the robot
  if (_robot != nullptr)
  {
    UpdateStarsState();
  }

  DetectBracketChangeForDas();

  PossiblyWriteToDevice();
  PossiblyStartWriteToRobot();

  _localNotifications->Generate();
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
    if (_timeWhenCooldownOver_s[needIndex] != 0.0f)
    {
      _timeWhenCooldownOver_s[needIndex] -= timeElaspsed_s;
      _timeWhenCooldownStarted_s[needIndex] -= timeElaspsed_s;
    }
  }

  const bool connected = _robot != nullptr;
  ApplyDecayAllNeeds(connected);

  SendNeedsStateToGame(NeedsActionId::Decay);

  WriteToDevice();
}
#endif

} // namespace Cozmo
} // namespace Anki

