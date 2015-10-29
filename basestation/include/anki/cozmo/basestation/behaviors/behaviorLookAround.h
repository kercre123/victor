/**
 * File: behaviorLookAround.h
 *
 * Author: Lee
 * Created: 08/13/15
 *
 * Description: Behavior for looking around the environment for stuff to interact with.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"

#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface { class MessageEngineToGame; }
  
class BehaviorLookAround : public IBehavior
{
public:
  BehaviorLookAround(Robot& robot, const Json::Value& config);
  virtual ~BehaviorLookAround() override;
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;

  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  enum class State {
    Inactive,
    StartLooking,
    LookingForObject,
    ExamineFoundObject,
    WaitToFinishExamining
  };
  
  enum class Destination {
    North = 0,
    West,
    South,
    East,
    Center,
    
    Count
  };
  
  // How long to wait before we trying to look around again (among other factors)
  constexpr static f32 kLookAroundCooldownDuration = 7;
  // How fast to rotate when looking around
  constexpr static f32 kDegreesRotatePerSec = 25;
  // The default radius (in mm) we assume exists for us to move around in
  constexpr static f32 kDefaultSafeRadius = 150;
  // Number of destinations we want to reach before resting for a bit (needs to be at least 2)
  constexpr static u32 kDestinationsToReach = 6;
  
  State _currentState = State::Inactive;
  Destination _currentDestination = Destination::North;
  f32 _lastLookAroundTime = 0;
  Pose3d _moveAreaCenter;
  f32 _safeRadius = kDefaultSafeRadius;
  u32 _currentDriveActionID = 0;
  u32 _numDestinationsLeft = kDestinationsToReach;
  
  std::set<ObjectID> _recentObjects;
  std::set<ObjectID> _oldBoringObjects;
  u32 _numObjectsToLookAt = 0;
  
  Result StartMoving(Robot& robot);
  Pose3d GetDestinationPose(Destination destination);
  void ResetBehavior(Robot& robot, float currentTime_sec);
  Destination GetNextDestination(Destination current);
  void UpdateSafeRegion(const Vec3f& objectPosition);
  void ResetSafeRegion(Robot& robot);
  
  void HandleObjectObserved(const EngineToGameEvent& event, Robot& robot);
  void HandleCompletedAction(const EngineToGameEvent& event);
  void HandleRobotPutDown(const EngineToGameEvent& event, Robot& robot);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
