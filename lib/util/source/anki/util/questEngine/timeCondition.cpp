/**
 * File: timeCondition.cpp
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

#include "util/questEngine/timeCondition.h"

namespace Anki {
namespace Util {
namespace QuestEngine {
    
  
TimeCondition::TimeCondition()
: _startTime(INT64_MAX)
, _stopTime(INT64_MAX)
{
}

bool TimeCondition::IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const
{
  std::time_t time = std::mktime(&eventTime);
  if( _startTime != INT64_MAX && _stopTime != INT64_MAX ) {
    return FilterRange(time);
  }
  else if( _startTime == INT64_MAX && _stopTime != INT64_MAX ) {
    return FilterLessThan(time);
  }
  else if( _startTime != INT64_MAX && _stopTime == INT64_MAX ) {
    return FilterGreaterThan(time);
  }
  return false;
}
  

// Private methods
  
bool TimeCondition::FilterGreaterThan(const uint64_t eventTime) const
{
  return _startTime <= eventTime;
}
  
bool TimeCondition::FilterLessThan(const uint64_t eventTime) const
{
  return _stopTime >= eventTime;
}
  
bool TimeCondition::FilterRange(const uint64_t eventTime) const
{
  return _startTime <= eventTime && _stopTime >= eventTime;
}
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki
