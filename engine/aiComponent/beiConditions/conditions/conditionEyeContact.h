/**
* File: conditionEyeContact.h
*
* Author: Robert Cosgriff
* Created: 3/8/2018
*
* Description: Strategy for responding to eye contact
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionEyeContact_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionEyeContact_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionEyeContact : public IBEICondition
{
public:
  ConditionEyeContact(const Json::Value& config, BEIConditionFactory& factory);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::DetectingBlinkAmount, EVisionUpdateFrequency::Standard });
  }

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionEyeContact_H__
