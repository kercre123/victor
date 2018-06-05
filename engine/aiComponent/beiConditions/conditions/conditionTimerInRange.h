/**
 * File: strategyTimerInRange.h
 *
 * Author: Brad Neuman
 * Created: 2017-12-10
 *
 * Description: Simple strategy to become true a given time after a reset
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_ConditionTimerInRange_H__
#define __Engine_AiComponent_StateConceptStrategies_ConditionTimerInRange_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include <limits>

namespace Anki {
namespace Cozmo {

class ConditionTimerInRange : public IBEICondition
{
public:
  explicit ConditionTimerInRange(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  // By default the timer will be reset when SetActive is called. 
  // This behavior can be modified from JSON or consoleVar
  virtual void SetActiveInternal(BehaviorExternalInterface& bei, bool setActive) override;
  
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
  // for use when _manualResetOnly
  void Reset();
  

private:


  struct Params {
    float _rangeBegin_s = 0.0f;
    float _rangeEnd_s = std::numeric_limits<float>::max();
    bool _manualResetOnly = false; // if true, SetActive() doesn't touch the timer, and public methods are used instead
  };

  Params _params;

  float _timeReset = -1.0f;
  
};


} // namespace
} // namespace



#endif
