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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceNormalAtRobot_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceNormalAtRobot_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionFaceNormalAtRobot : public IBEICondition
{
public:
  explicit ConditionFaceNormalAtRobot(const Json::Value& config);
  static uint32_t GetMaxTimeSinceTrackedFaceUpdated_ms();

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::DetectingGaze, EVisionUpdateFrequency::Standard });
    requiredVisionModes.insert({ VisionMode::DetectingBlinkAmount, EVisionUpdateFrequency::Standard });
  }

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionFaceNormalAtRobot_H__
