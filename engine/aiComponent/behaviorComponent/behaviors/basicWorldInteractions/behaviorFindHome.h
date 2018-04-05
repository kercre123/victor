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
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Standard });
  }

  virtual void OnBehaviorActivated() override;

private:
  
  void TransitionToSearchAnim();
  void TransitionToRandomDrive();
  
  void GetRandomDrivingPose(Pose3d& outPose);
  
  struct {
    AnimationTrigger searchAnimTrigger = AnimationTrigger::Count;
    int numSearches = 0;
    float minDrivingDist_mm = 0.f;
    float maxDrivingDist_mm = 0.f;
    TimeStamp_t maxObservedAge_ms = 0;
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  std::unique_ptr<BlockWorldFilter> _homeFilter;
  
  int _numSearchesCompleted = 0;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Behaviors_BehaviorFindHome_H__
