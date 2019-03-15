/**
 * File: conditionRobotHeldInPalm.h
 *
 * Author: Guillermo Bautista
 * Created: 2019/03/11
 *
 * Description: Condition that checks if Vector's held-in-palm matches that of the supplied config,
 * with the option to specify a minimum and/or maximum duration of time that Vector must be held in
 * the user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __AiComponent_BeiConditions_ConditionRobotHeldInPalm__
#define __AiComponent_BeiConditions_ConditionRobotHeldInPalm__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionRobotHeldInPalm : public IBEICondition
{
public:
  explicit ConditionRobotHeldInPalm(const Json::Value& config);
  explicit ConditionRobotHeldInPalm(const bool shouldBeHeldInPalm,
                                    const std::string& ownerDebugLabel,
                                    const int minDuration_ms = 0,
                                    const int maxDuration_ms = INT_MAX);

  virtual ~ConditionRobotHeldInPalm() {};

protected:

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;
  
private:
  
  bool _shouldBeHeldInPalm;
  
  int _minDuration_ms;
  int _maxDuration_ms;

};


} // namespace Vector
} // namespace Anki

#endif // __AiComponent_BeiConditions_ConditionRobotHeldInPalm__
