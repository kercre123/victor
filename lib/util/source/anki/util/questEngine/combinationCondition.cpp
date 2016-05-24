/**
 * File: combinationCondition.cpp
 *
 * Author: aubrey
 * Created: 05/12/15
 *
 * Description: Condition used to combine other conditions together logically
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/combinationCondition.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Util {
namespace QuestEngine {

CombinationCondition::CombinationCondition(CombinationConditionOperation operation, AbstractCondition* conditionA, AbstractCondition* conditionB)
: AbstractCondition()
, _operation(operation)
, _conditionA(conditionA)
, _conditionB(conditionB)
{
}
  
CombinationCondition::~CombinationCondition()
{
  SafeDelete(_conditionA);
  SafeDelete(_conditionB);
  for (auto& condition : _conditions) {
    SafeDelete(condition);
  }
}

void CombinationCondition::AddCondition(AbstractCondition* condition)
{
  _conditions.push_back(condition);
}

bool CombinationCondition::IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const
{
  bool triggered = false;
  switch (_operation) {
    case CombinationConditionOperationAnd:
      triggered = _conditionA->IsSatisfied(questEngine, eventTime) && _conditionB->IsSatisfied(questEngine, eventTime);
      break;
      
    case CombinationConditionOperationOr:
      triggered = _conditionA->IsSatisfied(questEngine, eventTime) || _conditionB->IsSatisfied(questEngine, eventTime);
      break;
      
    case CombinationConditionOperationNot:
      triggered = !_conditionA->IsSatisfied(questEngine, eventTime);
      break;
      
    case CombinationConditionOperationAny:
      for (const auto& condition : _conditions) {
        triggered |= condition->IsSatisfied(questEngine, eventTime);
      }
      break;
      
    case CombinationConditionOperationNone:
    default:
      break;
  }
  return triggered;
}
  

} // namespace StatEngine
} // namespace Util
} // namespace Anki
