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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
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
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/components/batteryComponent.h"
#include "engine/robot.h"
#include "engine/utils/cozmoFeatureGate.h"
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

TEST(BeiConditions, Emotion)
{
  const std::string jsonMax = R"json(
  {
    "conditionType": "Emotion",
    "emotion": "Confident",
    "max": -0.5
  })json";
  const std::string jsonMin = R"json(
  {
    "conditionType": "Emotion",
    "emotion": "Confident",
    "min": -0.5
  })json";
  const std::string jsonRange = R"json(
  {
    "conditionType": "Emotion",
    "emotion": "Confident",
    "min": -0.75,
    "max": -0.5
  })json";
  const std::string jsonValue = R"json(
  {
    "conditionType": "Emotion",
    "emotion": "Confident",
    "value": -0.33
  })json";
  const std::string jsonStimulation = R"json(
  {
    "conditionType": "Emotion",
    "emotion": "Stimulated",
    "value": 0.5
  })json";

  IBEIConditionPtr condMin;
  IBEIConditionPtr condMax;
  IBEIConditionPtr condRange;
  IBEIConditionPtr condValue;
  IBEIConditionPtr condStim;
  CreateBEI(jsonMax, condMax);
  CreateBEI(jsonMin, condMin);
  CreateBEI(jsonRange, condRange);
  CreateBEI(jsonValue, condValue);
  CreateBEI(jsonStimulation, condStim);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Robot& robot = testBehaviorFramework.GetRobot();
  BEIRobotInfo info(robot);
  MoodManager moodManager;
  InitBEIPartial( { {BEIComponentID::MoodManager, &moodManager}, {BEIComponentID::RobotInfo, &info} }, bei );
  
  // max
  condMax->Init(bei);
  condMax->SetActive(bei, true);
  EXPECT_FALSE( condMax->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, -1.0f);
  EXPECT_TRUE( condMax->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, 1.0f);
  EXPECT_FALSE( condMax->AreConditionsMet(bei) );
  
  // min
  condMin->Init(bei);
  condMin->SetActive(bei, true);
  moodManager.SetEmotion(EmotionType::Confident, -1.0f);
  EXPECT_FALSE( condMin->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, 1.0f);
  EXPECT_TRUE( condMin->AreConditionsMet(bei) );
  
  // range
  condRange->Init(bei);
  condRange->SetActive(bei, true);
  moodManager.SetEmotion(EmotionType::Confident, -0.8f);
  EXPECT_FALSE( condRange->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, -0.4f);
  EXPECT_FALSE( condRange->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, -0.6f);
  EXPECT_TRUE( condRange->AreConditionsMet(bei) );
  
  // value
  condValue->Init(bei);
  condValue->SetActive(bei, true);
  moodManager.SetEmotion(EmotionType::Confident, -1.0f);
  EXPECT_FALSE( condValue->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Confident, -0.330001f);
  EXPECT_TRUE( condValue->AreConditionsMet(bei) );
  
  // stim
  condStim->Init(bei);
  condStim->SetActive(bei, true);
  moodManager.SetEmotion(EmotionType::Stimulated, -1.0f);
  EXPECT_FALSE( condStim->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Stimulated, 0.500001f);
  EXPECT_TRUE( condStim->AreConditionsMet(bei) );
}

