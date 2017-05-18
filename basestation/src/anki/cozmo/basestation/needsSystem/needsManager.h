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
#include "util/graphEvaluator/graphEvaluator2d.h" // For json; todo find a better way to get it
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
  explicit NeedsManager(Robot& inRobot);
  ~NeedsManager();
  
  void Init(const Json::Value& inJson, const Json::Value& inStarsJson);
  void InitAfterConnection();
  
  void Update(const float currentTime_s);
  
  void SetPaused(bool paused);
  bool GetPaused() const { return _isPaused; };
  
  const NeedsState& GetCurNeedsState() const { return _needsState; };
  
  void RegisterNeedsActionCompleted(NeedsActionId actionCompleted);

  static const char* kLogChannelName;

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
#if ANKI_DEV_CHEATS
  void DebugFillNeedMeters();
  void DebugGiveStar();
  void DebugCompleteDay();
  void DebugResetNeeds();
#endif

private:

  void HandleMfgID(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  bool DeviceHasNeedsState();
  void PossiblyWriteToDevice(NeedsState& needsState);
  void WriteToDevice(const NeedsState& needsState);
  bool ReadFromDevice(NeedsState& needsState, bool& versionUpdated);

  static inline const std::string GetNurtureFolder() { return "nurture/"; }

  void PossiblyStartWriteToRobot(NeedsState& needsState, bool ignoreCooldown = false);
  void StartWriteToRobot(const NeedsState& needsState);
  void FinishWriteToRobot(NVStorage::NVResult res, const Time startTime);
  bool StartReadFromRobot();
  bool FinishReadFromRobot(u8* data, size_t size, NVStorage::NVResult res);

  void InitAfterReadFromRobotAttempt();

private:
  
  void UpdateStarsState();

  void SendNeedsStateToGame(NeedsActionId actionCausingTheUpdate);
  void SendNeedsPauseStateToGame();
  void SendNeedsPauseStatesToGame();
  void SendStarLevelCompletedToGame();
  void SendSingleStarAddedToGame();
  
  Robot&      _robot;

  NeedsState  _needsState;
  NeedsConfig _needsConfig;
  NeedsState  _needsStateFromRobot;
  
  std::shared_ptr<StarRewardsConfig> _starRewardsConfig;

  Time        _timeLastWrittenToRobot;
  bool        _robotHadValidNeedsData;
  bool        _deviceHadValidNeedsData;
  bool        _robotNeedsVersionUpdate;
  bool        _deviceNeedsVersionUpdate;

  bool        _isPaused;

  std::array<bool, static_cast<size_t>(NeedId::Count)> _isDecayPausedForNeed;
  std::array<bool, static_cast<size_t>(NeedId::Count)> _isActionsPausedForNeed;

  float       _currentTime_s;
  float       _timeForNextPeriodicDecay_s;
  float       _pausedDurRemainingPeriodicDecay;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  const std::string kPathToSavedStateFile;

  RobotStorageState _robotStorageState = RobotStorageState::Inactive;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

