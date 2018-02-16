/**
 * File: testUserIntentMap.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-02-01
 *
 * Description: Test for the user intent map
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"

#include "clad/types/behaviorComponent/userIntent.h"

#include "coretech/common/engine/utils/timer.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"

#include <json/json.h>

#include <string>


using namespace Anki;
using namespace Anki::Cozmo;

namespace {

const std::string& testMapConfig = R"json(
{
  "user_intent_map": [
    {
      "cloud_intent": "cloud_intent_1",
      "user_intent": "test_user_intent_1"
    },
    {
      "cloud_intent": "cloud_intent_2",
      "user_intent": "test_user_intent_2"
    },
    {
      "cloud_intent": "cloud_time_intent",
      "user_intent": "test_timeInSeconds"
    },
    {
      "cloud_intent": "cloud_name_intent",
      "user_intent": "test_name"
    },
    {
      "cloud_intent": "cloud_time_intent_substitution",
      "user_intent": "test_timeWithUnits",
      "substitutions": {
        "timer-duration.time": "time",
        "timer-duration.units": "units"
      },
      "numerics": ["timer-duration.time"]
    }
  ],

  "unmatched_intent": "unmatched_intent",
  
  "is_test": true // ignore a data validation step that ensures the above contains ALL clad enum values
})json";

}

void CreateComponent(const std::string& json, std::unique_ptr<UserIntentComponent>& comp, const Robot& robot)
{
  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  comp.reset(new UserIntentComponent(robot, config));
}
  
void Reset(UserIntent& intent)
{
  intent = UserIntent::Createunmatched_intent({});
}
  
TEST(UserIntentMap, CreateComponent)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  ASSERT_TRUE(comp != nullptr);
}

TEST(UserIntentMap, UserIntentsValid)
{
  EXPECT_TRUE(IsValidUserIntentTag("test_user_intent_1"));
  EXPECT_TRUE(IsValidUserIntentTag("test_user_intent_2"));
  EXPECT_FALSE(IsValidUserIntentTag("test_user_intent_3"));
  EXPECT_FALSE(IsValidUserIntentTag("cloud_intent_1"));
}

TEST(UserIntentMap, TriggerWord)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  EXPECT_FALSE(comp->IsTriggerWordPending());

  comp->SetTriggerWordPending();
  EXPECT_TRUE(comp->IsTriggerWordPending());

  comp->ClearPendingTriggerWord();
  EXPECT_FALSE(comp->IsTriggerWordPending());

  comp->SetTriggerWordPending();
  EXPECT_TRUE(comp->IsTriggerWordPending());

  comp->SetTriggerWordPending();
  EXPECT_TRUE(comp->IsTriggerWordPending());

  comp->ClearPendingTriggerWord();
  EXPECT_FALSE(comp->IsTriggerWordPending());  
}

TEST(UserIntentMap, UserIntent)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetUserIntentPending(USER_INTENT(test_user_intent_1));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(test_user_intent_1));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetUserIntentPending(USER_INTENT(test_user_intent_2));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetUserIntentPending(USER_INTENT(test_user_intent_1));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetUserIntentPending(USER_INTENT(unmatched_intent));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(test_user_intent_1));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(unmatched_intent));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));
}

TEST(UserIntentMap, CloudIntent)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetCloudIntentPending("cloud_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(test_user_intent_1));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetCloudIntentPending("cloud_intent_2");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetCloudIntentPending("cloud_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->SetCloudIntentPending("asdf");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(test_user_intent_1));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  comp->ClearUserIntent(USER_INTENT(unmatched_intent));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));
}

TEST(UserIntentMap, IntentExpiration)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  BaseStationTimer::getInstance()->UpdateTime(0);
  comp->Update();
  
  comp->SetUserIntentPending(USER_INTENT(test_user_intent_1));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));

  for( float t=0.1f; t<1.0f; t+=0.1f ) {
    BaseStationTimer::getInstance()->UpdateTime(t);
    comp->Update();
  }

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(unmatched_intent)));
}

TEST(UserIntentMap, JsonIntent)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  EXPECT_FALSE(comp->SetCloudIntentPendingFromJSON(""));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_FALSE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     'invalid", "format: no way }}}} this cant be valid
  )json"));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_FALSE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "wrong_key": "cloud_intent_1"
  })json"));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "intent": "cloud_intent_1"
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_1)));

  // this one is not expecting any data "data_tag"
  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "intent": "cloud_intent_2",
     "params": {
       "data_tag": 3.1415
     }
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_user_intent_2)));
}

TEST(UserIntentMap, ExtraData)
{
  TestBehaviorFramework testBehaviorFramework(1, nullptr);
  const Robot& robot = testBehaviorFramework.GetRobot();
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp, robot);

  UserIntent data;
  
  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "intent": "cloud_time_intent",
     "params": {
       "time_s": 42
     }
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  Reset(data);
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_timeInSeconds), data));
  EXPECT_EQ(data.GetTag(), UserIntentTag::test_timeInSeconds);
  EXPECT_EQ(data.Get_test_timeInSeconds().time_s, 42);

  comp->ClearUserIntent(USER_INTENT(test_timeInSeconds));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_timeInSeconds), data));

  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "intent": "cloud_time_intent",
     "params": {
       "time_s": 9001
     }
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  Reset(data);
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_timeInSeconds), data));
  EXPECT_EQ(data.GetTag(), UserIntentTag::test_timeInSeconds);
  EXPECT_EQ(data.Get_test_timeInSeconds().time_s, 9001);

  comp->ClearUserIntent(USER_INTENT(test_timeInSeconds));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_timeInSeconds)));
  
  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
     "intent": "cloud_name_intent",
     "params": {
       "name": "Victor"
     }
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  Reset(data);
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_timeInSeconds), data));
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_name), data));
  EXPECT_EQ(data.GetTag(), UserIntentTag::test_name);
  EXPECT_EQ(data.Get_test_name().name, "Victor");
              
  comp->ClearUserIntent(USER_INTENT(test_name));
  EXPECT_FALSE(comp->IsUserIntentPending(USER_INTENT(test_name)));
              
  // extra data with params that aren't camelCase or snake_case, and passing an int as a string
  EXPECT_TRUE(comp->SetCloudIntentPendingFromJSON(R"json(
  {
    "intent": "cloud_time_intent_substitution",
    "params": {
      "timer-duration.time": "60",
      "timer-duration.units": "s"
    }
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  Reset(data);
  EXPECT_TRUE(comp->IsUserIntentPending(USER_INTENT(test_timeWithUnits), data));
  EXPECT_EQ(data.GetTag(), UserIntentTag::test_timeWithUnits);
  EXPECT_EQ(data.Get_test_timeWithUnits().time, 60);
  EXPECT_EQ(data.Get_test_timeWithUnits().units, UserIntent_Test_Time_Units::s);

}
