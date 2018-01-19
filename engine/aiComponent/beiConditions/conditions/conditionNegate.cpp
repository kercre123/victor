/**
 * File: conditionNegate.h
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: Condition which reversed the result of it's sub-condition
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionNegate.h"

#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "lib/util/source/anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kOperandKey = "operand";

}

ConditionNegate::ConditionNegate(const Json::Value& config)
  : IBEICondition(config)
  , _operand( BEIConditionFactory::CreateBEICondition( config[kOperandKey] ) )
{
}  

ConditionNegate::ConditionNegate(IBEIConditionPtr operand)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Negate))
  , _operand(operand)
{
}

void ConditionNegate::ResetInternal(BehaviorExternalInterface& bei)
{  
  if( ANKI_VERIFY(_operand, "ConditionNegate.ResetInternal.NullOperand", "" ) ) {
    _operand->Reset(bei);
  }
}

void ConditionNegate::InitInternal(BehaviorExternalInterface& bei)
{
  if( ANKI_VERIFY(_operand, "ConditionNegate.ResetInternal.NullOperand", "" ) ) {
    _operand->Init(bei);
  }
}
  
bool ConditionNegate::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  if( ANKI_VERIFY(_operand, "ConditionNegate.ResetInternal.NullOperand", "" ) ) {
    const bool subResult = _operand->AreConditionsMet(bei);
    return !subResult;
  }
  return false;
}

}
}
