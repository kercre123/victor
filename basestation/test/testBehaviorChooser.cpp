/**
 * File: testBehaviorChooser
 *
 * Author: Mark Wesley
 * Created: 02/17/16
 *
 * Description: Unit tests for Behavior Chooser
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=BehaviorChooser*
 **/


#include "gtest/gtest.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"


using namespace Anki::Cozmo;


static const char* kTestBehavior1Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test1\","
"   \"behaviorGroups\" : ["
"     \"MiniGame\","
"     \"RequestSpeedTap\""
"   ]"
"}";

static const char* kTestBehavior2Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test2\","
"   \"behaviorGroups\" : ["
"     \"RequestMemoryMatch\","
"     \"RequestSpeedTap\""
"   ]"
"}";

static const char* kTestBehavior3Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test3\","
"   \"behaviorGroups\" : ["
"     \"RequestCubePounce\","
"     \"RequestSpeedTap\""
"   ]"
"}";


// groups only leaves orientation (2) disabled,
static const char* kTestChooser1Json =
"{"
"   \"disabledGroups\" : ["
"     \"RequestCubePounce\","
"     \"RequestSpeedTap\""
"   ],"
"   \"enabledGroups\" : ["
"     \"RequestCubePounce\","
"     \"RequestMemoryMatch\""
"   ],"
"   \"disabledBehaviors\" : ["
"     \"Test3\""
"   ],"
"   \"enabledBehaviors\" : ["
"     \"Test1\""
"   ]"
"}";


// groups leaves all disabled
static const char* kTestChooser2Json =
"{"
"   \"disabledGroups\" : ["
"     \"RequestCubePounce\","
"     \"RequestSpeedTap\""
"   ],"
"   \"enabledGroups\" : ["
"   ],"
"   \"disabledBehaviors\" : ["
"     \"Test1\""
"   ],"
"   \"enabledBehaviors\" : ["
"     \"Test3\""
"   ]"
"}";


bool LoadTestBehaviors(Robot& testRobot, SimpleBehaviorChooser& behaviorChooser)
{
  bool allAddedOk = true;
  BehaviorFactory& behaviorFactory = testRobot.GetBehaviorFactory();

  Json::Reader reader;
  
  std::vector<const char*> behaviorJsons = { kTestBehavior1Json, kTestBehavior2Json, kTestBehavior3Json };
  
  for (const char* behaviorJsonString : behaviorJsons)
  {
    Json::Value  testBehaviorJson;
    const bool parsedOK = reader.parse(behaviorJsonString, testBehaviorJson, false);
    EXPECT_TRUE(parsedOK);

    if (parsedOK)
    {
      // Factory will automatically delete the behaviors later when robot is destroyed
      
      IBehavior* newBehavior = behaviorFactory.CreateBehavior(testBehaviorJson, testRobot);
      EXPECT_NE(newBehavior, nullptr);
      if (newBehavior)
      {
        const bool addedOk = (behaviorChooser.TryAddBehavior(newBehavior) == Anki::RESULT_OK);
        allAddedOk = allAddedOk && addedOk;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  return allAddedOk;
}


TEST(BehaviorChooser, EnabledBehaviors)
{
  CozmoContext context{};
  Robot testRobot(0, &context);
  Json::Value chooserConfig;
  
  SimpleBehaviorChooser behaviorChooser(testRobot, chooserConfig);
  
  const bool loadedOK = LoadTestBehaviors(testRobot, behaviorChooser);
  EXPECT_TRUE(loadedOK);
  
  if (loadedOK)
  {
    const char* testName1 = "Test1";
    const char* testName2 = "Test2";
    const char* testName3 = "Test3";
    
    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName1) );
    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName2) );
    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName3) );

    Json::Value  chooserConfig1;
    Json::Value  chooserConfig2;
    Json::Reader reader;
    
    bool parsedOK = reader.parse(kTestChooser1Json, chooserConfig1, false);
    EXPECT_TRUE(parsedOK);
    parsedOK = reader.parse(kTestChooser2Json, chooserConfig2, false);
    EXPECT_TRUE(parsedOK);
    
    behaviorChooser.ReadEnabledBehaviorsConfiguration(chooserConfig1);

    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName1) );
    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName2) );
    EXPECT_FALSE( behaviorChooser.IsBehaviorEnabled(testName3) );
   
    behaviorChooser.ReadEnabledBehaviorsConfiguration(chooserConfig2);
    
    EXPECT_FALSE( behaviorChooser.IsBehaviorEnabled(testName1) );
    EXPECT_FALSE( behaviorChooser.IsBehaviorEnabled(testName2) );
    EXPECT_TRUE( behaviorChooser.IsBehaviorEnabled(testName3) );
  }
  
  
 
}

