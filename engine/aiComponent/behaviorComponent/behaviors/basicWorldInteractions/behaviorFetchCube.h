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
class BehaviorFindCube;
class BehaviorFindFaceAndThen;
class BehaviorPickUpCube;
class BehaviorPlaceCubeByCharger;
class BehaviorPutDownBlockAtPose;
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
    FindCube,
    AttemptConnection,
    PickUpCube,
    TakeCubeSomewhere,
    PutCubeDownHere,
    GetOutSuccess,
    GetOutFailure
  };

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<BlockWorldFilter>           cubesFilter;
    std::unique_ptr<BlockWorldFilter>           chargerFilter;

    std::shared_ptr<BehaviorFindFaceAndThen>    lookForUserBehavior;
    std::shared_ptr<BehaviorFindCube>           findCubeBehavior;
    std::shared_ptr<BehaviorPickUpCube>         pickUpCubeBehavior;
    std::shared_ptr<BehaviorPutDownBlockAtPose> putCubeSomewhereBehavior;

    ICozmoBehaviorPtr                           driveOffChargerBehavior;
    ICozmoBehaviorPtr                           reactToCliffBehavior;
    ICozmoBehaviorPtr                           connectToCubeBehavior;
    ICozmoBehaviorPtr                           putCubeByChargerBehavior;
    bool                                        skipConnectToCubeBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    FetchState   state;
    ObjectID     cubeID;
    SmartFaceID  targetFace;
    Pose3d       poseAtStartOfBehavior;
    Pose3d       destination;
    int          attemptsAtCurrentAction;
    bool         startedOnCharger;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToDriveOffCharger();
  void TransitionToLookForUser();
  void TransitionToFindCube();
  void TransitionToAttemptConnection();

  void TransitionToPickUpCube();
  void AttemptToPickUpCube();
  
  void TransitionToTakeCubeSomewhere();
  void TransitionToPutCubeDownHere();

  void TransitionToGetOutSuccess();
  void TransitionToGetOutFailure();

  bool ComputeFaceBasedTargetPose();

  // Returns a pointer to the object belonging with ID == _dVars.cubeID, nullptr if none is found
  ObservableObject* GetTargetCube();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorFetchCube__
