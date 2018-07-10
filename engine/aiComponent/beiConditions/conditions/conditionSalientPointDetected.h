/**
 * File: conditionSalientPointDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: Condition which is true when a person is detected. Uses SalientPointDetectorComponent
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__

#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"


namespace Anki {
namespace Cozmo {

class ConditionSalientPointDetected : public IBEICondition
{
public:
  explicit ConditionSalientPointDetected(const Json::Value& config);
  ConditionSalientPointDetected() = delete;
  ~ConditionSalientPointDetected() override ;

protected:
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override;

  Vision::SalientPointType _targetSalientPoint;
};

} // namespace Anki
} // namespace Cozmo


#endif //__Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
