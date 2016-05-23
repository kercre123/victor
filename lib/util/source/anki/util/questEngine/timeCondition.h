/**
 * File: timeCondition.h
 *
 * Author: aubrey
 * Created: 05/11/15
 *
 * Description: Condition satisfied based on time range
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_TimeCondition_H__
#define __Util_QuestEngine_TimeCondition_H__

#include "util/questEngine/abstractCondition.h"
#include <stdint.h>

namespace Anki {
namespace Util {
namespace QuestEngine {

  
class TimeCondition : public AbstractCondition
{
public:
  
  explicit TimeCondition();
  ~TimeCondition() {};
  
  bool IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const override;
  
  void SetStartTime(const uint64_t startTime) { _startTime = startTime; }
  
  void SetStopTime(const uint64_t stopTime) { _stopTime = stopTime; }
  
  void SetRange(const uint64_t startTime, const uint64_t stopTime) { _startTime = startTime; _stopTime = stopTime; }
  
private:
  
  bool FilterGreaterThan(const uint64_t eventTime) const;
  
  bool FilterLessThan(const uint64_t eventTime) const;
  
  bool FilterRange(const uint64_t eventTime) const;
  
  uint64_t _startTime;
  uint64_t _stopTime;
  
};

  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_TimeCondition_H__
