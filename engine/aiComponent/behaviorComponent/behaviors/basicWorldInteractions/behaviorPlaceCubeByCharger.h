/**
 * File: BehaviorPlaceCubeByCharger.h
 *
 * Author: Sam Russell
 * Created: 2018-08-08
 *
 * Description: Pick up a cube and place it nearby the charger, "put it away". If the charger can't be found,
 *              just put the cube somewhere else nearby.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/pose.h"

#include "util/random/rejectionSamplerHelper_fwd.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
namespace RobotPointSamplerHelper {
  class RejectIfInRange;
  class RejectIfWouldCrossCliff;
  class RejectIfCollidesWithMemoryMap;
}
class CarryingComponent;
class BlockWorldFilter;
class BehaviorPickUpCube;


class BehaviorPlaceCubeByCharger : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPlaceCubeByCharger() override;
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPlaceCubeByCharger(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = false;
  }
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  enum class PlaceCubeState{
    PickUpCube,
    ConfirmCharger,
    SearchForCharger,
    ReactToCharger,
    DriveToChargerPlacement,
    DriveToRandomPlacement,
    PutCubeDown,
    LookDownAtBlock,
    GetOut
  };

  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorPickUpCube> pickupBehavior;
    std::shared_ptr<ICozmoBehavior>     turnToLastFaceBehavior;
    std::unique_ptr<BlockWorldFilter>   chargerFilter;
    std::unique_ptr<BlockWorldFilter>   cubesFilter;

    std::unique_ptr<Util::RejectionSamplerHelper<Point2f>> searchSpacePointEvaluator;
    std::unique_ptr<Util::RejectionSamplerHelper<Poly2f>>  searchSpacePolyEvaluator;

    std::shared_ptr<RobotPointSamplerHelper::RejectIfWouldCrossCliff> condHandleCliffs;
    std::shared_ptr<RobotPointSamplerHelper::RejectIfCollidesWithMemoryMap> condHandleCollisions;

    bool turnToUserBeforeSuccessReaction;
    bool turnToUserAfterSuccessReaction;
  };

  struct DynamicVariables {
    DynamicVariables();
    PlaceCubeState state;
    ObjectID chargerID;
    Pose3d cubeStartPosition;
    float angleSwept_deg;
    int numTurnsCompleted;
    int attemptsAtCurrentAction;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToConfirmCharger();
  void TransitionToSearchForCharger();
  void TransitionToReactToCharger();
  void TransitionToPickUpCube();
  void TransitionToDriveToChargerPlacement();
  void TransitionToDriveToRandomPlacement();
  void TransitionToPutCubeDown();
  void TransitionToLookDownAtBlock();
  // void TransitionToGetOutFailed();
  // void TransitionToGetOutSucceeded();
  void TransitionToGetOut();

  void AttemptToPickUpCube();
  IActionRunner* CreateLookAfterPlaceAction(CarryingComponent& carryingComponent, bool doLookAtFaceAfter);
  // void AttemptToPlaceCubeByCharger();
  // void AttemptToPutCubeBack();
  void StartNextSearchForChargerTurn();

  bool RobotKnowsWhereChargerIs();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
