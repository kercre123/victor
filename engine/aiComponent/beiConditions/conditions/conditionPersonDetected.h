/**
 * File: conditionPersonDetected.h
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: Condition which is true when a person is detected
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

private:
  void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& bei) override;

  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
  mutable float _timeSinceLastObservation = 0; // TODO this is temporary for testing

};

} // namespace Anki
} // namespace Cozmo


#endif //__Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionPersonDetected_H__
