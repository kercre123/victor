/**
 * File: dateCondition.cpp
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

#include "util/questEngine/dateCondition.h"


namespace Anki {
namespace Util {
namespace QuestEngine {
  
DateCondition::DateCondition()
: _isDayOfWeek(false)
, _dayOfWeek(0)
, _isDayOfMonth(false)
, _dayOfMonth(0)
{
}
  
bool DateCondition::IsSatisfied(QuestEngine& questEngine, std::tm &eventTime) const
{
  if( _isDayOfWeek ) {
    return eventTime.tm_wday == _dayOfWeek;
  }
  else if( _isDayOfMonth ) {
    return eventTime.tm_mday == _dayOfMonth;
  }
  return false;
}

} // namespace QuestEngine
} // namespace Util
} // namespace Anki
