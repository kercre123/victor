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
 * --gtest_filter=BeiConditions.*
 **/

#include "gtest/gtest.h"

// access robot internals for test
#define private public
#define protected public

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCompound.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnitTest.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/components/batteryComponent.h"
#include "engine/robot.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/math/math.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
CONSOLE_VAR_EXTERN(float, kTimeMultiplier);
}
}

using namespace Anki;
using namespace Anki::Cozmo;



namespace {

void CreateBEI(const std::string& json, IBEIConditionPtr& cond)
{
  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  cond = BEIConditionFactory::CreateBEICondition(config, "testing");

  ASSERT_TRUE( cond != nullptr );
}

}

TEST(BeiConditions, TestUnitTestCondition)
{
  // a test of the test, if you will

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  auto cond = std::make_shared<ConditionUnitTest>(false);

  EXPECT_EQ(cond->_initCount, 0);
  EXPECT_EQ(cond->_setActiveCount, 0);
  EXPECT_EQ(cond->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_setActiveCount, 0);
  EXPECT_EQ(cond->_areMetCount, 0);

  cond->SetActive(bei, true);
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_setActiveCount, 1);
  EXPECT_EQ(cond->_areMetCount, 0);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_setActiveCount, 1);
  EXPECT_EQ(cond->_areMetCount, 1);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_setActiveCount, 1);
  EXPECT_EQ(cond->_areMetCount, 2);

  cond->_val = true;

  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_EQ(cond->_initCount, 1);
  EXPECT_EQ(cond->_setActiveCount, 1);
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

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  // don't call init, should be ok if no one uses it
  // bei.Init();
  
  cond->Init(bei);
  cond->SetActive(bei, true);

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
    "conditionType": "TrueCondition" 
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

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

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Robot& robot = testBehaviorFramework.GetRobot();
  BEIRobotInfo info(robot);
  MoodManager moodManager;
  InitBEIPartial( { {BEIComponentID::MoodManager, &moodManager}, {BEIComponentID::RobotInfo, &info} }, bei );
  
  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  moodManager.SetEmotion(EmotionType::Confident, -1.0f);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  moodManager.SetEmotion(EmotionType::Confident, 1.0f);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, Timer)
{
  const float oldVal = kTimeMultiplier;
  kTimeMultiplier = 1.0f;
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  const std::string json = R"json(
  {
    "conditionType": "TimerInRange",
    "begin_s": 30.0,
    "end_s": 35.0
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(2.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(29.9));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(30.01));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(34.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(35.01));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(900.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  const float resetTime_s = 950.0f;
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s));
  cond->SetActive(bei, true);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 1.0f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 29.0f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 30.01f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 34.7f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 40.0f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 80.0f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  kTimeMultiplier = oldVal;
}

TEST(BeiConditions, CompoundNot)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  auto subCond = std::make_shared<ConditionUnitTest>(true);

  auto cond = ConditionCompound::CreateNotCondition( subCond );

  EXPECT_EQ(subCond->_initCount, 0);
  EXPECT_EQ(subCond->_setActiveCount, 0);
  EXPECT_EQ(subCond->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 0);
  EXPECT_EQ(subCond->_areMetCount, 0);

  cond->SetActive(bei, true);
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 0);


  EXPECT_FALSE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 1);

  EXPECT_FALSE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 2);

  subCond->_val = false;
  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 3);

  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 3);

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond->_initCount, 1);
  EXPECT_EQ(subCond->_setActiveCount, 1);
  EXPECT_EQ(subCond->_areMetCount, 4);

}

TEST(BeiConditions, CompoundAnd)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  auto subCond1 = std::make_shared<ConditionUnitTest>(true);
  auto subCond2 = std::make_shared<ConditionUnitTest>(true);

  auto cond = ConditionCompound::CreateAndCondition( {subCond1, subCond2} );

  EXPECT_EQ(subCond1->_initCount, 0);
  EXPECT_EQ(subCond1->_setActiveCount, 0);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 0);
  EXPECT_EQ(subCond2->_setActiveCount, 0);
  EXPECT_EQ(subCond2->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 0);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 0);
  EXPECT_EQ(subCond2->_areMetCount, 0);

  cond->SetActive(bei, true);
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 0);
  
  subCond1->_val = true;
  subCond2->_val = true;

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 1);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 1);

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 2);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 2);

  subCond1->_val = true;
  subCond2->_val = false;
  EXPECT_FALSE(cond->AreConditionsMet(bei));
  
  subCond1->_val = false;
  subCond2->_val = true;
  EXPECT_FALSE(cond->AreConditionsMet(bei));
  
  subCond1->_val = false;
  subCond2->_val = false;
  EXPECT_FALSE(cond->AreConditionsMet(bei));
}

