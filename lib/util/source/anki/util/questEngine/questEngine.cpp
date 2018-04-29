/**
 * File: questEngine.cpp
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

#include "util/questEngine/questEngine.h"
#include "util/questEngine/abstractAction.h"
#include "util/questEngine/abstractCondition.h"
#include "util/questEngine/questEngineMessages.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/questNotice.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <fstream>
#include <algorithm>

namespace Anki {
namespace Util {
namespace QuestEngine {

const uint8_t kPersistentFileMarker = 7;

QuestEngine::QuestEngine(const std::string& fileName)
: _persistentStoreFilename(fileName)
, _eventSignal(new EventSignal())
, _addedSignal(new RuleAddedSignal())
, _availableSignal(new RuleAvailableSignal())
, _triggeredSignal(new RuleTriggeredSignal())
{
}

QuestEngine::~QuestEngine()
{
  for (auto it = _rules.begin(); it != _rules.end(); ++it) {
    delete (*it);
  }
  SafeDelete(_eventSignal);
  SafeDelete(_addedSignal);
  SafeDelete(_availableSignal);
  SafeDelete(_triggeredSignal);
}

void QuestEngine::IncrementEvent(const std::string& eventName, const uint16_t count)
{
  _countMap[eventName] += count;
  ProcessRules(eventName);
}

void QuestEngine::ResetEvent(const std::string& eventName)
{
  _countMap.erase(eventName);
}

uint32_t QuestEngine::GetEventCount(const std::string& eventName)
{
  const auto it = _countMap.find(eventName);

  if( it != _countMap.end() ) {
    return it->second;
  }

  return 0;
}

void QuestEngine::Load()
{
  std::vector<uint8_t> body;
  std::ifstream fileIn;
  bool success = false;
  fileIn.open(_persistentStoreFilename, std::ios::in | std::ifstream::binary);
  if( fileIn.is_open() ) {
    fileIn.seekg(0, fileIn.end);
    size_t len = (size_t) fileIn.tellg();
    body.resize(len);
    fileIn.seekg(0, fileIn.beg);
    fileIn.read((char*)(body.data()), len);
    fileIn.close();
    success = true;
  }

  if( success && body.size() > 0 ) {
    uint8_t marker = body[0];
    if( marker != kPersistentFileMarker ) {
      Anki::Util::FileUtils::DeleteFile(_persistentStoreFilename);
    }
    else {
      uint8_t* buffer = body.data();
      QuestStore store;
      store.Unpack(buffer+1, body.size()-1);

      _countMap.clear();
      for (auto eventIterator = store.eventStats.begin(); eventIterator != store.eventStats.end(); ++eventIterator) {
        const std::string& eventName = eventIterator->eventName;
        _countMap[eventName] = eventIterator->count;
      }

      _notices.clear();
      for (auto noticeIterator = store.notices.begin(); noticeIterator != store.notices.end(); ++noticeIterator) {
        const bool noticeForce = noticeIterator->force;
        const uint8_t noticePriority = noticeIterator->priority;
        const double noticeTime = noticeIterator->noticeTime;
        const std::string& guid = noticeIterator->guid;
        const std::string& target = noticeIterator->target;
        const std::string& titleKey = noticeIterator->titleKey;
        const std::string& descriptionKey = noticeIterator->descriptionKey;
        const std::string& buttonKey = noticeIterator->buttonKey;
        const std::string& navigationAction = noticeIterator->navigationAction;
        QuestNotice questNotice(noticeForce, noticePriority, noticeTime, guid, target, titleKey, descriptionKey, buttonKey, navigationAction);
        _notices.insert(questNotice);
      }

      _triggerHistoryMap.clear();
      for (auto triggerIterator = store.ruleTriggers.begin(); triggerIterator != store.ruleTriggers.end(); ++triggerIterator) {
        const std::string& ruleId = triggerIterator->ruleId;
        const std::vector<double>& log = triggerIterator->triggerTimes;
        for (auto historyIterator = log.begin(); historyIterator != log.end(); ++historyIterator) {
          _triggerHistoryMap[ruleId].push_back(*historyIterator);
        }
      }
    }
  }else{
    PRINT_NAMED_WARNING("QuestEngine.Load", "Failed!");
  }
}

void QuestEngine::Save()
{
  QuestStore store;
  for (auto eventIterator = _countMap.begin(); eventIterator != _countMap.end(); ++eventIterator) {
    store.eventStats.emplace_back(eventIterator->first, eventIterator->second);
  }
  for (auto noticeIterator = _notices.begin(); noticeIterator != _notices.end(); ++noticeIterator) {
    const QuestNotice& questNotice = *noticeIterator;
    store.notices.emplace_back(questNotice.IsForce(), questNotice.GetPriority(), questNotice.GetNoticeTime(), questNotice.GetGuid(), questNotice.GetTarget(), questNotice.GetTitleKey(), questNotice.GetDescriptionKey(), questNotice.GetButtonKey(), questNotice.GetNavigationAction());
  }
  for (auto triggerIterator = _triggerHistoryMap.begin(); triggerIterator != _triggerHistoryMap.end(); ++triggerIterator) {
    TriggerLog log;
    log.ruleId = triggerIterator->first;
    const TriggerHistory& history = triggerIterator->second;
    for (auto historyIterator = history.begin(); historyIterator != history.end(); ++historyIterator) {
      log.triggerTimes.push_back(*historyIterator);
    }
    store.ruleTriggers.push_back(log);
  }

  size_t bufferSize = store.Size()+1;
  uint8_t* buffer = new uint8_t[bufferSize];
  buffer[0] = kPersistentFileMarker;
  store.Pack(buffer+1, store.Size());

  Util::sInfo("quest_engine.save", {}, std::to_string(store.Size()).c_str());

  // write file
  bool success = false;
  std::ofstream fileOut;
  fileOut.open(_persistentStoreFilename, std::ios::out | std::ofstream::binary);
  if( fileOut.is_open() ) {
    std::copy(buffer, buffer + bufferSize, std::ostreambuf_iterator<char>(fileOut));
    fileOut.close();
    success = true;
  }
  delete[] buffer;

  if( !success ) {
    PRINT_NAMED_ERROR("QuestEngine.Save", "Failed!");
  }
}

bool QuestEngine::AddRule(QuestRule* rule)
{
  assert(nullptr != rule);

  const std::vector<std::string>& eventNames = rule->GetEventNames();
  const std::string& ruleId = rule->GetId();
  if( !eventNames.empty() && !ruleId.empty() && _ruleIds.find(ruleId) == _ruleIds.end() ) {
    _rules.push_back(rule);
    _ruleIds.insert(ruleId);
    for (auto eventNamesIterator = eventNames.begin(); eventNamesIterator != eventNames.end(); ++eventNamesIterator) {
      const auto eventRulesIterator = _eventRulesMap.find(*eventNamesIterator);
      if( eventRulesIterator == _eventRulesMap.end() ) {
        std::vector<QuestRule*> eventRules;
        eventRules.push_back(rule);
        _eventRulesMap.emplace( *eventNamesIterator,std::move(eventRules) );
      }else{
        std::vector<QuestRule*>& eventRules = eventRulesIterator->second;
        eventRules.push_back(rule);
      }

    }
    _addedSignal->emit(*rule);
    return true;
  }
  return false;
}

void QuestEngine::ClearRules()
{
  for (auto it = _rules.begin(); it != _rules.end(); ++it) {
    delete *it;
  }
  _rules.clear();
}

const std::vector<Util::QuestEngine::QuestRule*> QuestEngine::GetAvailableRules(std::tm &time)
{
  std::vector<Util::QuestEngine::QuestRule*> availableRules;
  for (auto* rule : _rules) {
    if( (rule->IsRepeatable() || !HasTriggered(rule->GetId())) &&
       rule->IsAvailable(*this, time) ) {
      availableRules.push_back(rule);
    }
  }
  return availableRules;
}

void QuestEngine::AddNotice(const QuestNotice& questNotice)
{
  _notices.insert(questNotice);
}

void QuestEngine::ConsumeNotice(const QuestNotice& questNotice)
{
  auto it = _notices.find(questNotice);
  if( it != _notices.end() ) {
    _notices.erase(it);
  }
}

bool QuestEngine::HasTriggered(const std::string& ruleId)
{
  const auto& found = _triggerHistoryMap.find(ruleId);
  return found != _triggerHistoryMap.end();
}

const std::time_t& QuestEngine::LastTriggeredAt(const std::string& ruleId)
{
  const TriggerHistory& ruleHistory = _triggerHistoryMap[ruleId];
  return ruleHistory.at(ruleHistory.size()-1);
}



// Private methods

void QuestEngine::AppendTriggerHistory(const std::string& ruleId, const time_t triggerTime)
{
  TriggerHistory& ruleHistory = _triggerHistoryMap[ruleId];
  ruleHistory.push_back(triggerTime);
}

void QuestEngine::ProcessRules(const std::string& eventName)
{
  auto eventRulesIterator = _eventRulesMap.find(eventName);
  std::time_t now = std::time(nullptr);
  std::tm localNow = *std::localtime(&now);
  if( eventRulesIterator != _eventRulesMap.end() ) {
    _eventSignal->emit(eventName);
    const std::vector<QuestRule*>& rules = eventRulesIterator->second;
    for (auto it = rules.begin(); it != rules.end(); ++it) {
      bool isRepeatable = (*it)->IsRepeatable();
      bool hasTriggered = HasTriggered((*it)->GetId());
      if( !isRepeatable && hasTriggered ) {
        continue;
      }

      bool isAvailable = (*it)->IsAvailable(*this, localNow);
      if( !isAvailable ) {
        _availableSignal->emit(**it, false);
        continue;
      }
      _availableSignal->emit(**it, true);

      bool isTriggered = (*it)->IsTriggered(*this, localNow);
      if( !isTriggered ) {
        continue;
      }
      _triggeredSignal->emit(**it);

      AbstractAction* action = (*it)->GetAction();
      action->PerformAction(*this);
      AppendTriggerHistory((*it)->GetId(), now);
    }
  }
}


} // namespace QuestEngine
} // namespace Util
} // namespace Anki
