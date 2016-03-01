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

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "util/math/math.h"

#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
  
class BehaviorLookAround : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorLookAround(Robot& robot, const Json::Value& config);
  
public:

  virtual ~BehaviorLookAround() override;
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;

  void SetLookAroundHeadAngle(float angle_rads) { _lookAroundHeadAngle_rads = angle_rads; }
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
  virtual void   StopInternal(Robot& robot, double currentTime_sec) override;

  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  enum class State {
    WaitForOtherActions,
    Inactive,
    Roaming,
    LookingAtPossibleObject,
    ExaminingFoundObject
  };

  void TransitionToWaitForOtherActions(Robot& robot);
  void TransitionToInactive(Robot& robot);
  void TransitionToRoaming(Robot& robot);
  void TransitionToLookingAtPossibleObject(Robot& robot);
  void TransitionToExaminingFoundObject(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);

  enum class Destination {
    North = 0,
    West,
    South,
    East,
    Center,
    
    Count
  };

  static const char* DestinationToString(const Destination& dest);
  
  // How long to wait before we trying to look around again (among other factors)
  constexpr static f32 kLookAroundCooldownDuration = 7;
  // How fast to rotate when looking around
  constexpr static f32 kDegreesRotatePerSec = 25;
  // The default radius (in mm) we assume exists for us to move around in
  constexpr static f32 kDefaultSafeRadius = 150;
  // Number of destinations we want to reach before resting for a bit (needs to be at least 2)
  constexpr static u32 kDestinationsToReach = 6;
  // How far back from a possible object to observe it (at most)
  constexpr static f32 kMaxObservationDistanceSq_mm = SQUARE(200.0f);
  // If the possible block is too far, this is the distance to view it from
  constexpr static f32 kPossibleObjectViewingDist_mm = 100.0f;
  // How long to wait to try to see a possible object (might do this 3 times)
  constexpr static f32 kPossibleObjectWaitTime_s = 0.5f;
  
  State _currentState = State::Inactive;
  Destination _currentDestination = Destination::North;
  f32 _lastLookAroundTime = 0;
  Pose3d _moveAreaCenter;
  f32 _safeRadius = kDefaultSafeRadius;
  u32 _currentDriveActionID = 0;
  u32 _numDestinationsLeft = kDestinationsToReach;
  f32 _lookAroundHeadAngle_rads = DEG_TO_RAD(-5);
  
  std::set<ObjectID> _recentObjects;
  std::set<ObjectID> _oldBoringObjects;
  Pose3d _lastPossibleObjectPose;
  
  Pose3d GetDestinationPose(Destination destination);
  void ResetBehavior(Robot& robot, float currentTime_sec);
  Destination GetNextDestination(Destination current);
  void UpdateSafeRegion(const Vec3f& objectPosition);
  void ResetSafeRegion(Robot& robot);

  void HandleObjectObserved(const ExternalInterface::RobotObservedObject& msg, bool confirmed, Robot& robot);
  void HandleRobotPutDown(const EngineToGameEvent& event, Robot& robot);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