TEST(BeiConditions, CompoundOr)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  auto subCond1 = std::make_shared<ConditionUnitTest>(true);
  auto subCond2 = std::make_shared<ConditionUnitTest>(true);

  auto cond = ConditionCompound::CreateOrCondition( {subCond1, subCond2} );

  EXPECT_EQ(subCond1->_initCount, 0);
  EXPECT_EQ(subCond1->_setActiveCount, 0);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 0);
  EXPECT_EQ(subCond2->_setActiveCount, 0);
  EXPECT_EQ(subCond2->_areMetCount, 0);

  cond->Init(bei);
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 0);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 0);
  EXPECT_EQ(subCond2->_areMetCount, 0);

  cond->SetActive(bei, true);
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 0);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 0);
  
  subCond1->_val = true;
  subCond2->_val = true;

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 1);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 1);

  EXPECT_TRUE(cond->AreConditionsMet(bei));
  EXPECT_EQ(subCond1->_initCount, 1);
  EXPECT_EQ(subCond1->_setActiveCount, 1);
  EXPECT_EQ(subCond1->_areMetCount, 2);
  EXPECT_EQ(subCond2->_initCount, 1);
  EXPECT_EQ(subCond2->_setActiveCount, 1);
  EXPECT_EQ(subCond2->_areMetCount, 2);

  subCond1->_val = true;
  subCond2->_val = false;
  EXPECT_TRUE(cond->AreConditionsMet(bei));
  
  subCond1->_val = false;
  subCond2->_val = true;
  EXPECT_TRUE(cond->AreConditionsMet(bei));
  
  subCond1->_val = false;
  subCond2->_val = false;
  EXPECT_FALSE(cond->AreConditionsMet(bei));

}

TEST(BeiConditions, CompoundNotJson)
{
  const std::string json = R"json(
  {
    "conditionType": "Compound",
    "not": {
      "conditionType": "TrueCondition"
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  cond->SetActive(bei, true);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, CompoundAndJson)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  // true && true
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "and": [
        {
          "conditionType": "TrueCondition"
        },
        {
          "conditionType": "TrueCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_TRUE( cond->AreConditionsMet(bei) );
  }
  // true && false (use UnitTestCondition as false)
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "and": [
        {
          "conditionType": "TrueCondition"
        },
        {
          "conditionType": "UnitTestCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_FALSE( cond->AreConditionsMet(bei) );
  }
  // false && false (use UnitTestCondition as false)
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "and": [
        {
          "conditionType": "UnitTestCondition"
        },
        {
          "conditionType": "UnitTestCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_FALSE( cond->AreConditionsMet(bei) );
  }
  
}

TEST(BeiConditions, CompoundOrJson)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  // true || true
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "or": [
        {
          "conditionType": "TrueCondition"
        },
        {
          "conditionType": "TrueCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_TRUE( cond->AreConditionsMet(bei) );
  }
  // true || false (use UnitTestCondition as false)
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "or": [
        {
          "conditionType": "TrueCondition"
        },
        {
          "conditionType": "UnitTestCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_TRUE( cond->AreConditionsMet(bei) );
  }
  // false || false (use UnitTestCondition as false)
  {
    const std::string json = R"json(
    {
      "conditionType": "Compound",
      "or": [
        {
          "conditionType": "UnitTestCondition"
        },
        {
          "conditionType": "UnitTestCondition"
        }
      ]
    })json";

    IBEIConditionPtr cond;
    CreateBEI(json, cond);
    
    cond->Init(bei);
    cond->SetActive(bei, true);
    EXPECT_FALSE( cond->AreConditionsMet(bei) );
  }
}

TEST(BeiConditions, CompoundComplex)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  

  const std::string json = R"json(
  {
    "conditionType": "Compound",
    "and": [
      {
        "or": [
          {
            "conditionType": "TrueCondition"
          },
          {
            "conditionType": "UnitTestCondition"
          }
        ]
      },
      {
        "not": {
          "conditionType": "UnitTestCondition"
        }
      },
      {
        "conditionType": "UnitTestCondition",
        "value": true
      }
    ]
  }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);
  
  cond->Init(bei);
  cond->SetActive(bei, true);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, NegateTimerInRange)
{
  const float oldVal = kTimeMultiplier;
  kTimeMultiplier = 1.0f;
  
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  const std::string json = R"json(
  {
    "conditionType": "Compound",
    "not": {
      "conditionType": "TimerInRange",
      "begin_s": 30.0,
      "end_s": 35.0
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(2.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(29.9));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(30.01));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(34.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(35.01));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(900.0));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  const float resetTime_s = 950.0f;
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s));
  cond->SetActive(bei, true);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 1.0f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 29.0f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 30.01f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 34.7f));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 40.0f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime_s + 80.0f));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  kTimeMultiplier = oldVal;
}

