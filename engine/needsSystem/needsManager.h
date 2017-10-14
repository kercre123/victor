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


#ifndef __Cozmo_Basestation_NeedsSystem_NeedsManager_H__
#define __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

#include "anki/common/basestation/utils/timer.h"
#include "anki/common/types.h"
#include "engine/needsSystem/needsConfig.h"
#include "engine/needsSystem/needsState.h"
#include "engine/needsSystem/localNotifications.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/global/globalDefinitions.h"
#include "util/signals/simpleSignal_fwd.h"
#include <assert.h>
#include <chrono>
#include <memory>
#include <set>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
  class MessageGameToEngine;
}
namespace RobotInterface {
  class MessageHandler;
}

#if ANKI_PROFILING_ENABLED
  #define ENABLE_NEEDS_READ_WRITE_PROFILING 1
#else
  #define ENABLE_NEEDS_READ_WRITE_PROFILING 0
#endif

class Robot;
class CozmoContext;
class DesiredFaceDistortionComponent;
class LocalNotifications;

class NeedsManager
{
public:
  explicit NeedsManager(const CozmoContext* cozmoContext);
  ~NeedsManager();

  void Init(const float currentTime_s,      const Json::Value& inJson,
            const Json::Value& inStarsJson, const Json::Value& inActionsJson,
            const Json::Value& inDecayJson, const Json::Value& inHandlersJson,
            const Json::Value& inLocalNotificationJson);
  void InitAfterConnection();
  void InitAfterSerialNumberAcquired(u32 serialNumber);

  void OnRobotDisconnected();

  void Update(const float currentTime_s);

  void SetPaused(const bool paused);
  bool GetPaused() const { return _isPausedOverall; };

  float GetCurrentTime() const { return _currentTime_s; };

  NeedsState& GetCurNeedsStateMutable();
  const NeedsState& GetCurNeedsState();

  NeedsConfig& GetNeedsConfigMutable() { return _needsConfig; };
  const NeedsConfig& GetNeedsConfig() const { return _needsConfig; };

  void RegisterNeedsActionCompleted(const NeedsActionId actionCompleted);
  void PredictNeedsActionResult(const NeedsActionId actionCompleted, NeedsState& outNeedsState);

  bool GetConnectionOccurredThisAppRun() const { return _connectionOccurredThisAppRun; };

  // Needs "handlers". Currently the only one is the eye glitch based on repair need:
  DesiredFaceDistortionComponent& GetDesiredFaceDistortionComponent() {
    assert(_faceDistortionComponent);
    return *_faceDistortionComponent;
  }
  const DesiredFaceDistortionComponent& GetDesiredFaceDistortionComponent() const {
    assert(_faceDistortionComponent);
    return *_faceDistortionComponent;
  }

  bool IsPendingSparksRewardMsg() const { return _pendingSparksRewardMsg; }
  void SparksRewardCommunicatedToUser();

  static inline const std::string GetDecayConfigBaseFilename() { return "config/engine/needs_decay_config"; }
  static inline const std::string GetConfigBaseFilename() { return "config/engine/needs_config"; }

  static const char* kLogChannelName;

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

#if ANKI_DEV_CHEATS
  void DebugFillNeedMeters();
  void DebugGiveStar();
  void DebugCompleteDay();
  void DebugResetNeeds();
  void DebugCompleteAction(const char* actionName);
  void DebugPredictActionResult(const char* actionName);
  void DebugPauseDecayForNeed(const char* needName);
  void DebugPauseActionsForNeed(const char* needName);
  void DebugUnpauseDecayForNeed(const char* needName);
  void DebugUnpauseActionsForNeed(const char* needName);
  void DebugImplPausing(const char* needName, const bool isDecay, const bool isPaused);
  void DebugSetNeedLevel(const NeedId needId, const float level);
  void DebugPassTimeMinutes(const float minutes);
#endif

private:

  void InitReset(const float currentTime_s, const u32 serialNumber);
  void InitInternal(const float currentTime_s);

  bool DeviceHasNeedsState(const std::string& filename);
  void PossiblyWriteToDevice();
  void WriteToDevice(bool stampWithNowTime = true);
  bool AttemptReadFromDevice(const std::string& filename, bool& versionUpdated);
  bool ReadFromDevice(const std::string& filename, bool& versionUpdated);
  std::string NeedsFilenameFromSerialNumber(const u32 serialNumber);

  static inline const std::string GetNurtureFolder() { return "nurture/"; }

  void PossiblyStartWriteToRobot(bool ignoreCooldown = false);
  void StartWriteToRobot(const Time time);
  void FinishWriteToRobot(const NVStorage::NVResult res, const Time startTime);
  bool StartReadFromRobot();
  bool FinishReadFromRobot(const u8* data, const size_t size, const NVStorage::NVResult res);

