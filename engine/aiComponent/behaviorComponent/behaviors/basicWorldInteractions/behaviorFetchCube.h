/**
 * File: BehaviorFetchCube.h
 *
 * Author: Sam Russell
 * Created: 2018-08-10
 *
 * Description: Search for a cube(if not known), go get it, bring it to the face who sent you
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFetchCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFetchCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/smartFaceId.h"
#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
class BehaviorFindFaceAndThen;
class BehaviorPickUpCube;
class BlockWorldFilter;

class BehaviorFetchCube : public ICozmoBehavior
{
public: 
  virtual ~BehaviorFetchCube();
  virtual bool WantsToBeActivatedBehavior() const override;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorFetchCube(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class FetchState{
    GetIn,
    DriveOffCharger,
    LookForUser,
    AttemptConnection,
    VisuallyCheckLastKnown,
    QuickSearchForCube,
    ExtensiveSearchForCube,
    LookToUserForHelp,
    ExasperatedSearchForCube,
    PickUpCube,
    TakeCubeSomewhere,
    PlayWithCube,
    GetOutSuccess,
    GetOutFailure,
  };

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<BlockWorldFilter>        cubesFilter;
    std::unique_ptr<BlockWorldFilter>        chargerFilter;
    std::shared_ptr<BehaviorPickUpCube>      pickUpCubeBehavior;
    std::shared_ptr<ICozmoBehavior>          driveOffChargerBehavior;
    std::shared_ptr<ICozmoBehavior>          connectToCubeBehavior;
    std::shared_ptr<BehaviorFindFaceAndThen> lookForUserBehavior;
    std::shared_ptr<ICozmoBehavior>          lookBackAtUserBehavior;
    std::shared_ptr<ICozmoBehavior>          extensiveSearchForCubeBehavior;
    std::shared_ptr<ICozmoBehavior>          playWithCubeBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    FetchState        state;
    ObservableObject* cubePtr;
    SmartFaceID       targetFace;
    Pose3d            poseAtStartOfBehavior;
    Pose3d            destination;
    int               attemptsAtCurrentAction;
    float             stopSearchTime_s;
    bool              startedOnCharger;
    RobotTimeStamp_t  searchStartTimeStamp;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToDriveOffCharger();
  void TransitionToLookForUser();
  void TransitionToAttemptConnection();
  void TransitionToVisuallyCheckLastKnown();
  void TransitionToQuickSearchForCube();
  void TransitionToExtensiveSearchForCube();
  void TransitionToLookToUserForHelp();
  void TransitionToExasperatedSearchForCube();

  void TransitionToPickUpCube();
  void AttemptToPickUpCube();
  
  void TransitionToTakeCubeSomewhere();
  void AttemptToTakeCubeSomewhere();

  void TransitionToPlayWithCube();
  void TransitionToGetOutSuccess();
  void TransitionToGetOutFailure();

  bool CheckForCube(bool useTimeStamp = false, RobotTimeStamp_t maxTimeSinceSeen_ms = 0);
  bool ComputeFaceBasedTargetPose();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFetchCube__