TEST(BeiConditions, OnCharger)
{
  const std::string json = R"json(
  {
    "conditionType": "OnCharger"
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  TestBehaviorFramework tbf(1, nullptr);
  Robot& robot = tbf.GetRobot();
  
  BEIRobotInfo info(robot);
  InitBEIPartial( { {BEIComponentID::RobotInfo, &info} }, bei );
  
  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  // charger implies platform here
  robot.GetBatteryComponent().SetOnChargeContacts(true);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  // off charger, but still on platform
  robot.GetBatteryComponent().SetOnChargeContacts(false);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  robot.GetBatteryComponent().SetOnChargerPlatform(false);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  // just on platform
  robot.GetBatteryComponent().SetOnChargerPlatform(true);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, TimedDedup)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  const std::string json = R"json(
  {
    "conditionType": "TimedDedup",
    "dedupInterval_ms" : 4000.0,
    "subCondition": {
      "conditionType": "TrueCondition"
    }
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();

  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(2.0));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(3.9));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );

  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(4.1));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, TriggerWordPending)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  const std::string json = R"json(
  {
    "conditionType": "TriggerWordPending"
  })json";
  
  IBEIConditionPtr cond;
  CreateBEI(json, cond);
  
  TestBehaviorFramework tbf(1, nullptr);
  tbf.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = tbf.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);
  
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  uic.SetTriggerWordPending();
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  uic.SetTriggerWordPending();
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  uic.ClearPendingTriggerWord();
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, UserIntentPending)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  const std::string json = R"json(
  {
    "conditionType": "UserIntentPending",
    "list": [
      {
        "type": "test_user_intent_1"
      },
      {
        "type": "set_timer"
      },
      {
        "type": "test_name",
        "name": ""
      },
      {
        "type": "test_timeWithUnits",
        "time": 60,
        "units": "m"
      },
      {
        "type": "test_name",
        "_lambda": "test_lambda"
      }
    ]
  })json";
  // in the above, the condition should fire if
  // (1) test_user_intent_1  matches the tag
  // (2) set_timer           matches the tag
  // (3) test_name           matches the tag and name must strictly be empty
  // (4) test_timeWithUnits  matches the tag and and data
  // (5) test_name           matches the tag and lambda must eval (name must be Victor)
  
  IBEIConditionPtr ptr;
  std::shared_ptr<ConditionUserIntentPending> cond;
  CreateBEI( json, ptr );
  cond = std::dynamic_pointer_cast<ConditionUserIntentPending>(ptr);
  ASSERT_NE( cond, nullptr );
  
  TestBehaviorFramework tbf(1, nullptr);
  tbf.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = tbf.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);
  
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  
  // (1) test_user_intent_1  matches the tag
  
  uic.SetUserIntentPending( USER_INTENT(test_user_intent_1) ); // right intent
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(test_user_intent_1) );
  
  
  uic.ClearUserIntent( USER_INTENT(test_user_intent_1) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) ); // no intent
  
  UserIntent_Test_TimeWithUnits timeWithUnits;
  uic.SetUserIntentPending( UserIntent::Createtest_timeWithUnits(std::move(timeWithUnits)) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) ); // wrong intent
  uic.ClearUserIntent( USER_INTENT(test_timeWithUnits) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) ); // no intent
  
  // (2) set_timer  matches the tag
  
  UserIntent_TimeInSeconds timeInSeconds1; // default
  UserIntent_TimeInSeconds timeInSeconds2{10}; // non default
  uic.SetUserIntentPending( UserIntent::Createset_timer(std::move(timeInSeconds1)) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) ); // correct intent
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(set_timer) );
  uic.ClearUserIntent( USER_INTENT(set_timer) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  uic.SetUserIntentPending( UserIntent::Createset_timer(std::move(timeInSeconds2)) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) ); // correct intent
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(set_timer) );
  uic.ClearUserIntent( USER_INTENT(set_timer) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  // (3) test_name           matches the tag and name must strictly be empty
  
  UserIntent_Test_Name name1; // default
  UserIntent_Test_Name name2{"whizmo"}; // non default
  uic.SetUserIntentPending( UserIntent::Createtest_name(std::move(name1)) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) ); // correct intent
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(test_name) );
  uic.ClearUserIntent( USER_INTENT(test_name) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  uic.SetUserIntentPending( UserIntent::Createtest_name(std::move(name2)) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) ); // wrong intent
  uic.ClearUserIntent( USER_INTENT(test_name) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  // (4) test_timeWithUnits  matches the tag and data (60mins)
  
  UserIntent_Test_TimeWithUnits timeWithUnits1{60, UserIntent_Test_Time_Units::m};
  UserIntent_Test_TimeWithUnits timeWithUnits2{20, UserIntent_Test_Time_Units::m};
  uic.SetUserIntentPending( UserIntent::Createtest_timeWithUnits(std::move(timeWithUnits1)) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) ); // correct intent
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(test_timeWithUnits) );
  uic.ClearUserIntent( USER_INTENT(test_timeWithUnits) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  uic.SetUserIntentPending( UserIntent::Createtest_timeWithUnits(std::move(timeWithUnits2)) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) ); // wrong data
  uic.ClearUserIntent( USER_INTENT(test_timeWithUnits) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  // (5) test_name           matches the tag and lambda must eval (name must be Victor)
  
  UserIntent name5;
  name5.Set_test_name( UserIntent_Test_Name{"Victor"} );
  uic.SetUserIntentPending( std::move(name5) ); // right intent with right data
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_EQ( cond->GetUserIntentTagSelected(), USER_INTENT(test_name) );
  uic.ClearUserIntent( USER_INTENT(test_name) );
}

