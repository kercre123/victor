/**
 * File: behaviorFindFaces.h
 *
 * Author: Lee Crippen
 * Created: 12/22/15
 *
 * Description: Behavior for rotating around to find faces.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface { class MessageEngineToGame; }
  
class BehaviorFindFaces : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindFaces(Robot& robot, const Json::Value& config);
  
public:

  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  enum class State {
    Inactive,
    StartMoving,
    WaitToFinishMoving,
    PauseToSee
  };
  
  constexpr static float kPanMin = 25;
  
  constexpr static float kPanMax = 80;
  
  constexpr static float kTiltMin = 17;
  
  constexpr static float kTiltMax = 23;
  
  constexpr static float kPauseMin = 1;
  
  constexpr static float kPauseMax = 3.5;
  
  State _currentState = State::Inactive;
  uint32_t _currentDriveActionID; // Init in cpp to not have constants include in header
  double _lookPauseTimer = 0;
  
  void StartMoving(Robot& robot);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
