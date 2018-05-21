/**
* File: conditionUserIntentPending.cpp
*
* Author: ross
* Created: 2018 Feb 13
*
* Description: Condition to check if one or more user intent(s) are pending
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value ConditionUserIntentPending::GenerateConfig( const Json::Value& config )
{
  Json::Value ret;
  
  ret[kList] = config;
  ret[kConditionTypeKey] = BEIConditionTypeToString( BEIConditionType::UserIntentPending );
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUserIntentPending::ConditionUserIntentPending( const Json::Value& config )
  : IConditionUserIntent( BEIConditionType::UserIntentPending, config )
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUserIntentPending::ConditionUserIntentPending()
  : IConditionUserIntent( BEIConditionType::UserIntentPending )
{
}
  
} // end namespace
} // end namespace
