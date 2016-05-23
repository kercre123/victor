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

#ifndef __Util_QuestEngine_QuestNotice_H__
#define __Util_QuestEngine_QuestNotice_H__

#include <string>

namespace Anki {
namespace Util {
namespace QuestEngine {

class QuestNotice
{
public:
  
  QuestNotice(const bool force, const uint8_t priority, const time_t noticeTime, const std::string& guid, const std::string& target, const std::string& titleKey, const std::string& descriptionKey, const std::string& buttonKey, const std::string& navigationAction);
  ~QuestNotice() {};
  
  const time_t GetNoticeTime() const { return _noticeTime; };
  
  const uint8_t GetPriority() const { return _priority; };

  const std::string& GetGuid() const { return _guid; };

  const std::string& GetTarget() const { return _target; };
  
  const std::string& GetButtonKey() const { return _buttonKey; };
  
  const std::string& GetDescriptionKey() const { return _descriptionKey; };

  const std::string& GetTitleKey() const { return _titleKey; };
  
  const std::string& GetNavigationAction() const { return _navigationAction; };
  
  const bool IsForce() const { return _force; };
  
  bool operator()(const QuestNotice& lhs, const QuestNotice& rhs) const;
  bool operator<(const QuestNotice& obj) const;
  
  
private:

  bool _force;
  
  uint8_t _priority;
  
  time_t _noticeTime;
  
  std::string _guid;
  
  std::string _target;
  
  std::string _buttonKey;
  
  std::string _descriptionKey;
  
  std::string _titleKey;
  
  std::string _navigationAction;
  
};

} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_QuestNotice_H__
