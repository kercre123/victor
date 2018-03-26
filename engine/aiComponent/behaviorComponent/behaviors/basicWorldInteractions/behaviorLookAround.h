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

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
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
  
class BehaviorLookAround : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorLookAround(const Json::Value& config);
  
public:
  virtual ~BehaviorLookAround() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  void SetLookAroundHeadAngle(float angle_rads) { _dVars.lookAroundHeadAngle_rads = angle_rads; }
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

private:
  enum class State {
    WaitForOtherActions,
    Inactive,
    Roaming,
    LookingAtPossibleObject,
    ExaminingFoundObject
  };
  enum class Destination {
    North = 0,
    West,
    South,
    East,
    Center,
    
    Count
  };

  struct InstanceConfig {
    InstanceConfig();
    bool shouldHandleConfirmedObjectOverved;
    bool shouldHandlePossibleObjectOverved;
  };

  struct DynamicVariables {
    DynamicVariables();
    State       currentState;
    Destination currentDestination;
    
    // note that this is reset when the robot is put down, so no need to worry about origins
    Pose3d moveAreaCenter;
    f32    safeRadius;
    u32    numDestinationsLeft;
    f32    lookAroundHeadAngle_rads;

    std::set<ObjectID> recentObjects;
    std::set<ObjectID> oldBoringObjects;
    Pose3d             lastPossibleObjectPose;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void SetState_internal(State state, const std::string& stateName);

  void TransitionToWaitForOtherActions();
  void TransitionToInactive();
  void TransitionToRoaming();
  void TransitionToLookingAtPossibleObject();
  void TransitionToExaminingFoundObject();

  static const char* DestinationToString(const Destination& dest);
  
  Pose3d GetDestinationPose(Destination destination);
  Destination GetNextDestination(Destination current);

  void ResetSafeRegion();

  // this function may extend the safe region, since we know that if a cube can rest there, we probably can as
  // well
  void UpdateSafeRegionForCube(const Vec3f& objectPosition);

  // This version may shrink the safe region, and/or move it away from the position of the cliff
  void UpdateSafeRegionForCliff(const Pose3d& objectPose);

  void HandleObjectObserved(const ExternalInterface::RobotObservedObject& msg, bool confirmed);
  void HandleRobotOfftreadsStateChanged(const EngineToGameEvent& event);
  void HandleCliffEvent(const EngineToGameEvent& event);

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
