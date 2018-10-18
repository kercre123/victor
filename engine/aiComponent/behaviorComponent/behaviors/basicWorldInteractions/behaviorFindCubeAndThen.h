/**
 * File: BehaviorFindCubeAndThen.h
 *
 * Author: Sam Russell
 * Created: 2018-08-20
 *
 * Description: If Vector thinks he know's where a cube is, visually verify that its there. Otherwise, 
 *   search quickly for a cube. Go to the pre-dock pose if found/known, delegate to next behavior if specified
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

class BlockWorldFilter;

class BehaviorFindCubeAndThen : public ICozmoBehavior
{

public: 
  virtual ~BehaviorFindCubeAndThen();
  virtual bool WantsToBeActivatedBehavior() const override;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorFindCubeAndThen(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class FindCubeState{
    GetIn,
    DriveOffCharger,
    VisuallyCheckLastKnown,
    CheckForCubeInFront,
    QuickSearchForCube,
    BackUpAndCheck,
    ReactToCube,
    DriveToPredockPose,
    AttemptConnection,
    FollowUpBehavior,
    GetOutFailure
  };

  enum class CubeObservationState{
    Unreliable,
    ObservedRecently,
    Confirmed
  };

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<BlockWorldFilter> cubesFilter;
    ICozmoBehaviorPtr                 driveOffChargerBehavior;
    ICozmoBehaviorPtr                 connectToCubeBehavior;
    std::string                       followUpBehaviorID;
    ICozmoBehaviorPtr                 followUpBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    FindCubeState        state;
    ObservableObject*    cubePtr;
    CubeObservationState cubeState;
    Pose3d               cubePoseAtSearchStart;
    RobotTimeStamp_t     lastPoseCheckTimestamp;
    float                angleSwept_deg;
    int                  numTurnsCompleted; 
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToDriveOffCharger();
  void TransitionToVisuallyCheckLastKnown();
  void TransitionToCheckForCubeInFront();
  void TransitionToQuickSearchForCube();
  void TransitionToBackUpAndCheck();
  void TransitionToReactToCube();
  void TransitionToDriveToPredockPose();
  void TransitionToAttemptConnection();
  void TransitionToFollowUpBehavior();
  void TransitionToGetOutFailure();

  void StartNextTurn();

  // Returns true if worldViz returned a valid pointer stored in _dVars.cubePtr
  bool UpdateTargetCube();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCubeAndThen__
