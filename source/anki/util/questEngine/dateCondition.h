/**
 * File: dateCondition.h
 *
 * Author: aubrey
 * Created: 05/11/15
 *
 * Description: Condition based on date
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_DateCondition_H__
#define __Util_QuestEngine_DateCondition_H__

#include "util/questEngine/abstractCondition.h"
#include <stdint.h>
#include <ctime>

namespace Anki {
namespace Util {
namespace QuestEngine {
    

class DateCondition : public AbstractCondition
{
public:
  
  explicit DateCondition();
  ~DateCondition() {};
  
  bool IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const override;
  
  void SetDayOfWeek(const uint8_t dayOfWeek) { _isDayOfWeek = true; _isDayOfMonth = false; _dayOfWeek = dayOfWeek; }
  
  void SetDayOfMonth(const uint8_t dayOfMonth) { _isDayOfWeek = false; _isDayOfMonth = true; _dayOfMonth = dayOfMonth; }
  
private:
  
  bool _isDayOfWeek;
  uint8_t _dayOfWeek;
  
  bool _isDayOfMonth;
  uint8_t _dayOfMonth;
  
};
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_DateCondition_H__
