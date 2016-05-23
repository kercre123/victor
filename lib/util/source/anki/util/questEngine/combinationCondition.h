/**
 * File: combinationCondition.h 
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

#ifndef __Util_QuestEngine_CombinationCondition_H__
#define __Util_QuestEngine_CombinationCondition_H__

#include "util/questEngine/abstractCondition.h"
#include <vector>


namespace Anki {
namespace Util {
namespace QuestEngine {

class QuestEngine;

typedef enum {
  CombinationConditionOperationNone = -1,
  CombinationConditionOperationNot,
  CombinationConditionOperationOr,
  CombinationConditionOperationAnd,
  CombinationConditionOperationAny,
} CombinationConditionOperation;

  
class CombinationCondition : public AbstractCondition
{
public:
  
  CombinationCondition(CombinationConditionOperation operation, AbstractCondition* conditionA, AbstractCondition* conditionB = nullptr);
  
  ~CombinationCondition() override;
  
  void AddCondition(AbstractCondition* condition);
  
  bool IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const override;
  
private:
  
  CombinationConditionOperation _operation;
  
  AbstractCondition* _conditionA;
  
  AbstractCondition* _conditionB;
  
  std::vector<AbstractCondition*> _conditions;
  
};
  
} // namespace QuestEngine
} // namespace Util
} // namsepace Anki

#endif // __Util_QuestEngine_CombinationCondition_H__