  void InitAfterReadFromRobotAttempt();

  void AttemptActivateDecayExperiment(u32 robotSerialNumber);
  void AttemptActivateTuningExperiment(u32 robotSerialNumber);

  void ApplyDecayAllNeeds(const bool connected);
  void ApplyDecayForTimeSinceLastDeviceWrite(const bool connected);
  void StartFullnessCooldownForNeed(const NeedId needId);

  void RegisterNeedsActionCompletedInternal(const NeedsActionId actionCompleted,
                                            NeedsState& needsState,
                                            const bool predictionOnly);

  bool ShouldRewardSparksForFreeplay();
  int  RewardSparksForFreeplay();
  int  AwardSparks(const int targetSparks, const float minPct, const float maxPct,
                   const int minSparks, const int minMaxSparks);

  bool UpdateStarsState(const bool cheatGiveStar = false);

  void DetectBracketChangeForDas(const bool forceSend = false);
  void SendRepairDasEvent(const NeedsState& needsState,
                          const NeedsActionId cause,
                          const RepairablePartId part);

  void FormatStringOldAndNewLevels(std::ostringstream& stream,
                                   NeedsState::CurNeedsMap& prevNeedsLevels);

  void SendNeedsLevelsDasEvent(const char * whenTag);
  void SendTimeSinceBackgroundedDasEvent();

  void SendNeedsStateToGame(const NeedsActionId actionCausingTheUpdate = NeedsActionId::NoAction);
  void SendNeedsPauseStateToGame();
  void SendNeedsPauseStatesToGame();
  void SendStarLevelCompletedToGame();
  void SendStarUnlockedToGame();
  void SendNeedsOnboardingToGame();
  void SendNeedsDebugVizString(const NeedsActionId actionCausingTheUpdate);

  void ProcessLevelRewards(const int level, std::vector<NeedsReward>& rewards,
                           const bool unlocksOnly = false, int* allowedPriorUnlocks = nullptr);

  const CozmoContext* _cozmoContext;
  Robot*        _robot;

  NeedsState    _needsState;
  NeedsState    _needsStateFromRobot;
  
  NeedsConfig   _needsConfig;
  ActionsConfig _actionsConfig;
  std::shared_ptr<StarRewardsConfig>  _starRewardsConfig;
  std::shared_ptr<LocalNotifications> _localNotifications;

  Time          _savedTimeLastWrittenToDevice;
  Time          _timeLastWrittenToRobot;
  bool          _robotHadValidNeedsData;
  bool          _deviceHadValidNeedsData;
  bool          _robotNeedsVersionUpdate;
  bool          _deviceNeedsVersionUpdate;
  u32           _previousRobotSerialNumber;
  int           _robotOnboardingStageCompleted;
  bool          _connectionOccurredThisAppRun;

  bool          _isPausedOverall;
  float         _timeWhenPausedOverall_s;

  std::array<bool, static_cast<size_t>(NeedId::Count)> _isDecayPausedForNeed;
  std::array<bool, static_cast<size_t>(NeedId::Count)> _isActionsPausedForNeed;

  std::array<float, static_cast<size_t>(NeedId::Count)> _lastDecayUpdateTime_s;
  std::array<float, static_cast<size_t>(NeedId::Count)> _timeWhenPaused_s;
  std::array<float, static_cast<size_t>(NeedId::Count)> _timeWhenCooldownStarted_s;
  std::array<float, static_cast<size_t>(NeedId::Count)> _timeWhenCooldownOver_s;
  std::array<float, static_cast<size_t>(NeedId::Count)> _timeWhenBracketChanged_s;

  std::array<std::vector<NeedDelta>, static_cast<size_t>(NeedId::Count)> _queuedNeedDeltas;

  std::array<float, static_cast<size_t>(NeedsActionId::Count)> _actionCooldown_s;
  
  bool          _onlyWhiteListedActionsEnabled;
  std::set<NeedsActionId> _whiteListedActions;

  float         _currentTime_s;
  float         _timeForNextPeriodicDecay_s;
  float         _pausedDurRemainingPeriodicDecay;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  const std::string kPathToSavedStateFile;

  bool          _pendingReadFromRobot = false;

  // component to figure out what the current face distortion level should be
  // This gets instantiated when Init is called
  std::unique_ptr<DesiredFaceDistortionComponent> _faceDistortionComponent;

  bool          _pendingSparksRewardMsg;
  ExternalInterface::FreeplaySparksAwarded _sparksRewardMsg;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

