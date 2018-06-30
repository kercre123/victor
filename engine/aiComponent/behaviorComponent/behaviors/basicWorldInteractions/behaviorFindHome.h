/**
* File: BehaviorFindHome.h
*
* Author: Matt Michini
* Created: 1/31/18
*
* Description:
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorFindHome_H__
#define __Engine_Behaviors_BehaviorFindHome_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
  
class BehaviorFindHome : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindHome(const Json::Value& config);
  
public:
  virtual ~BehaviorFindHome() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;

private:
  struct InstanceConfig {
    InstanceConfig() {}
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    float       minSearchAngleSweep_deg = 0.f;
    int         maxSearchTurns = 0;
    int         numSearches = 0;
    float       minDrivingDist_mm = 0.f;
    float       maxDrivingDist_mm = 0.f;
    
    AnimationTrigger searchTurnAnimTrigger = AnimationTrigger::Count;
    std::unique_ptr<BlockWorldFilter> homeFilter;
  };

  struct DynamicVariables {
    DynamicVariables() {}
    
    // Number of completed 'searches'. One 'search' means
    // spinning around in place and looking for the charger
    int numSearchesCompleted = 0;
    
    // Number of turn animations played while searching in
    // place for the charger
    int numTurnsCompleted = 0;
    
    // Cumulative angle swept while searching in place for
    // the charger
    float angleSwept_deg = 0.f;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToHeadStraight();
  void TransitionToSearchTurn();
  void TransitionToRandomDrive();
  
  void GetRandomDrivingPose(Pose3d& outPose);
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorFindHome_H__
