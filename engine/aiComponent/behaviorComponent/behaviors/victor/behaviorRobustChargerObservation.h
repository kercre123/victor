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

struct VisionProcessingResult;

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


  void TransitionToIlluminationCheck();
  void TransitionToObserveCharger();

  // Helper method to evaluate which vision mode modifier is needed here
  void CheckVisionProcessingResult(const VisionProcessingResult& result);

private:

  struct InstanceConfig {
    InstanceConfig();

    int numImagesToWaitFor = 15;

    int numImageCompositesToWaitFor = 3;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    // Whether image compositing is expected to be used here
    bool useImageCompositing = false;

    // Handle provided by VisionComponent when registering a 
    //  callback to its VisionProcessingResult signal. When
    //  destroyed, the callback is automatically unregistered.
    Signal::SmartHandle visionResultSignalHandle = nullptr;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorRobustChargerObservation__
