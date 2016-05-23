/**
 * File: countThresholdCondition.h
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

#ifndef __Util_QuestEngine_CountThresholdCondition_H__
#define __Util_QuestEngine_CountThresholdCondition_H__

#include "util/questEngine/abstractCondition.h"
#include <string>

namespace Anki {
namespace Util {
namespace QuestEngine {

typedef enum {
  CountThresholdOperatorEquals = 0,
  CountThresholdOperatorGreaterThan,
  CountThresholdOperatorGreaterThanEqual,
  CountThresholdOperatorLessThan,
  CountThresholdOperatorLessThanEqual,
} CountThresholdOperator;
  
class CountThresholdCondition : public AbstractCondition
{
public:
  
  CountThresholdCondition(CountThresholdOperator countOperator, const uint16_t targetValue, const std::string& triggerKey);
  ~CountThresholdCondition() override {}
  
  bool IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const override;
  
private:
  
  uint16_t _targetValue;
  
  CountThresholdOperator _operator;
  
  std::string _triggerKey;

};
 
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_CountThresholdCondition_H__
