/**
 * File: questNotice.cpp
 *
 * Author: aubrey
 * Created: 05/08/15
 *
 * Description: Represents a notice to be displayed to the user
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/questNotice.h"

namespace Anki {
namespace Util {
namespace QuestEngine {


QuestNotice::QuestNotice(const bool force, const uint8_t priority, const time_t noticeTime, const std::string& guid, const std::string& target, const std::string& titleKey, const std::string& descriptionKey, const std::string& buttonKey, const std::string& navigationAction)
: _force(force)
, _priority(priority)
, _noticeTime(noticeTime)
, _guid(guid)
, _target(target)
, _buttonKey(buttonKey)
, _descriptionKey(descriptionKey)
, _titleKey(titleKey)
, _navigationAction(navigationAction)
{
}

bool QuestNotice::operator()(const QuestNotice& lhs, const QuestNotice& rhs) const
{
  return lhs < rhs;
}
  
bool QuestNotice::operator<(const QuestNotice& obj) const
{
  if( GetNoticeTime() < obj.GetNoticeTime() ) return true;
  if( GetNoticeTime() > obj.GetNoticeTime() ) return false;
  if( GetTitleKey() < obj.GetTitleKey() ) return true;
  return false;
}
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki
