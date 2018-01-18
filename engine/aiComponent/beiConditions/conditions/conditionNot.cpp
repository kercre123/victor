/**
 * File: conditionNot.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: Condition which reversed the result of it's sub-condition
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionNot.h"

#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "lib/util/source/anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kSubConditionKey = "subCondition";

}

ConditionNot::ConditionNot(const Json::Value& config)
  : IBEICondition(config)
  , _subCondition( BEIConditionFactory::CreateBEICondition( config[kSubConditionKey] ) )
{
}  

ConditionNot::ConditionNot(IBEIConditionPtr subCondition)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Not))
  , _subCondition(subCondition)
{
}

void ConditionNot::ResetInternal(BehaviorExternalInterface& bei)
{  
  if( ANKI_VERIFY(_subCondition, "ConditionNot.ResetInternal.NullSubCondition", "" ) ) {
    _subCondition->Reset(bei);
  }
}

void ConditionNot::InitInternal(BehaviorExternalInterface& bei)
{
  if( ANKI_VERIFY(_subCondition, "ConditionNot.ResetInternal.NullSubCondition", "" ) ) {
    _subCondition->Init(bei);
  }
}
  
bool ConditionNot::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  if( ANKI_VERIFY(_subCondition, "ConditionNot.ResetInternal.NullSubCondition", "" ) ) {
    const bool subResult = _subCondition->AreConditionsMet(bei);
    return !subResult;
  }
  return false;
}

}
}
