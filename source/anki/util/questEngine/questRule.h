/**
 * File: questRule.h
 *
 * Author: aubrey
 * Created: 05/14/15
 *
 * Description: Rule for triggering actions in quest engine
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_QuestRule_H__
#define __Util_QuestEngine_QuestRule_H__

#include <string>
#include <vector>
#include <ctime>


namespace Anki {
namespace Util {
namespace QuestEngine {

class AbstractCondition;
class AbstractAction;
class QuestEngine;

class QuestRule
{
public:
  
  QuestRule(const std::string& ruleId, const std::vector<std::string>& eventNames, const std::string& titleKey, const std::string& descriptionKey, const std::string& iconImage, const std::string& cellType, const std::string& link, AbstractCondition* availableCondition, AbstractCondition* triggerCondition, AbstractAction* action);
  ~QuestRule();
  
  AbstractAction* GetAction() const { return _action; }
  
  const std::string& GetCellType() const { return _cellType; };
  
  const std::string& GetLink() const { return _link; };

  const std::vector<std::string>& GetEventNames() const { return _eventNames; }
  
  const std::string& GetIconImage() const { return _iconImage; };
  
  const std::string& GetId() const { return _ruleId; };
  
  const std::string& GetDescriptionKey() const { return _descriptionKey; };
  
  const std::string& GetTitleKey() const { return _titleKey; };
  
  bool IsAvailable(QuestEngine& questEngine, std::tm& time) const;
  
  bool IsRepeatable() const { return _isRepeatable; };
  
  bool IsTriggered(QuestEngine& questEngine, std::tm& eventTime);
  
  void SetRepeatable(bool repeatable) { _isRepeatable = repeatable; };


private:
  
  std::string _cellType;
  
  std::string _link;
  
  std::vector<std::string> _eventNames;
  
  std::string _descriptionKey;
  
  std::string _iconImage;
  
  bool _isRepeatable;
  
  std::string _ruleId;
  
  std::string _titleKey;
  
  AbstractCondition* _availabilityCondition;
  
  AbstractCondition* _triggerCondition;
  
  AbstractAction* _action;
  
};
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_QuestRule_H__
