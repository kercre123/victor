/**
 * File: conditionTimedDedup.cpp
 *
 * Author: Kevin M. Karol
 * Created: 1/24/18
 * 
 * Description: Condition which deduplicates its sub conditions results
 * Once the subcondition returns true once, the deduper will return false
 * for the specified interval, and then return the sub conditions results again
 * This makes it easy to toggle states based on conditions without worrying about
 * throttling back and forth on a per-tick basis
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionTimedDedup.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "lib/util/source/anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kSubConditionKey  = "subCondition";
const char* kDedupIntervalKey = "dedupInterval_ms";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTimedDedup::ConditionTimedDedup(const Json::Value& config)
: IBEICondition(config)
{
  _instanceParams.subCondition     = BEIConditionFactory::CreateBEICondition( config[kSubConditionKey], GetDebugLabel() );
  _instanceParams.dedupInterval_ms = JsonTools::ParseFloat(config,
                                                           kDedupIntervalKey,
                                                           "ConditionTimedDedup.ConfigError.DedupInterval");
  ANKI_VERIFY(_instanceParams.subCondition, "ConditionTimedDedup.Constructor.Config.NullSubCondition", "" );
}  


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTimedDedup::ConditionTimedDedup(IBEIConditionPtr subCondition, float dedupInterval_ms, const std::string& ownerDebugLabel)
: IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::TimedDedup))
{
  SetOwnerDebugLabel( ownerDebugLabel );
  _instanceParams.subCondition     = subCondition;
  _instanceParams.dedupInterval_ms = dedupInterval_ms;
  ANKI_VERIFY(_instanceParams.subCondition, "ConditionTimedDedup.Constructor.Direct.NullSubCondition", "" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionTimedDedup::InitInternal(BehaviorExternalInterface& bei)
{
  if( _instanceParams.subCondition ) {
    _instanceParams.subCondition->Init(bei);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionTimedDedup::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const EngineTimeStamp_t currentTime_ms =  BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if( _instanceParams.subCondition && 
      (currentTime_ms >= _lifetimeParams.nextTimeValid_ms)) {
    const bool subResult = _instanceParams.subCondition->AreConditionsMet(bei);
    if(subResult){
      _lifetimeParams.nextTimeValid_ms = currentTime_ms + _instanceParams.dedupInterval_ms;
    }
    return subResult;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionTimedDedup::SetActiveInternal(BehaviorExternalInterface& bei, bool setActive)
{
  if(_instanceParams.subCondition){
    _instanceParams.subCondition->SetActive(bei, setActive);
  }
}

} // namespace Cozmo
} // namespace Anki