CONSOLE_VAR( unsigned int, kTestBEIConsoleVar, "unit tests", 0);
TEST(BeiConditions, ConsoleVar)
{
  const std::string json = R"json(
  {
    "conditionType": "ConsoleVar",
    "variable": "TestBEIConsoleVar",
    "value": 5
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);

  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  kTestBEIConsoleVar = 1;
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  kTestBEIConsoleVar = 5;
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  kTestBEIConsoleVar = 1;
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
}

TEST(BeiConditions, BehaviorTimer)
{
  BaseStationTimer::getInstance()->UpdateTime(0.0);
  
  const float oldVal = kTimeMultiplier;
  kTimeMultiplier = 1.0f;
  
  const std::string json = R"json(
  {
    "conditionType": "BehaviorTimer",
    "timerName": "FistBump",
    "cooldown_s": 15
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  cond->Init(bei);
  cond->SetActive(bei, true);
  
  // never been reset before
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  double resetTime1 = 2.0;
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime1));
  
  // never been reset before
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  bei.GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::FistBump ).Reset();
  
  // has been reset, no time elapsed
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  // prior to expiration
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime1 + 14.9999));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  // just past expiration
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime1 + 15.0001));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );

  // works a second time
  
  double resetTime2 = resetTime1 + 16.0001; // still valid after first timer
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime2));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  bei.GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::FistBump ).Reset();
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime2 + 14.9999));
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  BaseStationTimer::getInstance()->UpdateTime(Util::SecToNanoSec(resetTime2 + 15.0001));
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  kTimeMultiplier = oldVal;
}


