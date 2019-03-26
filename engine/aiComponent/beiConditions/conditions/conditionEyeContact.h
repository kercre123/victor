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
namespace Vector {

class ConditionEyeContact : public IBEICondition
{
public:
  explicit ConditionEyeContact(const Json::Value& config);
  static uint32_t GetMaxTimeSinceTrackedFaceUpdated_ms();

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::Faces, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::Faces_Gaze, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::Faces_Blink, EVisionUpdateFrequency::Standard });
  }

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionEyeContact_H__
