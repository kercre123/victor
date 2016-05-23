//
//  testRepeatCondition.cpp
//  util
//
//  Created by Aubrey Goodman on 8/10/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/repeatCondition.h"
#include "util/questEngine/combinationCondition.h"
#include "util/questEngine/noticeAction.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class RepeatConditionTest : public FileHelper
{
public:
  
  static void SetUpTestCase() {}
  
  static void TearDownTestCase() {}
  
};

TEST_F(RepeatConditionTest, NoRepeatTest)
{
  QuestEngine questEngine("stats.dat");
  
  RepeatCondition* condition = new RepeatCondition("test.repeat.none", 100);
  CombinationCondition* combo = new CombinationCondition(CombinationConditionOperationNot, condition);
  
  std::vector<std::string> eventNames;
  eventNames.push_back("game.event");
  NoticeAction* action = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.repeat.none", eventNames, "test.repeat.none.title", "test.repeat.none.description", "", "", "", nullptr, combo, action);
  rule->SetRepeatable(true);
  questEngine.AddRule(rule);

  std::time_t now = std::time(nullptr);
  std::tm localNow = *std::localtime(&now);
  bool satisfied = condition->IsSatisfied(questEngine, localNow);
  EXPECT_TRUE(!satisfied);

  questEngine.IncrementEvent("game.event");
  
  localNow.tm_sec += 1;
  std::mktime(&localNow);
  
  satisfied = condition->IsSatisfied(questEngine, localNow);
  EXPECT_TRUE(satisfied);
}

TEST_F(RepeatConditionTest, RepeatTest)
{
  QuestEngine questEngine("stats.dat");
  
  RepeatCondition* condition = new RepeatCondition("test.repeat.one", 100);
  CombinationCondition* combo = new CombinationCondition(CombinationConditionOperationNot, condition);
  
  std::vector<std::string> eventNames;
  eventNames.push_back("game.event");
  NoticeAction* action = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.repeat.one", eventNames, "test.repeat.one.title", "test.repeat.one.description", "", "", "", nullptr, combo, action);
  rule->SetRepeatable(true);
  questEngine.AddRule(rule);
  
  std::time_t now = std::time(nullptr);
  std::tm localNow = *std::localtime(&now);
  bool satisfied = condition->IsSatisfied(questEngine, localNow);
  EXPECT_TRUE(!satisfied);
  
  questEngine.IncrementEvent("game.event");

  satisfied = condition->IsSatisfied(questEngine, localNow);
  EXPECT_TRUE(satisfied);

  localNow.tm_sec += 101;
  std::mktime(&localNow);
  
  satisfied = condition->IsSatisfied(questEngine, localNow);
  EXPECT_TRUE(!satisfied);
}
