/**
 * File: questRule.cpp
 *
 * Author: aubrey
 * Created: 05/14/15
 *
 * Description: Rule for triggering actions in quest engine
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/questRule.h"
#include "util/questEngine/abstractAction.h"
#include "util/questEngine/abstractCondition.h"

namespace Anki {
namespace Util {
namespace QuestEngine {

  QuestRule::QuestRule(const std::string& ruleId, const std::vector<std::string>& eventNames, const std::string& titleKey, const std::string& descriptionKey, const std::string& iconImage, const std::string& cellType, const std::string& link, AbstractCondition* availabilityCondition, AbstractCondition* triggerCondition, AbstractAction* action)
: _cellType(cellType)
, _link(link)
, _eventNames(eventNames)
, _descriptionKey(descriptionKey)
, _iconImage(iconImage)
, _isRepeatable(false)
, _ruleId(ruleId)
, _titleKey(titleKey)
, _availabilityCondition(availabilityCondition)
, _triggerCondition(triggerCondition)
, _action(action)
{
}
  
QuestRule::~QuestRule()
{
  delete _availabilityCondition;
  delete _triggerCondition;
  delete _action;
}

bool QuestRule::IsAvailable(QuestEngine& questEngine, std::tm &time) const
{
  bool isAvailable = false;
  if( _availabilityCondition != nullptr ) {
    isAvailable = _availabilityCondition->IsSatisfied(questEngine, time);
  }
  else {
    // when no availability conditions exist, rule is automatically available
    isAvailable = true;
  }
  return isAvailable;
}

bool QuestRule::IsTriggered(QuestEngine& questEngine, std::tm& eventTime)
{
  bool isTriggered = false;
  if( _triggerCondition != nullptr ) {
    isTriggered = _triggerCondition->IsSatisfied(questEngine, eventTime);
  }
  else {
    // when no trigger conditions exist, rule is automatically triggered
    isTriggered = true;
  }
  return isTriggered;
}
  
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki
