/**
 * File: behaviorClearChargerArea
 *
 * Author: Matt Michini
 * Created: 5/9/18
 *
 * Description: One way or another, clear out the area in front of the charger so the robot
 *              successfully docks with it.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Behaviors_BehaviorClearChargerArea_H__
#define __Engine_Behaviors_BehaviorClearChargerArea_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BlockWorldFilter;
class BehaviorPickUpCube;
class BehaviorRequestToGoHome;
  
class BehaviorClearChargerArea : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorClearChargerArea(const Json::Value& config);
  
public:
  virtual ~BehaviorClearChargerArea() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);

    std::shared_ptr<BehaviorPickUpCube>       pickupBehavior;
    std::shared_ptr<BehaviorRequestToGoHome>  requestHomeBehavior;
    const int maxNumAttempts;
    const bool tryToPickUpCube;
    std::unique_ptr<BlockWorldFilter> chargerFilter;
    std::unique_ptr<BlockWorldFilter> cubesFilter;
  };

  struct DynamicVariables {
    DynamicVariables() {};
    ObjectID chargerID;
    int numAttempts = 0;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToCheckDockingArea();
  void TransitionToPositionForRamming();
  void TransitionToRamCube();
  void TransitionToPlacingCubeOnGround();
  
  // Get a pointer to a cube that is in the charger's
  // docking area (default to the closest one), or nullptr
  // if there is none
  const ObservableObject* GetCubeInChargerArea() const;

};
  

} // namespace Vector
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorClearChargerArea_H__
