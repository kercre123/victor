/**
 * File: noticeAction.cpp
 *
 * Author: aubrey
 * Created: 05/08/15
 *
 * Description: Action responsible for adding notices to quest service
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/noticeAction.h"
#include "util/questEngine/questNotice.h"
#include "util/questEngine/questEngine.h"
#include <time.h>

namespace Anki {
namespace Util {
namespace QuestEngine {

NoticeAction::NoticeAction(const bool force, const uint8_t priority, const std::string& target, const std::string& titleKey, const std::string& descriptionKey, const std::string& buttonKey, const std::string& navigationAction)
: _force(force)
, _priority(priority)
, _target(target)
, _buttonKey(buttonKey)
, _descriptionKey(descriptionKey)
, _titleKey(titleKey)
, _navigationAction(navigationAction)
{
}

void NoticeAction::PerformAction(QuestEngine& questEngine)
{
  time_t now = time(0);
  QuestNotice questNotice(_force, _priority, now, "", _target, _titleKey, _descriptionKey, _buttonKey, _navigationAction);
  questEngine.AddNotice(questNotice);
}

} // namespace QuestEngine
} // namespace Util
} // namespace Anki
