//
//  testCombinationCondition.cpp
//  util
//
//  Created by Aubrey Goodman on 5/14/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/countThresholdCondition.h"
#include "util/questEngine/combinationCondition.h"
#include "util/questEngine/noticeAction.h"
#include "util/questEngine/dateCondition.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class CombinationConditionTest : public FileHelper
{
public:
  
  static void SetUpTestCase() {}
  
  static void TearDownTestCase() {}
  
};

TEST_F(CombinationConditionTest, AndTest)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");

  CountThresholdCondition* countOne = new CountThresholdCondition(CountThresholdOperatorEquals, 1, "game.event");
  DateCondition* dateTuesday = new DateCondition();
  dateTuesday->SetDayOfWeek(2);
  
  CombinationCondition* combo = new CombinationCondition(CombinationConditionOperationAnd, countOne, dateTuesday);
  
  std::vector<std::string> eventNames;
  eventNames.push_back("game.event");
  NoticeAction* action = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.combination.and", eventNames, "test.combination.and.title", "test.combination.and.description", "", "","", nullptr, combo, action);
  questEngine->AddRule(rule);
  
  questEngine->IncrementEvent("game.event");
  std::time_t now = std::time(nullptr);
  std::tm tues = *std::localtime(&now);
  tues.tm_wday = 2;
  bool satisfied = combo->IsSatisfied(*questEngine, tues);
  
  EXPECT_TRUE(satisfied);
  
  delete questEngine;
}

TEST_F(CombinationConditionTest, OrTest)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");
  
  CountThresholdCondition* countOne = new CountThresholdCondition(CountThresholdOperatorEquals, 1, "game.event");
  DateCondition* dateTuesday = new DateCondition();
  dateTuesday->SetDayOfWeek(2);
  
  CombinationCondition* combo = new CombinationCondition(CombinationConditionOperationOr, countOne, dateTuesday);
  
  std::vector<std::string> eventNames;
  eventNames.push_back("game.event");
  NoticeAction* action = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.combination.or", eventNames, "test.combination.or.title", "test.combination.or.description", "", "", "", nullptr, combo, action);
  questEngine->AddRule(rule);
  
  bool satisfied;
  
  std::time_t now = std::time(nullptr);
  std::tm tues = *std::localtime(&now);
  tues.tm_wday = 1; // monday
  
  satisfied = dateTuesday->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(!satisfied);
  
  satisfied = countOne->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(!satisfied);

  satisfied = combo->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(!satisfied);
  
  tues.tm_wday = 2; // tuesday
  satisfied = dateTuesday->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(satisfied);

  satisfied = combo->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(satisfied);
  
  questEngine->IncrementEvent("game.event"); // count is 1
  satisfied = countOne->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(satisfied);
  
  satisfied = combo->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(satisfied);
  
  tues.tm_wday = 1; // monday
  questEngine->IncrementEvent("game.event"); // count is 2
  satisfied = combo->IsSatisfied(*questEngine, tues);
  EXPECT_TRUE(!satisfied);
  
  delete questEngine;
}