TEST(BeiConditions, SimpleMood)
{
  BaseStationTimer::getInstance()->UpdateTime( 0 );
  
  const std::string jsonMood = R"json(
  {
    "conditionType": "SimpleMood",
    "mood": "MedStim"
  })json";
  const std::string jsonTrans = R"json(
  {
    "conditionType": "SimpleMood",
    "from": "MedStim",
    "to": "HighStim"
  })json";
  const std::string jsonTransFromAny = R"json(
  {
    "conditionType": "SimpleMood",
    "to": "HighStim"
  })json";
  const std::string jsonTransToAny = R"json(
  {
    "conditionType": "SimpleMood",
    "from": "Frustrated"
  })json";

  IBEIConditionPtr condMood;
  IBEIConditionPtr condTrans;
  IBEIConditionPtr condTransFromAny;
  IBEIConditionPtr condTransToAny;
  CreateBEI(jsonMood, condMood);
  CreateBEI(jsonTrans, condTrans);
  CreateBEI(jsonTransFromAny, condTransFromAny);
  CreateBEI(jsonTransToAny, condTransToAny);

  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Robot& robot = testBehaviorFramework.GetRobot();
  BEIRobotInfo info(robot);
  MoodManager moodManager;
  InitBEIPartial( { {BEIComponentID::MoodManager, &moodManager}, {BEIComponentID::RobotInfo, &info} }, bei );
  
  // whenever SimpleMood definitions change significantly, this test might fail,
  // so change stimulation with these to be in the mid-range of each definition to reduce
  // the chance they need changing again
  const float lowStim = 0.01f;
  const float medStim = 0.5f;
  const float highStim = 1.0f;
  const float notFrustrated = 1.0f;
  const float frustrated = -0.5f;
  
  double gCurrentTime = 0.0; // Fake time for update calls
  auto tickMoodManager = [&](uint32_t numTicks) {
    const double kTickTimestep = 0.01;
    DependencyManagedEntity<RobotComponentID> dependencies;
    for (uint32_t i=0; i < numTicks; ++i)
    {
      gCurrentTime += kTickTimestep;
      BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( gCurrentTime ) );
      moodManager.UpdateDependent(dependencies);
    }
  };
  
  moodManager.SetEmotion(EmotionType::Stimulated, lowStim);
  moodManager.SetEmotion(EmotionType::Confident, notFrustrated);
  
  // check mood
  condMood->Init(bei);
  condMood->SetActive(bei, true);
  EXPECT_FALSE( condMood->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Stimulated, medStim);
  EXPECT_TRUE( condMood->AreConditionsMet(bei) );
  
  // check transition
  condTrans->Init(bei);
  condTrans->SetActive(bei, true);
  EXPECT_FALSE( condTrans->AreConditionsMet(bei) ); // no transition
  tickMoodManager( 1 );
  EXPECT_FALSE( condTrans->AreConditionsMet(bei) );
  moodManager.SetEmotion(EmotionType::Stimulated, highStim);
  EXPECT_TRUE( condTrans->AreConditionsMet(bei) ); // med->high, correct transition
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Stimulated, medStim);
  EXPECT_FALSE( condTrans->AreConditionsMet(bei) ); // high->med, wrong transition
  
  // check transition from any
  condTransFromAny->Init(bei);
  condTransFromAny->SetActive(bei, true);
  tickMoodManager( 1 );
  EXPECT_FALSE( condTransFromAny->AreConditionsMet(bei) ); // no transition
  moodManager.SetEmotion(EmotionType::Stimulated, lowStim);
  EXPECT_FALSE( condTransFromAny->AreConditionsMet(bei) ); // med->low, wrong transition
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Stimulated, highStim);
  EXPECT_TRUE( condTransFromAny->AreConditionsMet(bei) ); // low->high, correct transition
  moodManager.SetEmotion(EmotionType::Stimulated, medStim);
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Stimulated, highStim);
  EXPECT_TRUE( condTransFromAny->AreConditionsMet(bei) ); // med->high also a correct transition
  
  // check transition to any
  condTransToAny->Init(bei);
  condTransToAny->SetActive(bei, true);
  moodManager.SetEmotion(EmotionType::Confident, frustrated);
  EXPECT_FALSE( condTransToAny->AreConditionsMet(bei) ); // no transition
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Confident, notFrustrated);
  EXPECT_TRUE( condTransToAny->AreConditionsMet(bei) ); // frustrated->highstim, correct transition
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Confident, frustrated); // highstim->frustrated, wrong transition
  tickMoodManager( 1 );
  moodManager.SetEmotion(EmotionType::Stimulated, lowStim);
  EXPECT_TRUE( condTransToAny->AreConditionsMet(bei) ); // frustrated->lowstim, correct transition
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

  robot.GetBatteryComponent().SetOnChargeContacts(true);
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  robot.GetBatteryComponent().SetOnChargeContacts(false);
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
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

TEST(BeiConditions, FeatureGate)
{
  const std::string json = R"json(
  {
    "conditionType": "FeatureGate",
    "feature": "Exploring"
  })json";

  IBEIConditionPtr cond;
  CreateBEI(json, cond);

  TestBehaviorFramework testBehaviorFramework;
  testBehaviorFramework.InitializeStandardBehaviorComponent();
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  
  const auto feature = FeatureType::Exploring;
  bool oldVal = bei.GetRobotInfo().GetContext()->GetFeatureGate()->IsFeatureEnabled( feature );
  
  cond->Init(bei);
  cond->SetActive(bei, true);
  
  bei.GetRobotInfo().GetContext()->GetFeatureGate()->SetFeatureEnabled( feature, true );
  EXPECT_TRUE( cond->AreConditionsMet(bei) );
  
  bei.GetRobotInfo().GetContext()->GetFeatureGate()->SetFeatureEnabled( feature, false );
  EXPECT_FALSE( cond->AreConditionsMet(bei) );
  
  bei.GetRobotInfo().GetContext()->GetFeatureGate()->SetFeatureEnabled( feature, oldVal );
}




