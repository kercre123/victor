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
  
  void Init(const Json::Value& inJson);
  void InitAfterConnection();
  
  void Update(const float currentTime_s);
  
  void SetPaused(bool paused);
  bool GetPaused() const { return _isPaused; };
  
  const NeedsState& GetCurNeedsState() const { return _needsState; };
  
  void RegisterNeedsActionCompleted(NeedsActionId actionCompleted);

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

  void HandleMfgID(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
  bool DeviceHasNeedsState();
  void WriteToDevice(const NeedsState& needsState);
  bool ReadFromDevice(NeedsState& needsState, bool& versionUpdated);

  static inline const std::string GetNurtureFolder() { return "nurture/"; }

  void StartWriteToRobot(const NeedsState& needsState);
  void FinishWriteToRobot(NVStorage::NVResult res);
  bool StartReadFromRobot();
  void FinishReadFromRobot(u8* data, size_t size, NVStorage::NVResult res);

  static const char* kLogChannelName;

private:

  void SendNeedsStateToGame(NeedsActionId actionCausingTheUpdate);
  void SendNeedsPauseStateToGame();
  void SendAllNeedsMetToGame();
  
  Robot&      _robot;

  NeedsState  _needsState;
  NeedsConfig _needsConfig;
  NeedsState  _needsStateFromRobot; // Not used yet but may when I implement 'resolve on connect'

  bool        _isPaused;

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

