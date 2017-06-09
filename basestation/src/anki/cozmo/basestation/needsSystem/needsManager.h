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
#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/global/globalDefinitions.h" // ANKI_DEV_CHEATS define
#include "util/signals/simpleSignal_fwd.h"
#include <assert.h>
#include <chrono>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
  class MessageGameToEngine;
}
namespace RobotInterface {
  class MessageHandler;
}

class Robot;

enum class RobotStorageState
{
  Inactive = 0,
  Reading,
  Writing
};


class NeedsManager
{
public:
  explicit NeedsManager(const CozmoContext* cozmoContext);
  ~NeedsManager();

  void Init(const float currentTime_s, const Json::Value& inJson,
            const Json::Value& inStarsJson, const Json::Value& inActionsJson,
            const Json::Value& inDecayJson);
  void InitAfterConnection();
  void InitAfterSerialNumberAcquired(u32 serialNumber);

  void OnRobotDisconnected();

  void Update(const float currentTime_s);

  void SetPaused(const bool paused);
  bool GetPaused() const { return _isPausedOverall; };

  NeedsState& GetCurNeedsStateMutable();
  const NeedsState& GetCurNeedsState();

  void RegisterNeedsActionCompleted(const NeedsActionId actionCompleted);

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
  void DebugPauseDecayForNeed(const char* needName);
  void DebugPauseActionsForNeed(const char* needName);
  void DebugUnpauseDecayForNeed(const char* needName);
  void DebugUnpauseActionsForNeed(const char* needName);
  void DebugImplPausing(const char* needName, const bool isDecay, const bool isPaused);
  void DebugSetNeedLevel(const NeedId needId, const float level);
  void DebugPassTimeMinutes(const float minutes);
#endif

private:

  bool DeviceHasNeedsState();
  void PossiblyWriteToDevice(NeedsState& needsState);
  void WriteToDevice(const NeedsState& needsState);
  bool ReadFromDevice(NeedsState& needsState, bool& versionUpdated);

  static inline const std::string GetNurtureFolder() { return "nurture/"; }

  void PossiblyStartWriteToRobot(NeedsState& needsState, bool ignoreCooldown = false);
  void StartWriteToRobot(const NeedsState& needsState);
  void FinishWriteToRobot(const NVStorage::NVResult res, const Time startTime);
  bool StartReadFromRobot();
  bool FinishReadFromRobot(const u8* data, const size_t size, const NVStorage::NVResult res);

  void InitAfterReadFromRobotAttempt();

  void ApplyDecayAllNeeds();

  void UpdateStarsState();

  void SendNeedsStateToGame(const NeedsActionId actionCausingTheUpdate = NeedsActionId::NoAction);
  void SendNeedsPauseStateToGame();
  void SendNeedsPauseStatesToGame();
  void SendStarLevelCompletedToGame();
  void SendSingleStarAddedToGame();

  const CozmoContext* _cozmoContext;
  Robot*        _robot;

  NeedsState    _needsState;
  NeedsState    _needsStateFromRobot;
  
  NeedsConfig   _needsConfig;
  ActionsConfig _actionsConfig;
  std::shared_ptr<StarRewardsConfig> _starRewardsConfig;

  Time          _savedTimeLastWrittenToDevice;
  Time          _timeLastWrittenToRobot;
  bool          _robotHadValidNeedsData;
  bool          _deviceHadValidNeedsData;
  bool          _robotNeedsVersionUpdate;
  bool          _deviceNeedsVersionUpdate;

  bool          _isPausedOverall;
  float         _timeWhenPausedOverall_s;

  std::array<bool, static_cast<size_t>(NeedId::Count)> _isDecayPausedForNeed;
  std::array<bool, static_cast<size_t>(NeedId::Count)> _isActionsPausedForNeed;

  std::array<float, static_cast<size_t>(NeedId::Count)> _lastDecayUpdateTime_s;
  std::array<float, static_cast<size_t>(NeedId::Count)> _timeWhenPaused_s;

  std::array<std::vector<NeedDelta>, static_cast<size_t>(NeedId::Count)> _queuedNeedDeltas;

  float         _currentTime_s;
  float         _timeForNextPeriodicDecay_s;
  float         _pausedDurRemainingPeriodicDecay;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  const std::string kPathToSavedStateFile;

  RobotStorageState _robotStorageState = RobotStorageState::Inactive;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

