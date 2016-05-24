//
//  testQuestEngine.cpp
//  driveEngine
//
//  Created by Aubrey Goodman on 5/4/15.
//
//

#include "util/questEngine/questEngine.h"
#include "util/questEngine/countThresholdCondition.h"
#include "util/questEngine/noticeAction.h"
#include "util/questEngine/combinationCondition.h"
#include "util/questEngine/multiAction.h"
#include "util/questEngine/resetAction.h"
#include "util/questEngine/questNotice.h"
#include "util/questEngine/questRule.h"
#include "util/fileUtils/fileUtils.h"
#include "utilUnitTest/tests/testFileHelper.h"

using namespace Anki::Util::QuestEngine;

class QuestEngineTest : public FileHelper
{
public:
  
  static std::string StatFile()
  {
    std::string statFile;
    GetWorkroot(statFile);
    statFile += "/stats.dat";
    return statFile;
  }
  
  static void SetUpTestCase() {
    Anki::Util::FileUtils::DeleteFile(StatFile());
    FileShouldNotExist(StatFile());
  }
  
  static void TearDownTestCase() {}
  
};

TEST_F(QuestEngineTest, IncrementSingleEvent)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  // win twice
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  
  EXPECT_EQ(2, questEngine->GetEventCount("game.win"));

  delete questEngine;
}

TEST_F(QuestEngineTest, IncrementMultipleEvents)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  // win twice
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  
  // lose twice
  questEngine->IncrementEvent("game.lose");
  questEngine->IncrementEvent("game.lose");
  
  EXPECT_EQ(2, questEngine->GetEventCount("game.win"));
  EXPECT_EQ(2, questEngine->GetEventCount("game.lose"));
  
  delete questEngine;
}

TEST_F(QuestEngineTest, Save)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  // win twice
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  
  // lose twice
  questEngine->IncrementEvent("game.lose");
  questEngine->IncrementEvent("game.lose");
  
  questEngine->Save();
  
  FileShouldExist(StatFile());
  
  delete questEngine;
}

TEST_F(QuestEngineTest, SaveAndLoad)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  CountThresholdCondition* limitThree = new CountThresholdCondition(CountThresholdOperatorEquals, 2, "game.win");
  std::vector<std::string> eventNames;
  eventNames.push_back("game.win");
  NoticeAction* triggeredAction = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.save.load", eventNames, "test.save.load.title", "test.save.load.description", "", "", "", nullptr, limitThree, triggeredAction);
  questEngine->AddRule(rule);
  
  // win twice
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  
  // lose twice
  questEngine->IncrementEvent("game.lose");
  questEngine->IncrementEvent("game.lose");
  
  std::set<QuestNotice> notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  EXPECT_TRUE(questEngine->HasTriggered("test.save.load"));
  
  questEngine->Save();
  FileShouldExist(StatFile());
  
  delete questEngine;
  
  questEngine = new QuestEngine(StatFile());
  questEngine->Load();
  
  EXPECT_EQ(2, questEngine->GetEventCount("game.win"));
  EXPECT_EQ(2, questEngine->GetEventCount("game.lose"));
  
  std::set<QuestNotice> loadedNotices = questEngine->GetNotices();
  EXPECT_EQ(1, loadedNotices.size());
  EXPECT_TRUE(questEngine->HasTriggered("test.save.load"));
  
  delete questEngine;
  
  Anki::Util::FileUtils::DeleteFile(StatFile());
  FileShouldNotExist(StatFile());
}

TEST_F(QuestEngineTest, SaveLoadReset)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  // win twice
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  
  // lose twice
  questEngine->IncrementEvent("game.lose");
  questEngine->IncrementEvent("game.lose");
  
  questEngine->Save();
  FileShouldExist(StatFile());
  
  delete questEngine;
  
  questEngine = new QuestEngine(StatFile());
  questEngine->Load();
  
  EXPECT_EQ(2, questEngine->GetEventCount("game.win"));
  EXPECT_EQ(2, questEngine->GetEventCount("game.lose"));
  
  questEngine->ResetEvent("game.win");
  EXPECT_EQ(0, questEngine->GetEventCount("game.win"));
  EXPECT_EQ(2, questEngine->GetEventCount("game.lose"));
  
  delete questEngine;
  
  Anki::Util::FileUtils::DeleteFile(StatFile());
  FileShouldNotExist(StatFile());
}

TEST_F(QuestEngineTest, Clear)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
 
  std::string triggerKey("game.event");
  std::vector<std::string> eventNames;
  eventNames.push_back(triggerKey);
  NoticeAction* triggeredAction = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  CountThresholdCondition* limitThree = new CountThresholdCondition(CountThresholdOperatorEquals, 3, triggerKey);
  QuestRule* rule = new QuestRule("test.clear", eventNames, "test.clear.title", "test.clear.description", "", "", "", nullptr, limitThree, triggeredAction);
  questEngine->AddRule(rule);

  questEngine->ClearRules();
  
  delete questEngine;
}

