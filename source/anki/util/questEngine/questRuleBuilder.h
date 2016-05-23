/**
 * File: questRuleBuilder.h
 *
 * Author: aubrey
 * Created: 05/11/15
 *
 * Description: Builder class responsible for parsing JSON rules definition and configuring quest engine
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_QuestRuleBuilder_H__
#define __Util_QuestEngine_QuestRuleBuilder_H__

#include <string>
#include <functional>
#include <map>


namespace Json {
class Value;
}


namespace Anki {
namespace Util {
namespace QuestEngine {


class AbstractCondition;
class AbstractAction;
class QuestRule;
class QuestEngine;

using ActionHandler = std::function<AbstractAction*(const Json::Value&)>;
  
using ConditionHandler = std::function<AbstractCondition*(const Json::Value&)>;
  

class QuestRuleBuilder
{
public:
  
  QuestRuleBuilder();
  ~QuestRuleBuilder() {};
  
  void AddActionHandler(const std::string& actionType, const ActionHandler handler);
  
  void AddConditionHandler(const std::string& conditionType, const ConditionHandler handler);
  
  bool LoadDefinitions(const std::string& fileName, QuestEngine& questEngine);
  
private:
  
  QuestRule* BuildRule(const Json::Value& item);
  
  AbstractCondition* BuildCondition(const Json::Value& item);
  
  AbstractAction* BuildAction(const Json::Value& item);
  
  void LoadDefaultActionHandlers();
  
  void LoadDefaultConditionHandlers();
  
  std::map<std::string,ActionHandler> _actionHandlers;
  std::map<std::string,ConditionHandler> _conditionHandlers;
  
};
  
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_QuestRuleBuilder_H__
