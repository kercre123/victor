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
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;

private:
  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    int         numSearches;
    float       minDrivingDist_mm;
    float       maxDrivingDist_mm;
    TimeStamp_t maxObservedAge_ms;
    
    AnimationTrigger searchAnimTrigger;
    std::unique_ptr<BlockWorldFilter> homeFilter;
  };

  struct DynamicVariables {
    DynamicVariables();    
    int numSearchesCompleted;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToHeadStraight();
  void TransitionToSearchAnim();
  void TransitionToRandomDrive();
  
  void GetRandomDrivingPose(Pose3d& outPose);
  
  

  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorFindHome_H__
