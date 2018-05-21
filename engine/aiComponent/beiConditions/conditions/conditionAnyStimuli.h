/**
* File: conditionAnyStimuli.cpp
*
* Author: ross
* Created: May 15 2018
*
* Description: Condition that standardizes the params of stimuli conditions, so you only specify the stimuli
*
* Copyright: Anki, Inc. 2018
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionAnyStimuli_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionAnyStimuli_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionAnyStimuli : public IBEICondition
{
public:
  explicit ConditionAnyStimuli(const Json::Value& config);
  
  explicit ConditionAnyStimuli(const std::set<BEIConditionType>& types);
  
  virtual ~ConditionAnyStimuli() = default;
  
  void AddCondition( BEIConditionType type );
  
protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool active) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override;
  virtual DebugFactorsList GetDebugFactors() const override;
  
private:
  std::vector<IBEIConditionPtr> _conditions;
};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionAnyStimuli_H__
