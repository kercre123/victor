/**
 * File: testBeiConditions.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-16
 *
 * Description: Unit tests for BEI Conditions
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/aiComponent/beiConditions/conditions/conditionNot.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/moodSystem/moodManager.h"
#include "util/math/math.h"

using namespace Anki;
using namespace Anki::Cozmo;

namespace {

void CreateBEI(const std::string& json, IBEIConditionPtr& cond)
{
  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  cond = BEIConditionFactory::CreateBEICondition(config);

  ASSERT_TRUE( cond != nullptr );
}

class TestCondition : public IBEICondition
{
public:
  explicit TestCondition()
    // use an arbitrary type to make the system happy
    : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::AlwaysRun))
    {
    }

  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) override {
    _resetCount++;
  }
  
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {
    _initCount++;
  }
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override {
    _areMetCount++;
    return _val;
  }

  bool _val = false;

  int _resetCount = 0;
  int _initCount = 0;
  mutable int _areMetCount = 0;  
};

}

TEST(BeiConditions, TestCondition)
{
  // a test of the test, if you will

  BehaviorExternalInterface bei;

  auto cond = std::make_shared<TestCondition>();

  EXPECT_EQ(cond->_resetCount, 0);
  EXPECT_EQ(cond->_initCount, 0);
  EXPECT_EQ(cond->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(cond->_resetCount, 0);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_areMetCount, 0);

  cond->Reset(bei);
  EXPECT_EQ(cond->_resetCount, 1);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_areMetCount, 0);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_resetCount, 1);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_areMetCount, 1);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_resetCount, 1);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_areMetCount, 2);

  cond->_val = true;

  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_resetCount, 1);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_areMetCount, 3);

}

TEST(BeiConditions, CreateLambda)
{
  bool val = false;
  
  auto cond = std::make_shared<ConditionLambda>(
    [&val](BehaviorExternalInterface& behaviorExternalInterface) {
      return val;
    });

  ASSERT_TRUE( cond != nullptr );

  BehaviorExternalInterface bei;
  // don't call init, should be ok if no one uses it
  // bei.Init();
  
  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  val = true;
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, True)
{
  const std::string json = R"json(
  {
    "conditionType": "AlwaysRun" 
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  BehaviorExternalInterface bei;
  
  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  cond->Reset(bei);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, Frustration)
{
  const std::string json = R"json(
  {
    "conditionType": "Frustration",
    "frustrationParams": {
      "maxConfidence": -0.5
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  BehaviorExternalInterface bei;

  MoodManager moodManager;
  bei.Init(nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           &moodManager,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr,
           nullptr);
  
  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  moodManager.SetEmotion(EmotionType::Confident, -1.0f);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  moodManager.SetEmotion(EmotionType::Confident, 1.0f);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, Timer)
{
  const std::string json = R"json(
  {
    "conditionType": "Timer",
    "timeout": 30.0
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  BehaviorExternalInterface bei;

  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(2.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(29.9));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(30.0001));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(35.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(65.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(900.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  cond->Reset(bei);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(920.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(930.1));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(9001.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, Not)
{
  BehaviorExternalInterface bei;

  auto subCond = std::make_shared<TestCondition>();

  auto cond = std::make_shared<ConditionNot>(subCond);

  EXPECT_EQ(subCond->_resetCount, 0);
  EXPECT_EQ(subCond->_initCount, 0);
  EXPECT_EQ(subCond->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(subCond->_resetCount, 0);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 0);

  cond->Reset(bei);
  EXPECT_EQ(subCond->_resetCount, 1);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 0);

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_resetCount, 1);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 1);

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_resetCount, 1);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 2);

  subCond->_val = true;
  EXPECT_FALSE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_resetCount, 1);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 3);

  cond->Reset(bei);
  EXPECT_EQ(subCond->_resetCount, 2);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 3);

  EXPECT_FALSE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_resetCount, 2);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 4);

}

TEST(BeiConditions, NotTrue)
{
  const std::string json = R"json(
  {
    "conditionType": "Not",
    "subCondition": {
      "conditionType": "AlwaysRun"
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  BehaviorExternalInterface bei;
  
  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  cond->Reset(bei);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, NotTimer)
{
  const std::string json = R"json(
  {
    "conditionType": "Not",
    "subCondition": {
      "conditionType": "Timer",
      "timeout": 30.0
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  BehaviorExternalInterface bei;

  cond->Init(bei);
  cond->Reset(bei);

  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(2.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(29.9));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(30.0001));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(35.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(65.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(900.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  cond->Reset(bei);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(920.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(930.1));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(9001.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

}
