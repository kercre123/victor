/**
* File: conditionUserIntentActive.cpp
*
* Author: ross
* Created: 2018 May 16
*
* Description: Condition to check if there is an active intent that matches one or more user intents
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentActive.h"
#include "json/json.h"

namespace Anki {
namespace Vector {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value ConditionUserIntentActive::GenerateConfig( const Json::Value& config )
{
  Json::Value ret;
  
  ret[kList] = config;
  ret[kConditionTypeKey] = BEIConditionTypeToString( BEIConditionType::UserIntentActive );
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUserIntentActive::ConditionUserIntentActive( const Json::Value& config )
  : IConditionUserIntent( BEIConditionType::UserIntentActive, config )
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionUserIntentActive::ConditionUserIntentActive( const std::string& ownerDebugLabel )
  : IConditionUserIntent( BEIConditionType::UserIntentActive )
{
  SetOwnerDebugLabel( ownerDebugLabel );
}
  
} // end namespace
} // end namespace
