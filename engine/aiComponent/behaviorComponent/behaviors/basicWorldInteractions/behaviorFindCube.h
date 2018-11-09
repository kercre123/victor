/**
 * File: BehaviorFindCube.h
 *
 * Author: Sam Russell
 * Created: 2018-10-15
 *
 * Description: Quickly check the last known location of a cube and search nearby for one if none are
 *              known/found in the initial check
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

//FWD Declartions
class BlockWorldFilter;

class BehaviorFindCube : public ICozmoBehavior
{

public: 
  virtual ~BehaviorFindCube();
  virtual bool WantsToBeActivatedBehavior() const override;

  ObjectID GetFoundCubeID() { return _dVars.cubeID; }

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorFindCube(const Json::Value& config);  

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
    GetOutFailure
  };

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<BlockWorldFilter> cubesFilter;
    ICozmoBehaviorPtr                 driveOffChargerBehavior;
    bool                              skipReactToCubeAnim;
  };

  struct DynamicVariables {
    DynamicVariables();
    FindCubeState        state;
    ObjectID             cubeID;
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
  void TransitionToGetOutFailure();

  void StartNextTurn();

  // Returns true if worldViz returned a valid pointer stored in _dVars.cubePtr
  bool UpdateTargetCube();

  // Returns a pointer to the object belonging with ID == _dVars.cubeID, nullptr if none is found
  ObservableObject* GetTargetCube();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFindCube__
