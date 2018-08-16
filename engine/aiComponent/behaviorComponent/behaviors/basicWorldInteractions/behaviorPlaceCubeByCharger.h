/**
 * File: BehaviorPlaceCubeByCharger.h
 *
 * Author: Sam Russell
 * Created: 2018-08-08
 *
 * Description: Pick up a cube and place it nearby the charger, "put it away"
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
class BlockWorldFilter;
class BehaviorPickUpCube;


class BehaviorPlaceCubeByCharger : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPlaceCubeByCharger() {}
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
    PlaceCubeByCharger,
    PutCubeBack,
    GetOutSuccess,
    GetOutFailed
  };

  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorPickUpCube> pickupBehavior;
    std::shared_ptr<ICozmoBehavior>     turnToLastFaceBehavior;
    std::unique_ptr<BlockWorldFilter>   chargerFilter;
    std::unique_ptr<BlockWorldFilter>   cubesFilter;
    bool turnToUserBeforeSuccessReaction;
    bool turnToUserAfterSuccessReaction;
  };

  struct DynamicVariables {
    DynamicVariables();
    PlaceCubeState state;
    ObjectID chargerID;
    Pose3d cubeStartPosition;
    int attemptsAtCurrentAction;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToConfirmCharger();
  void TransitionToSearchForCharger();
  void TransitionToReactToCharger();
  void TransitionToPickUpCube();
  void TransitionToPlacingCubeByCharger();
  void TransitionToPuttingCubeBack();
  void TransitionToGetOutFailed();
  void TransitionToGetOutSucceeded();

  void AttemptToPickUpCube();
  void AttemptToPlaceCubeByCharger();
  void AttemptToPutCubeBack();

  bool RobotKnowsWhereChargerIs();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlaceCubeByCharger__
