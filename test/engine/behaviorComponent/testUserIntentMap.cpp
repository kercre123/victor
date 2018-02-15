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

#include "engine/aiComponent/userIntentComponent.h"

#include "clad/types/userIntents.h"

#include "coretech/common/engine/utils/timer.h"

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
      "user_intent": "user_intent_1"
    },
    {
      "cloud_intent": "cloud_intent_2",
      "user_intent": "user_intent_2"
    },
    {
      "cloud_intent": "cloud_time_intent",
      "user_intent": "user_time_intent",
      "extra_data": "timeInSeconds"
    },
    {
      "cloud_intent": "cloud_name_intent",
      "user_intent": "user_name_intent",
      "extra_data": "name"
    }
  ],

  "unmatched_intent": "default_intent"
})json";

}

void CreateComponent(const std::string& json, std::unique_ptr<UserIntentComponent>& comp)
{
  Json::Reader reader;
  Json::Value config;
  const bool parsedOK = reader.parse(json, config, false);
  ASSERT_TRUE(parsedOK);

  comp.reset(new UserIntentComponent(config));  
}

TEST(UserIntentMap, CreateComponent)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  ASSERT_TRUE(comp != nullptr);
}

TEST(UserIntentMap, UserIntentsValid)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  EXPECT_TRUE(comp->IsValidUserIntent("user_intent_1"));
  EXPECT_TRUE(comp->IsValidUserIntent("user_intent_2"));
  EXPECT_FALSE(comp->IsValidUserIntent("user_intent_3"));
  EXPECT_FALSE(comp->IsValidUserIntent("cloud_intent_1"));
}

TEST(UserIntentMap, TriggerWord)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

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
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetUserIntentPending("user_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("user_intent_1");
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetUserIntentPending("user_intent_2");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetUserIntentPending("user_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetUserIntentPending("default_intent");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_TRUE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("user_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_TRUE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("default_intent");
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));
}

TEST(UserIntentMap, CloudIntent)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetCloudIntentPending("cloud_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("user_intent_1");
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetCloudIntentPending("cloud_intent_2");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetCloudIntentPending("cloud_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  comp->SetCloudIntentPending("asdf");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_TRUE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("user_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_TRUE(comp->IsUserIntentPending("default_intent"));

  comp->ClearUserIntent("default_intent");
  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));
}

TEST(UserIntentMap, IntentExpiration)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  BaseStationTimer::getInstance()->UpdateTime(0);
  comp->Update();
  
  comp->SetUserIntentPending("user_intent_1");
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));

  for( float t=0.1f; t<1.0f; t+=0.1f ) {
    BaseStationTimer::getInstance()->UpdateTime(t);
    comp->Update();
  }

  EXPECT_FALSE(comp->IsAnyUserIntentPending());
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_1"));
  EXPECT_FALSE(comp->IsUserIntentPending("user_intent_2"));
  EXPECT_FALSE(comp->IsUserIntentPending("default_intent"));
}

TEST(UserIntentMap, JsonIntent)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  EXPECT_FALSE(comp->SetCloudIntentFromJSON(""));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_FALSE(comp->SetCloudIntentFromJSON(R"json(
  {
     'invalid", "format: no way }}}} this cant be valid
  )json"));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_FALSE(comp->SetCloudIntentFromJSON(R"json(
  {
     "wrong_key": "cloud_intent_1"
  })json"));
  EXPECT_FALSE(comp->IsAnyUserIntentPending());

  EXPECT_TRUE(comp->SetCloudIntentFromJSON(R"json(
  {
     "intent": "cloud_intent_1"
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_1"));

  EXPECT_TRUE(comp->SetCloudIntentFromJSON(R"json(
  {
     "intent": "cloud_intent_2",
     "extra_data": 3.1415
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  EXPECT_TRUE(comp->IsUserIntentPending("user_intent_2"));
}

TEST(UserIntentMap, ExtraData)
{
  std::unique_ptr<UserIntentComponent> comp;
  CreateComponent(testMapConfig, comp);

  UserIntentData data;
  
  EXPECT_TRUE(comp->SetCloudIntentFromJSON(R"json(
  {
     "intent": "cloud_time_intent",
     "time_s": 42  
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  data.Set_none({});
  EXPECT_TRUE(comp->IsUserIntentPending("user_time_intent", data));
  EXPECT_EQ(data.GetTag(), UserIntentDataTag::timeInSeconds);
  EXPECT_EQ(data.Get_timeInSeconds().time_s, 42);

  comp->ClearUserIntent("user_time_intent");
  EXPECT_FALSE(comp->IsUserIntentPending("user_time_intent", data));

  EXPECT_TRUE(comp->SetCloudIntentFromJSON(R"json(
  {
     "intent": "cloud_time_intent",
     "time_s": 9001
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  data.Set_none({});
  EXPECT_TRUE(comp->IsUserIntentPending("user_time_intent", data));
  EXPECT_EQ(data.GetTag(), UserIntentDataTag::timeInSeconds);
  EXPECT_EQ(data.Get_timeInSeconds().time_s, 9001);

  comp->ClearUserIntent("user_time_intent");
  EXPECT_FALSE(comp->IsUserIntentPending("user_time_intent"));
  
  EXPECT_TRUE(comp->SetCloudIntentFromJSON(R"json(
  {
     "intent": "cloud_name_intent",
     "name": "Victor"
  })json"));
  EXPECT_TRUE(comp->IsAnyUserIntentPending());
  data.Set_none({});
  EXPECT_FALSE(comp->IsUserIntentPending("user_time_intent", data));
  EXPECT_TRUE(comp->IsUserIntentPending("user_name_intent", data));
  EXPECT_EQ(data.GetTag(), UserIntentDataTag::name);
  EXPECT_EQ(data.Get_name().name, "Victor");

}
