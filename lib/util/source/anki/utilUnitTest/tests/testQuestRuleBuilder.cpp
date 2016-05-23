//
//  testQuestRuleBuilder.cpp
//  util
//
//  Created by Aubrey Goodman on 5/11/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/questRuleBuilder.h"
#include "util/questEngine/questNotice.h"
#include "util/questEngine/abstractAction.h"
#include "util/questEngine/abstractCondition.h"
#include "util/fileUtils/fileUtils.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class QuestRuleBuilderTest : public FileHelper
{
public:
  
  static std::string StatFile()
  {
    std::string statFile;
    GetWorkroot(statFile);
    statFile += "/stats.dat";
    return statFile;
  }
  
  static std::string RulesFile()
  {
    std::string rulesFile;
    GetWorkroot(rulesFile);
    rulesFile += "/rules.json";
    return rulesFile;
  }
  
  static void SetUpTestCase() {
    Anki::Util::FileUtils::DeleteFile(StatFile());
  }
  
  static void TearDownTestCase() {
    Anki::Util::FileUtils::DeleteFile(StatFile());
    Anki::Util::FileUtils::DeleteFile(RulesFile());
  }

};

TEST_F(QuestRuleBuilderTest, LoadDefinitions)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.load.definitions\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}},\"condition\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":5,\"triggerKey\":\"game.end\"}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(success);
  
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(0, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(0, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(0, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(0, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  std::vector<QuestNotice> allNotices(notices.begin(), notices.end());
  EXPECT_EQ("home.notice.title", allNotices[0].GetTitleKey());
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, EmptyFile)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString;
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(!success);
  EXPECT_EQ(0, questEngine->GetRules().size());
  
  delete builder;
  delete questEngine;
}

// intermittent failures, disabled by Damjan 5/9/2016, please fix
TEST_F(QuestRuleBuilderTest, DISABLED_RepeatCondition)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.repeat.condition\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}},\"condition\":{\"type\":\"Combination\",\"attributes\":{\"operation\":\"AND\",\"conditionA\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":3,\"triggerKey\":\"game.end\"}},\"conditionB\":{\"type\":\"Combination\",\"attributes\":{\"operation\":\"NOT\",\"conditionA\":{\"type\":\"Repeat\",\"attributes\":{\"noticeKey\":\"home.notice.tires\",\"secondsSinceLast\":86400}}}}}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(success);
  
  time_t tNow = std::time(NULL);
  QuestNotice previousNotice(false, 0, tNow, "", "", "home.notice.title", "home.notice.description", "", "");
  questEngine->AddNotice(previousNotice);
  EXPECT_EQ(1, questEngine->GetNotices().size());
  
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(1, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(1, questEngine->GetNotices().size());

  // this should not add a notice, since the time between events is less than the delta time in the repeat condition
  questEngine->IncrementEvent("game.end");
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  
  if( notices.size() == 1 ) {
    std::vector<QuestNotice> allNotices(notices.begin(), notices.end());
    EXPECT_EQ("home.notice.title", allNotices[0].GetTitleKey());
  }
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, ExpiredRepeatCondition)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.expired.repeat.condition\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}},\"condition\":{\"type\":\"Combination\",\"attributes\":{\"operation\":\"AND\",\"conditionA\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":3,\"triggerKey\":\"game.end\"}},\"conditionB\":{\"type\":\"Combination\",\"attributes\":{\"operation\":\"NOT\",\"conditionA\":{\"type\":\"Repeat\",\"attributes\":{\"noticeKey\":\"home.notice.tires\",\"secondsSinceLast\":86400}}}}}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(success);
  
  time_t tNow = std::time(NULL) - 86400;
  QuestNotice previousNotice(false, 0, tNow, "", "", "home.notice.title", "home.notice.description", "", "");
  questEngine->AddNotice(previousNotice);
  EXPECT_EQ(1, questEngine->GetNotices().size());
  
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(1, questEngine->GetNotices().size());
  questEngine->IncrementEvent("game.end");
  EXPECT_EQ(1, questEngine->GetNotices().size());
  
  // this should add a notice, since the time between events is greater than the delta time in the repeat condition
  questEngine->IncrementEvent("game.end");
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(2, notices.size());
  
  if( notices.size() == 2 ) {
    std::vector<QuestNotice> allNotices(notices.begin(), notices.end());
    EXPECT_EQ("home.notice.title", allNotices[0].GetTitleKey());
    EXPECT_EQ("home.notice.title", allNotices[1].GetTitleKey());
  }
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, InvalidJson)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.invalid.json\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}},\"condition\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":5,\"triggerKey\":\"game.end\",,,,}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(!success);
  EXPECT_EQ(0, questEngine->GetRules().size());
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, EmptyRule)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(!success);
  EXPECT_EQ(0, questEngine->GetRules().size());
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, EmptyCondition)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.empty.condition\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(success);
  EXPECT_EQ(1, questEngine->GetRules().size());
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, EmptyActions)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.empty.action\",\"eventNames\":[\"game.end\"],\"condition\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":5,\"triggerKey\":\"game.end\"}}}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(!success);
  EXPECT_EQ(0, questEngine->GetRules().size());
  
  delete builder;
  delete questEngine;
}

TEST_F(QuestRuleBuilderTest, PartialValidRules)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string jsonString("{\"rules\":[{\"id\":\"test.partial.valid.rules\",\"eventNames\":[\"game.end\"],\"action\":{\"type\":\"Notice\",\"attributes\":{\"titleKey\":\"home.notice.title\",\"descriptionKey\":\"home.notice.description\"}},\"condition\":{\"type\":\"CountThreshold\",\"attributes\":{\"operator\":\"CountThresholdOperatorEquals\",\"targetValue\":5,\"triggerKey\":\"game.end\"}}},{}]}");
  Anki::Util::FileUtils::WriteFile(RulesFile(), jsonString);
  
  QuestRuleBuilder* builder = new QuestRuleBuilder();
  bool success = builder->LoadDefinitions(RulesFile(), *questEngine);
  EXPECT_TRUE(!success);
  
  delete builder;
  delete questEngine;
}
