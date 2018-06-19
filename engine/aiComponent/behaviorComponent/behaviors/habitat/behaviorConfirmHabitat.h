/**
 * File: behaviorConfirmHabitat.h
 *
 * Author: Arjun Menon
 * Date:   06 04 2018
 *
 * Description:
 * behavior that represents the robot's deliberate maneuvers to confirm whether
 *  it is inside the habitat after a pickup and putdown, or driving off charger
 *
 * the robot will engage in discrete actions to get to a configuration on top
 * of the white line so that its front cliff sensors are triggered. At this
 * position, the behavior will take no more actions since we are in the pose
 * to confirm the habitat in.
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;

class BehaviorConfirmHabitat : public ICozmoBehavior
{
protected:

  friend class BehaviorFactory;
  BehaviorConfirmHabitat(const Json::Value& config);
  
public:

  virtual ~BehaviorConfirmHabitat();
  virtual bool WantsToBeActivatedBehavior() const override;
  void InitBehavior() override;
  
protected:

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override
  {
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  }
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  // state action helper methods
  void DelegateActionHelper(IActionRunner* action, RobotCompletedActionCallback&& callback);
  void TransitionToRandomWalk();
  void TransitionToSeekLineFromCharger();
  void TransitionToBackupForward();
  void TransitionToLocalizeCharger();
  
  // helper motions to reposition the robot from certain line positions
  void TransitionToCliffAlignWhite();
  void TransitionToBackupAndTurn(f32 angle);
  void TransitionToTurnBackupForward(f32 angle, int backDist_mm, int forwardDist_mm);
  
  // when we receive a StopOnWhite message, there is a 1-2 tick lag for engine to register
  // that it is seeing a white line beneath it. This is used as a buffer for engine to
  // catch up and update its internal tracking of white
  void TransitionToWaitForWhite();

  // helper methods -- compute a pose (in world origin) relative to a pose
  //  or compute a pose in world origin
  // both functions return true if the operation is possible
  std::pair<bool,Pose3d> GetPoseInWorldOrigin(const Pose3d& pose) const;
  std::pair<bool,Pose3d> GetPoseOffsetFromPose(const Pose3d& basePose, Pose3d offset) const;
  
  // returns nullptr if there is no charger seen
  const ObservableObject* GetChargerIfObserved() const;
  
  // steps in the process of confirming habitat
  enum class ConfirmHabitatPhase
  {
    Initial,
    LocalizeCharger,
    SeekLineFromCharger,
    RandomWalk,
    LineDocking
  };
  
  struct DynamicVariables
  {
    // the current step or phase of habitat confirmation procedure
    ConfirmHabitatPhase _phase = ConfirmHabitatPhase::Initial;
    
    // flag set if the random walk detects a nearby obstacle
    // this will gate choosing certain drive actions, or
    // make the robot take a different maneuver to unblock it
    bool _randomWalkTooCloseObstacle = false;
    
    // internal tracker for angle swept by the robot
    // while searching for the charger
    // TODO(agm) remove this when we can delegate to
    // another behavior
    f32 _lookForChargerAngleSwept_rad = 0.0;

    explicit DynamicVariables();
  };
  
  // persist this filter because we use it to search for the charger
  std::unique_ptr<BlockWorldFilter> _chargerFilter;
  
  DynamicVariables _dVars;
};

}
}

#endif//__Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__
