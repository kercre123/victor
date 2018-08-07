/**
 * File: conditionBecameTrueThisTick.cpp
 *
 * Author: ross
 * Created: Jun 5 2018
 * 
 * Description: Condition that is true when a subcondition transitions from false to true this tick
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionBecameTrueThisTick.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "lib/util/source/anki/util/logging/logging.h"

namespace Anki {
namespace Vector {

namespace {
const char* kSubConditionKey  = "subCondition";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionBecameTrueThisTick::ConditionBecameTrueThisTick(const Json::Value& config)
: IBEICondition(config)
{
  _subCondition = BEIConditionFactory::CreateBEICondition( config[kSubConditionKey], GetDebugLabel() );
  ANKI_VERIFY(_subCondition, "ConditionBecameTrueThisTick.Constructor.Config.NullSubCondition", "" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionBecameTrueThisTick::InitInternal(BehaviorExternalInterface& bei)
{
  if( _subCondition ) {
    _subCondition->Init(bei);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionBecameTrueThisTick::SetActiveInternal(BehaviorExternalInterface& bei, bool setActive)
{
  if(_subCondition){
    _subCondition->SetActive(bei, setActive);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionBecameTrueThisTick::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool subResult = _subCondition->AreConditionsMet( bei );
  if( _lastResult == -1 ) {
    // even if the very first result is true, that doesn't count!
    _lastResult = subResult ? 1 : 0;
    return false;
  } else {
    const bool result = ((_lastResult == 0) && subResult);
    _lastResult = subResult ? 1 : 0;
    return result;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionBecameTrueThisTick::BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const
{
  factors.AddChild( "subCondition", _subCondition );
}

} // namespace Vector
} // namespace Anki
