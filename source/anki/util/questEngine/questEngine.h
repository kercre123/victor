/**
 * File: questEngine.h
 *
 * Author: aubrey
 * Created: 05/04/15
 *
 * Description: Responsible for processing events and triggering quest behavior
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_QuestEngine_H__
#define __Util_QuestEngine_QuestEngine_H__

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <ctime>

namespace Anki {
namespace Util {
namespace QuestEngine {

using TriggerHistory = std::vector<std::time_t>;

class QuestRule;
class QuestNotice;

class QuestEngine : public noncopyable {

public:
  
  using EventSignal = Signal::Signal<void(const std::string&)>;
  using RuleAddedSignal = Signal::Signal<void(QuestRule&)>;
  using RuleAvailableSignal = Signal::Signal<void(QuestRule&,bool)>;
  using RuleTriggeredSignal = Signal::Signal<void(QuestRule&)>;
  
  explicit QuestEngine(const std::string& fileName);
  ~QuestEngine();
  
  void Load();
  
  void Save();
  
  
  void IncrementEvent(const std::string& eventName, const uint16_t count = 1);
  
  void ResetEvent(const std::string& eventName);
  
  uint32_t GetEventCount(const std::string& eventName);
  
  
  void AddNotice(const QuestNotice& QuestNotice);
  
  const std::set<QuestNotice>& GetNotices() const { return _notices; }
  
  void ConsumeNotice(const QuestNotice& questNotice);
  
  
  bool AddRule(QuestRule* rule);
  
  void ClearRules();
  
  const std::vector<QuestRule*> GetAvailableRules(std::tm& time);
  
  const std::vector<QuestRule*>& GetRules() const { return _rules; }
  
  const std::set<std::string>& GetRuleIds() const { return _ruleIds; }
  
  bool HasTriggered(const std::string& ruleId);

  const std::time_t& LastTriggeredAt(const std::string& ruleId);
  
  
  EventSignal* GetEventSignal() const { return _eventSignal; };
  
  RuleAddedSignal* GetRuleAddedSignal() const { return _addedSignal; };
  
  RuleAvailableSignal* GetRuleAvailableSignal() const { return _availableSignal; };
  
  RuleTriggeredSignal* GetRuleTriggeredSignal() const { return _triggeredSignal; };
  
  
private:
  
  void AppendTriggerHistory(const std::string& ruleId, const std::time_t triggerTime);
  
  void ProcessRules(const std::string& eventName);
  

  std::map<std::string,uint32_t> _countMap;
  
  std::map<std::string,std::vector<QuestRule*>> _eventRulesMap;
  
  std::set<QuestNotice> _notices;
  
  std::string _persistentStoreFilename;
  
  std::vector<QuestRule*> _rules;
  
  std::set<std::string> _ruleIds;
  
  std::map<std::string,TriggerHistory> _triggerHistoryMap;
  
  EventSignal* _eventSignal;
  
  RuleAddedSignal* _addedSignal;
  
  RuleAvailableSignal* _availableSignal;
  
  RuleTriggeredSignal* _triggeredSignal;
  
};
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki


#endif // __Util_QuestEngine_QuestEngine_H__
