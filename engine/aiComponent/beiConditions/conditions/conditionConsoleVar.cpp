/**
 * File: conditionConsoleVar.h
 *
 * Author: ross
 * Created: feb 22 2018
 *
 * Description: Condition that is true when a console var matches a particular value
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionConsoleVar.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/console/consoleSystem.h"
#include "util/console/consoleVariable.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kVariableNameKey = "variable";
const char* kValueKey = "value";
const char* kParseDebug = "ConditionConsoleVar";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionConsoleVar::ConditionConsoleVar(const Json::Value& config)
  : IBEICondition(config)
{
  _variableName = JsonTools::ParseString(config, kVariableNameKey, kParseDebug);
  _expectedValue = JsonTools::ParseInt32(config, kValueKey, kParseDebug);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionConsoleVar::~ConditionConsoleVar()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionConsoleVar::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  return (_var != nullptr) && (_var->GetAsInt64() == _expectedValue);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionConsoleVar::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive)
{
  if( !ANKI_DEV_CHEATS ) {
    return;
  }
  
  if( isActive ) {
    _var = Anki::Util::ConsoleSystem::Instance().FindVariable( _variableName.c_str() );
  } else {
    _var = nullptr;
  }
}

} // namespace
} // namespace
