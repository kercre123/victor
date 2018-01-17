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

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/moodSystem/moodManager.h"

using namespace Anki;
using namespace Anki::Cozmo;


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


  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  auto cond = BEIConditionFactory::CreateBEICondition(config);

  ASSERT_TRUE( cond != nullptr );

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

  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  auto cond = BEIConditionFactory::CreateBEICondition(config);

  ASSERT_TRUE( cond != nullptr );

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

