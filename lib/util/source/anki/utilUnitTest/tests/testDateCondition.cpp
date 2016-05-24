//
//  testDateCondition.cpp
//  util
//
//  Created by Aubrey Goodman on 7/20/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRule.h"
#include "util/questEngine/dateCondition.h"
#include "util/questEngine/noticeAction.h"
#include "util/questEngine/dateCondition.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class DateConditionTest : public FileHelper
{
public:
  
  static void SetUpTestCase() {}
  
  static void TearDownTestCase() {}
  
};

TEST_F(DateConditionTest, DayOfWeek)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");
  
  DateCondition* dateTuesday = new DateCondition();
  dateTuesday->SetDayOfWeek(2);
  
  std::time_t now = std::time(nullptr);
  std::tm* tues = std::localtime(&now);
  tues->tm_wday = 2;
  bool satisfied = dateTuesday->IsSatisfied(*questEngine, *tues);

  EXPECT_TRUE(satisfied);

  tues->tm_wday = 3;
  satisfied = dateTuesday->IsSatisfied(*questEngine, *tues);

  EXPECT_TRUE(!satisfied);
  
  delete dateTuesday;
  delete questEngine;
}

TEST_F(DateConditionTest, DayOfMonth)
{
  QuestEngine* questEngine = new QuestEngine("stats.dat");
  
  DateCondition* dateTenth = new DateCondition();
  dateTenth->SetDayOfMonth(10);
  
  std::time_t now = std::time(nullptr);
  std::tm* tues = std::localtime(&now);
  tues->tm_mday = 10;
  bool satisfied = dateTenth->IsSatisfied(*questEngine, *tues);
  
  EXPECT_TRUE(satisfied);

  tues->tm_mday = 15;
  satisfied = dateTenth->IsSatisfied(*questEngine, *tues);
  
  EXPECT_TRUE(!satisfied);

  delete dateTenth;
  delete questEngine;
}
