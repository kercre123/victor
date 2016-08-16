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
#include "anki/common/shared/radians.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface { class MessageEngineToGame; }
  
class BehaviorFindFaces : public IBehavior
{
private:
  using super = IBehavior;
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindFaces(Robot& robot, const Json::Value& config);
  
public:

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual float EvaluateScoreInternal(const Anki::Cozmo::Robot &robot) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;

  virtual float EvaluateRunningScoreInternal(const Robot& robot) const override;
  
private:
  enum class State {
    Inactive,
    StartMoving,
    WaitToFinishMoving,
    PauseToSee
  };
  
  // Min angle to move relative to current, in degrees
  constexpr static float kPanMin = 35;
  // Max angle to move relative to current, in degrees
  constexpr static float kPanMax = 80;
  // Min absolute angle to move head to, in degrees
  constexpr static float kTiltMin = 25;
  // Max absolute angle to move head to, in degrees
  constexpr static float kTiltMax = 42;
  // Width of zone to focus on
  constexpr static float kFocusAreaAngle_deg = 120;
  
  // The length of time in seconds it has to have been since we last
  // saw a face in order to enter this behavior
  double _minimumTimeSinceSeenLastFace_sec;

  // Min time to pause after moving
  float _pauseMin_s;
  // Max time to pause after moving
  float _pauseMax_s;

  State _currentState = State::Inactive;
  uint32_t _currentDriveActionID; // Init in cpp to not have constants include in header
  double _lookPauseTimer = 0;
  Radians _faceAngleCenter;
  bool _faceAngleCenterSet = false;
  bool _useFaceAngleCenter = true;
  float _minScoreWhileActive = 0.0f;
  bool _stopOnAnyFace = false;
  
  float GetRandomPanAmount() const;
  void StartMoving(Robot& robot);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
