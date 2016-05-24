/**
 * File: questRuleBuilder.cpp
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

#include "util/questEngine/questRuleBuilder.h"
#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/abstractCondition.h"
#include "util/questEngine/countThresholdCondition.h"
#include "util/questEngine/combinationCondition.h"
#include "util/questEngine/dateCondition.h"
#include "util/questEngine/repeatCondition.h"
#include "util/questEngine/multiAction.h"
#include "util/questEngine/noticeAction.h"
#include "util/questEngine/resetAction.h"
#include "util/questEngine/timeCondition.h"
#include "util/fileUtils/fileUtils.h"
#include "json/json.h"

namespace Anki {
namespace Util {
namespace QuestEngine {

const char* kQuestRuleRulesKey = "rules";
const char* kQuestRuleIdKey = "id";
const char* kQuestRuleRepeatableKey = "repeatable";
const char* kQuestRuleTitleKey = "titleKey";
const char* kQuestRuleDescriptionKey = "descriptionKey";
const char* kQuestRuleIconKey = "iconImage";
const char* kQuestRuleCellTypeKey = "cellType";
const char* kQuestRuleLinkKey = "link";
const char* kQuestRuleAvailabilityKey = "availability";
const char* kQuestRuleConditionKey = "condition";
const char* kQuestRuleActionKey = "action";
const char* kQuestRuleEventNamesKey = "eventNames";
const char* kQuestRuleConditionTypeKey = "type";
const char* kQuestRuleConditionAttributesKey = "attributes";
const char* kQuestRuleAttributeOperatorKey = "operator";
const char* kQuestRuleAttributeTargetValueKey = "targetValue";
const char* kQuestRuleAttributeTriggerKeyKey = "triggerKey";
const char* kQuestRuleAttributeTriggerActionKey = "triggeredAction";
const char* kQuestRuleActionTypeKey = "type";
const char* kQuestRuleActionAttributesKey = "attributes";
const char* kQuestRuleNoticeForceKey = "force";
const char* kQuestRuleNoticePriorityKey = "priority";
const char* kQuestRuleNoticeTargetKey = "target";
const char* kQuestRuleNoticeTitleKey = "titleKey";
const char* kQuestRuleNoticeDescriptionKey = "descriptionKey";
const char* kQuestRuleNoticeButtonKey = "buttonKey";
const char* kQuestRuleNoticeNavigationActionKey = "navigationAction";
const char* kQuestRuleActionAttributeEventName = "eventName";
  

QuestRuleBuilder::QuestRuleBuilder()
{
  LoadDefaultActionHandlers();
  LoadDefaultConditionHandlers();
}
  
void QuestRuleBuilder::AddActionHandler(const std::string &actionType, const ActionHandler handler)
{
  _actionHandlers[actionType] = handler;
}

void QuestRuleBuilder::AddConditionHandler(const std::string &conditionType, const ConditionHandler handler)
{
  _conditionHandlers[conditionType] = handler;
}
  
  
bool QuestRuleBuilder::LoadDefinitions(const std::string& fileName, QuestEngine& questEngine)
{
  std::string jsonString = FileUtils::ReadFile(fileName);

  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(jsonString, root);

  bool rulesAdded = false;
  
  if(parsingSuccessful){
    rulesAdded = true;
    std::vector<QuestRule*> newRules;
    
    Json::Value rulesArray = root[kQuestRuleRulesKey];
    for (Json::ValueIterator it = rulesArray.begin(); it != rulesArray.end(); ++it) {
      Json::Value item = (*it);
      QuestRule* rule = BuildRule(item);
      if( rule == nullptr ) {
        rulesAdded = false;
        break;
      }
      else {
        newRules.push_back(rule);
      }
    }
    
    if( rulesAdded ) {
      questEngine.ClearRules();
      
      for (auto it = newRules.begin(); it != newRules.end(); ++it) {
        questEngine.AddRule(*it);
      }
    }
    else {
      // clean up any rules we created
      for (auto it = newRules.begin(); it != newRules.end(); ++it) {
        delete *it;
      }
    }
  }
  
  return rulesAdded;
}
  
QuestRule* QuestRuleBuilder::BuildRule(const Json::Value &item)
{
  std::string ruleId;
  std::vector<std::string> eventNames;
  AbstractCondition* availabilityCondition = nullptr;
  AbstractCondition* triggerCondition = nullptr;
  AbstractAction* action = nullptr;
  bool repeatable = false;
  std::string titleKey;
  std::string descriptionKey;
  std::string iconImage;
  std::string cellType;
  std::string link;
  
  ruleId = item[kQuestRuleIdKey].asString();
  repeatable = item.get(kQuestRuleRepeatableKey, false).asBool();
  titleKey = item[kQuestRuleTitleKey].asString();
  descriptionKey = item[kQuestRuleDescriptionKey].asString();
  iconImage = item[kQuestRuleIconKey].asString();
  cellType = item[kQuestRuleCellTypeKey].asString();
  link = item[kQuestRuleLinkKey].asString();
  
  const Json::Value& eventNamesArray = item[kQuestRuleEventNamesKey];
  for (Json::ArrayIndex k = 0; k < eventNamesArray.size(); ++k) {
    std::string eventName = eventNamesArray[k].asString();
    if( !eventName.empty() ) {
      eventNames.push_back(eventName);
    }
  }
  
  const Json::Value& actionItem = item[kQuestRuleActionKey];
  action = BuildAction(actionItem);
  
  const Json::Value& conditionItem = item[kQuestRuleConditionKey];
  triggerCondition = BuildCondition(conditionItem);
  
  const Json::Value& availabilityItem = item[kQuestRuleAvailabilityKey];
  availabilityCondition = BuildCondition(availabilityItem);

  QuestRule* rule = nullptr;
  if( action != nullptr && !ruleId.empty() && !eventNames.empty() ) {
    rule = new QuestRule(ruleId, eventNames, titleKey, descriptionKey, iconImage, cellType, link, availabilityCondition, triggerCondition, action);
    rule->SetRepeatable(repeatable);
  }
  else {
    delete availabilityCondition;
    delete triggerCondition;
    delete action;
  }

  return rule;
}

AbstractCondition* QuestRuleBuilder::BuildCondition(const Json::Value &item)
{
  const std::string& conditionType = item.get(kQuestRuleConditionTypeKey, "").asString();
  const auto& it = _conditionHandlers.find(conditionType);
  if( it != _conditionHandlers.end() ) {
    return it->second(item);
  }
  return nullptr;
}
  
AbstractAction* QuestRuleBuilder::BuildAction(const Json::Value &item)
{
  const std::string& actionType = item.get(kQuestRuleActionTypeKey, "").asString();
  const auto& it = _actionHandlers.find(actionType);
  if( it != _actionHandlers.end() ) {
    return it->second(item);
  }
  return nullptr;
}
 

void QuestRuleBuilder::LoadDefaultActionHandlers()
{
  const ActionHandler noticeHandler = [] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleActionAttributesKey];
    const bool force = attributes.get(kQuestRuleNoticeForceKey, false).asBool();
    const uint8_t priority = attributes.get(kQuestRuleNoticePriorityKey, 0).asUInt();
    const std::string& target = attributes.get(kQuestRuleNoticeTargetKey, "").asString();
    const std::string& titleKey = attributes.get(kQuestRuleNoticeTitleKey, "").asString();
    const std::string& descriptionKey = attributes.get(kQuestRuleNoticeDescriptionKey, "").asString();
    const std::string& buttonKey = attributes.get(kQuestRuleNoticeButtonKey, "").asString();
    const std::string& navigationAction = attributes.get(kQuestRuleNoticeNavigationActionKey, "").asString();
    NoticeAction* noticeAction = new NoticeAction(force, priority, target, titleKey, descriptionKey, buttonKey, navigationAction);
    return noticeAction;
  };
  
  const ActionHandler resetHandler = [] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleActionAttributesKey];
    const std::string& eventName = attributes.get(kQuestRuleActionAttributeEventName, "").asString();
    ResetAction* resetAction = new ResetAction(eventName);
    return resetAction;
  };
  
  const ActionHandler multiHandler = [this] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleActionAttributesKey];
    const Json::Value& actionsArray = attributes["actions"];
    std::vector<AbstractAction*> actions;
    for (Json::ArrayIndex k = 0; k < actionsArray.size(); ++k) {
      AbstractAction* action = BuildAction(actionsArray[k]);
      if( nullptr != action ) {
        actions.push_back(action);
      }
    }

    MultiAction* multiAction = new MultiAction(actions);
    return multiAction;
  };
  
  AddActionHandler("Notice", noticeHandler);
  AddActionHandler("Reset", resetHandler);
  AddActionHandler("Multi", multiHandler);
}
  
void QuestRuleBuilder::LoadDefaultConditionHandlers()
{
  const ConditionHandler combinationHandler = [this] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleConditionAttributesKey];
    const std::string& combinationOperation = attributes.get("operation", "").asString();

    CombinationConditionOperation op = CombinationConditionOperationNone;
    AbstractCondition* conditionA = nullptr;
    AbstractCondition* conditionB = nullptr;
    
    CombinationCondition* condition = nullptr;
    bool valid = true;
    if( combinationOperation == "AND" ) {
      op = CombinationConditionOperationAnd;
      conditionA = BuildCondition(attributes["conditionA"]);
      conditionB = BuildCondition(attributes["conditionB"]);
      if( conditionA == nullptr || conditionB == nullptr ) {
        valid = false;
        delete conditionA;
        delete conditionB;
      }
    }
    else if( combinationOperation == "OR" ) {
      op = CombinationConditionOperationOr;
      conditionA = BuildCondition(attributes["conditionA"]);
      conditionB = BuildCondition(attributes["conditionB"]);
      if( conditionA == nullptr || conditionB == nullptr ) {
        valid = false;
        delete conditionA;
        delete conditionB;
      }
    }
    else if( combinationOperation == "NOT" ) {
      op = CombinationConditionOperationNot;
      conditionA = BuildCondition(attributes["conditionA"]);
      if( conditionA == nullptr ) {
        valid = false;
      }
    }
    
    if( combinationOperation == "ANY" ) {
      op = CombinationConditionOperationAny;
      std::vector<AbstractCondition*> conditions;
      const Json::Value& conditionsArray = attributes["conditions"];
      for (Json::ArrayIndex k = 0; k < conditionsArray.size(); ++k) {
        AbstractCondition* condition = BuildCondition(conditionsArray[k]);
        if( nullptr != condition ) {
          conditions.push_back(condition);
        }
      }
      if( !conditions.empty() ) {
        condition = new CombinationCondition(op, nullptr, nullptr);
        for (const auto& c : conditions) {
          condition->AddCondition(c);
        }
      }
    }
    else if( valid ) {
      condition = new CombinationCondition(op, conditionA, conditionB);
    }
    return condition;
  };
  
  const ConditionHandler repeatHandler = [] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleConditionAttributesKey];
    std::string ruleId = attributes.get("ruleId", "").asString();
    double noticeTime = attributes.get("secondsSinceLast", 0).asDouble();
    RepeatCondition* repeatCondition = new RepeatCondition(ruleId, noticeTime);
    return repeatCondition;
  };
  
  const ConditionHandler countThresholdHandler = [] (const Json::Value& item)
  {
    const Json::Value& attributes = item[kQuestRuleConditionAttributesKey];
    const std::string& operatorStr = attributes.get(kQuestRuleAttributeOperatorKey, "").asString();
    uint16_t targetValue = (uint16_t)attributes.get(kQuestRuleAttributeTargetValueKey, 0).asUInt();
    const std::string& triggerKey = attributes.get(kQuestRuleAttributeTriggerKeyKey, "").asString();
    
    CountThresholdOperator countOperator = CountThresholdOperatorEquals;
    if( operatorStr == "CountThresholdOperatorGreaterThan" ) {
      countOperator = CountThresholdOperatorGreaterThan;
    }else if( operatorStr == "CountThresholdOperatorLessThan" ) {
      countOperator = CountThresholdOperatorLessThan;
    }else if( operatorStr == "CountThresholdOperatorGreaterThanEqual" ) {
      countOperator = CountThresholdOperatorGreaterThanEqual;
    }else if( operatorStr == "CountThresholdOperatorLessThanEqual" ) {
      countOperator = CountThresholdOperatorLessThanEqual;
    }
    
    CountThresholdCondition* countThresholdCondition = new CountThresholdCondition(countOperator, targetValue, triggerKey);
    return countThresholdCondition;
  };
  
  const ConditionHandler dateHandler = [] (const Json::Value& item) {
    const Json::Value& attributes = item[kQuestRuleActionAttributesKey];
    const std::string& dateType = attributes.get("type", "").asString();
    
    DateCondition* dateCondition = nullptr;
    if( dateType == "week" ) {
      uint16_t dayOfWeek = attributes.get("day", 0).asInt();
      dateCondition = new DateCondition();
      dateCondition->SetDayOfWeek(dayOfWeek);
    }
    else if( dateType == "month" ) {
      uint16_t dayOfMonth = attributes.get("day", 0).asInt();
      dateCondition = new DateCondition();
      dateCondition->SetDayOfMonth(dayOfMonth);
    }
    return dateCondition;
  };
  
  const ConditionHandler timeHandler = [] (const Json::Value& item) {
    const Json::Value& attributes = item[kQuestRuleActionAttributesKey];
    
    TimeCondition* timeCondition = nullptr;
    uint64_t startTime = attributes.get("startTime", 0).asInt64();
    uint64_t stopTime = attributes.get("stopTime", 0).asInt64();
    if( startTime != 0 && stopTime == 0 ) {
      timeCondition = new TimeCondition();
      timeCondition->SetStartTime(startTime);
    }
    else if( startTime == 0 && stopTime != 0 ) {
      timeCondition = new TimeCondition();
      timeCondition->SetStopTime(stopTime);
    }
    else if( startTime != 0 && stopTime != 0 ) {
      timeCondition = new TimeCondition();
      timeCondition->SetRange(startTime, stopTime);
    }
    return timeCondition;
  };

  AddConditionHandler("Combination", combinationHandler);
  AddConditionHandler("Repeat", repeatHandler);
  AddConditionHandler("CountThreshold", countThresholdHandler);
  AddConditionHandler("Date", dateHandler);
  AddConditionHandler("Time", timeHandler);
}

  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki
