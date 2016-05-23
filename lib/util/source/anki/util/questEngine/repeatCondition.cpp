/**
 * File: repeatCondition.cpp
 *
 * Author: aubrey
 * Created: 07/07/15
 *
 * Description: Condition based on time since rule last triggered
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/repeatCondition.h"
#include "util/questEngine/questEngine.h"
#include "util/questEngine/questNotice.h"
#include "util/questEngine/questRule.h"


namespace Anki {
namespace Util {
namespace QuestEngine {
      
RepeatCondition::RepeatCondition(const std::string& ruleId, const double secondsSinceLast)
: _ruleId(ruleId)
, _secondsSinceLast(secondsSinceLast)
{
}

bool RepeatCondition::IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const
{
  bool hasTriggered = questEngine.HasTriggered(_ruleId);
  
  if( hasTriggered ) {
    std::time_t tEvent = std::mktime(&eventTime);
    std::time_t tLast = questEngine.LastTriggeredAt(_ruleId);
    const double deltaTime = std::difftime(tEvent, tLast);
    if( deltaTime < _secondsSinceLast ) {
      return true;
    }
  }
  return false;
}

} // namespace QuestEngine
} // namespace Util
} // namespace Anki
