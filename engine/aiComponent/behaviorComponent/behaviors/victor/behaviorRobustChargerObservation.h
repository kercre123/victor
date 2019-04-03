/**
 * File: BehaviorRobustChargerObservation.h
 *
 * Author: Arjun Menon
 * Created: 2019-03-05
 *
 * Description: Under certain lighting conditions, this behavior 
 * will handle choosing the appropriate vision modes to maximize 
 * probability of observing the charger
 * 
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorRobustChargerObservation__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorRobustChargerObservation__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class WaitForLambdaAction;

class BehaviorRobustChargerObservation : public ICozmoBehavior
{
public: 
  virtual ~BehaviorRobustChargerObservation();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorRobustChargerObservation(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

  void TransitionToIlluminationCheck();
  void TransitionToObserveCharger();

  bool IsLowLightVision() const;

private:

  struct InstanceConfig {
    InstanceConfig();

    // Note: these frame counts are not image frames
    int numImageCompositingCyclesToWaitFor = 1;
    int numCyclingExposureCyclesToWaitFor = 1;
  };

  struct DynamicVariables {
    DynamicVariables();

    // Count of the frames of marker detection being run
    //  while this behavior is activated.
    u32 numFramesOfDetectingMarkers = 0;

    // Count of the frames where the image quality was TooDark.
    // NOTE: only counted while marker detection is being run.
    u32 numFramesOfImageTooDark = 0; 

    bool isLowlight = false;
    bool playedGetout = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorRobustChargerObservation__
