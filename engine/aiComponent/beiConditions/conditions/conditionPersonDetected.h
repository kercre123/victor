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

#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"


namespace Anki {
namespace Cozmo {

class ConditionPersonDetected : public IBEICondition, private IBEIConditionEventHandler
{
public:
  explicit ConditionPersonDetected(const Json::Value& config);
  ConditionPersonDetected();
  ~ConditionPersonDetected() override ;

protected:
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override;

};

} // namespace Anki
} // namespace Cozmo


#endif //__Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
