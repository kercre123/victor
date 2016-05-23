//
//  testTimeCondition.cpp
//  util
//
//  Created by Aubrey Goodman on 7/20/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/timeCondition.h"
#include "util/questEngine/noticeAction.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class TimeConditionTest : public FileHelper
{
public:
  
  static void SetUpTestCase() {}
  
  static void TearDownTestCase() {}
  
};

TEST_F(TimeConditionTest, LessThanTime)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");
  
  std::time_t now = std::time(nullptr);

  TimeCondition* timeLimit = new TimeCondition();
  timeLimit->SetStopTime(now);
  
  std::tm* maxTime = std::localtime(&now);
  maxTime->tm_sec -= 10;
  std::mktime(maxTime);
  
  bool satisfied = timeLimit->IsSatisfied(*questEngine, *maxTime);
  
  EXPECT_TRUE(satisfied);
  
  maxTime->tm_sec += 20;
  std::mktime(maxTime);
  
  satisfied = timeLimit->IsSatisfied(*questEngine, *maxTime);
  
  EXPECT_TRUE(!satisfied);
  
  delete timeLimit;
  delete questEngine;
}

TEST_F(TimeConditionTest, GreaterThanTime)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");
  
  std::time_t now = std::time(nullptr);
  
  TimeCondition* timeLimit = new TimeCondition();
  timeLimit->SetStartTime(now);
  
  std::tm* maxTime = std::localtime(&now);
  maxTime->tm_sec += 10;
  std::mktime(maxTime);
  
  bool satisfied = timeLimit->IsSatisfied(*questEngine, *maxTime);
  
  EXPECT_TRUE(satisfied);
  
  maxTime->tm_sec -= 20;
  std::mktime(maxTime);
  
  satisfied = timeLimit->IsSatisfied(*questEngine, *maxTime);
  
  EXPECT_TRUE(!satisfied);
  
  delete timeLimit;
  delete questEngine;
}
