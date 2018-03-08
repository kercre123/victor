/**
* File: behaviorObservingEyeContact.h
*
* Author: Robert Cosgriff
* Created: 2/5/18
*
* Description: Simple behavior to react to eye contact
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorObservingEyeContact_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorObservingEyeContact_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorObservingEyeContact : public ICozmoBehavior
{
protected:
  friend class BehaviorFactory;
  BehaviorObservingEyeContact(const Json::Value& config);
  
public:
  virtual ~BehaviorObservingEyeContact() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingFaces,
                                                     EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingGaze,
                                                     EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingBlinkAmount,
                                                     EVisionUpdateFrequency::Standard });
  }

  virtual void OnBehaviorActivated() override;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorObservingEyeContact_H__