TEST_F(QuestEngineTest, CountThresholdRule)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string triggerKey("game.win");
  std::vector<std::string> eventNames;
  eventNames.push_back(triggerKey);
  
  NoticeAction* triggeredAction = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  CountThresholdCondition* limitThree = new CountThresholdCondition(CountThresholdOperatorEquals, 3, triggerKey);
  QuestRule* rule = new QuestRule("test.count", eventNames, "test.count.title", "test.count.description", "", "", "", nullptr, limitThree, triggeredAction);
  questEngine->AddRule(rule);
  
  questEngine->IncrementEvent("game.win");
  const std::set<QuestNotice>& emptyNotices = questEngine->GetNotices();
  EXPECT_EQ(0, emptyNotices.size());
  
  questEngine->IncrementEvent("game.win");
  const std::set<QuestNotice>& moreEmptyNotices = questEngine->GetNotices();
  EXPECT_EQ(0, moreEmptyNotices.size());

  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.win");
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  std::vector<QuestNotice> allNotices(notices.begin(), notices.end());
  const std::string& titleKey = allNotices[0].GetTitleKey();
  EXPECT_EQ("results.notice.title", titleKey);

  delete questEngine;
}

TEST_F(QuestEngineTest, CombinationRule)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string winKey("game.win");
  std::string starKey("game.star");
  
  std::vector<std::string> eventNames;
  eventNames.push_back(winKey);
  eventNames.push_back(starKey);
  
  CountThresholdCondition* winCondition = new CountThresholdCondition(CountThresholdOperatorGreaterThanEqual, 1, winKey);
  
  CountThresholdCondition* starCondition = new CountThresholdCondition(CountThresholdOperatorEquals, 1, starKey);

  CombinationCondition* oneWinThreeStars = new CombinationCondition(CombinationConditionOperationAnd, winCondition, starCondition);

  NoticeAction* triggeredAction = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.combination.rule", eventNames, "test.combination.rule.title", "test.combination.rule.description", "", "", "", nullptr, oneWinThreeStars, triggeredAction);
  questEngine->AddRule(rule);

  questEngine->IncrementEvent("game.win");
  const std::set<QuestNotice>& emptyNotices = questEngine->GetNotices();
  EXPECT_EQ(0, emptyNotices.size());
  
  questEngine->IncrementEvent("game.win");
  questEngine->IncrementEvent("game.star");
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  std::vector<QuestNotice> allNotices(notices.begin(), notices.end());
  const std::string& titleKey = allNotices[0].GetTitleKey();
  EXPECT_EQ("results.notice.title", titleKey);
  
  delete questEngine;
}

TEST_F(QuestEngineTest, DuplicateEvents)
{
  QuestEngine* questEngine = new QuestEngine(StatFile());
  
  std::string winKey("game.win");

  std::string startKey("game.start");
  std::vector<std::string> startEvents;
  startEvents.push_back(startKey);
  CountThresholdCondition* gameStart = new CountThresholdCondition(CountThresholdOperatorEquals, 1, startKey);
  ResetAction* resetWinAction = new ResetAction(winKey);
  ResetAction* resetStartAction = new ResetAction(startKey);
  std::vector<AbstractAction*> actions;
  actions.push_back(resetWinAction);
  actions.push_back(resetStartAction);
  MultiAction* startAction = new MultiAction(actions);
  QuestRule* resetStartOnStart = new QuestRule("test.duplicate.one", startEvents, "test.duplicate.one.title", "test.duplicate.one.description", "", "", "", nullptr, gameStart, startAction);
  questEngine->AddRule(resetStartOnStart);

  std::vector<std::string> winEvents;
  winEvents.push_back(winKey);
  CountThresholdCondition* oneWin = new CountThresholdCondition(CountThresholdOperatorEquals, 1, winKey);
  NoticeAction* triggeredAction = new NoticeAction(false, 0, "", "results.notice.title", "results.notice.description", "", "");
  QuestRule* rule = new QuestRule("test.duplicate.two", winEvents, "test.duplicate.two.title", "test.duplicate.two.description", "", "", "", nullptr, oneWin, triggeredAction);
  questEngine->AddRule(rule);
  
  questEngine->IncrementEvent(startKey);
  questEngine->IncrementEvent(winKey);
  const std::set<QuestNotice>& notices = questEngine->GetNotices();
  EXPECT_EQ(1, notices.size());
  
  std::vector<QuestNotice> allNotices;
  std::copy(notices.begin(), notices.end(), std::back_inserter(allNotices));
  
  questEngine->ConsumeNotice(allNotices[0]);
  const std::set<QuestNotice>& emptyNotices = questEngine->GetNotices();
  EXPECT_EQ(0, emptyNotices.size());
  
  questEngine->IncrementEvent(startKey);
  questEngine->IncrementEvent(winKey);
  const std::set<QuestNotice>& moreNotices = questEngine->GetNotices();
  EXPECT_EQ(0, moreNotices.size());
  
  delete questEngine;
}
