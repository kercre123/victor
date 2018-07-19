/**
* File: conditionStare.h
*
* Author: Robert Cosgriff
* Created: 7/17/2018
*
* Description: Strategy for responding to user staring
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionStare_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionStare_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionStare : public IBEICondition
{
public:
  explicit ConditionStare(const Json::Value& config);
  static uint32_t GetMaxTimeSinceTrackedFaceUpdated_ms();

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

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionStare_H__
