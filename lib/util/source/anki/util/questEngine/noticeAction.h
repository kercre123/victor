/**
 * File: noticeAction.h
 *
 * Author: aubrey
 * Created: 05/08/15
 *
 * Description: Action responsible for adding notices to quest engine
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_NoticeAction_H__
#define __Util_QuestEngine_NoticeAction_H__

#include "util/questEngine/abstractAction.h"
#include <string>

namespace Anki {
namespace Util {
namespace QuestEngine {

class NoticeAction : public AbstractAction
{
public:
  
  NoticeAction(const bool force, const uint8_t priority, const std::string& target, const std::string& titleKey, const std::string& descriptionKey, const std::string& buttonKey, const std::string& navigationAction);
  ~NoticeAction() {};
  
  void PerformAction(QuestEngine& questEngine) override;
  
private:
  
  bool _force;
  
  uint8_t _priority;
  
  std::string _target;
  
  std::string _buttonKey;
  
  std::string _descriptionKey;
  
  std::string _titleKey;
  
  std::string _navigationAction;
  
};
 
} // namespace QuestEngine
} // namespace Util
} // namsepace Anki

#endif // __Util_QuestEngine_NoticeAction_H__
