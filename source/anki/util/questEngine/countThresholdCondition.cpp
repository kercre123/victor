/**
 * File: countThresholdCondition.cpp
 *
 * Author: aubrey
 * Created: 05/06/15
 *
 * Description: Condition based on event count
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/countThresholdCondition.h"
#include "util/questEngine/questEngine.h"

namespace Anki {
namespace Util {
namespace QuestEngine {
    
CountThresholdCondition::CountThresholdCondition(CountThresholdOperator countOperator, const uint16_t targetValue, const std::string& triggerKey)
: AbstractCondition()
, _targetValue(targetValue)
, _operator(countOperator)
, _triggerKey(triggerKey)
{
}
  
bool CountThresholdCondition::IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const
{
  const std::string& eventName = _triggerKey;
  uint32_t count = questEngine.GetEventCount(eventName);
  bool isTriggered = false;
  switch (_operator) {
    case CountThresholdOperatorEquals:
      isTriggered = (count == _targetValue);
      break;
      
    case CountThresholdOperatorGreaterThan:
      isTriggered = (count > _targetValue);
      break;
      
    case CountThresholdOperatorGreaterThanEqual:
      isTriggered = (count >= _targetValue);
      break;
      
    case CountThresholdOperatorLessThan:
      isTriggered = (count < _targetValue);
      break;
      
    case CountThresholdOperatorLessThanEqual:
      isTriggered = (count <= _targetValue);
      break;
  }
  return isTriggered;
}

} // namespace QuestEngine
} // namespace Util
} // namespace Anki
