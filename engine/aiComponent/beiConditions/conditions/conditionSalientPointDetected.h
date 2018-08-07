/**
 * File: conditionPersonDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: Condition which is true when a person is detected. Uses SalientPointDetectorComponent
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__

#include "clad/types/salientPointTypes.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"


namespace Anki {
namespace Vector {

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

  Vision::SalientPointType _targetSalientPointType;
};

} // namespace Anki
} // namespace Vector


#endif //__Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
