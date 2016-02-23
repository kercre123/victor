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

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"


using namespace Anki::Cozmo;


static const char* kTestBehavior1Json =
"{"
"   \"behaviorType\" : \"NoneBehavior\","
"   \"name\" : \"Test1\","
"   \"behaviorGroups\" : ["
"     \"AffectsBlocks\","
"     \"ChangesPosition\""
"   ]"
"}";

static const char* kTestBehavior2Json =
"{"
"   \"behaviorType\" : \"NoneBehavior\","
"   \"name\" : \"Test2\","
"   \"behaviorGroups\" : ["
"     \"ChangesOrientation\","
"     \"ChangesPosition\""
"   ]"
"}";

static const char* kTestBehavior3Json =
"{"
"   \"behaviorType\" : \"NoneBehavior\","
"   \"name\" : \"Test3\","
"   \"behaviorGroups\" : ["
"     \"MovesLift\","
"     \"ChangesPosition\""
"   ]"
"}";


// groups only leaves orientation (2) disabled,
static const char* kTestChooser1Json =
"{"
"   \"disabledGroups\" : ["
"     \"MovesLift\","
"     \"ChangesPosition\""
"   ],"
"   \"enabledGroups\" : ["
"     \"MovesLift\","
"     \"ChangesOrientation\""
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
"     \"MovesLift\","
"     \"ChangesPosition\""
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
        behaviorChooser.AddBehavior(newBehavior);
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

  return true;
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
    BehaviorFactory& behaviorFactory = testRobot.GetBehaviorFactory();
    
    const IBehavior* test1 = behaviorFactory.FindBehaviorByName("Test1");
    const IBehavior* test2 = behaviorFactory.FindBehaviorByName("Test2");
    const IBehavior* test3 = behaviorFactory.FindBehaviorByName("Test3");

    ASSERT_NE(test1, nullptr);
    ASSERT_NE(test2, nullptr);
    ASSERT_NE(test3, nullptr);
    
    EXPECT_TRUE(test1->IsChoosable());
    EXPECT_TRUE(test2->IsChoosable());
    EXPECT_TRUE(test3->IsChoosable());

    Json::Value  chooserConfig1;
    Json::Value  chooserConfig2;
    Json::Reader reader;
    
    bool parsedOK = reader.parse(kTestChooser1Json, chooserConfig1, false);
    EXPECT_TRUE(parsedOK);
    parsedOK = reader.parse(kTestChooser2Json, chooserConfig2, false);
    EXPECT_TRUE(parsedOK);
    
    behaviorChooser.InitEnabledBehaviors(chooserConfig1);
   
    EXPECT_TRUE(test1->IsChoosable());
    EXPECT_TRUE(test2->IsChoosable());
    EXPECT_FALSE(test3->IsChoosable());
  
    behaviorChooser.EnableAllBehaviors();
    behaviorChooser.InitEnabledBehaviors(chooserConfig2);
    
    EXPECT_FALSE(test1->IsChoosable());
    EXPECT_FALSE(test2->IsChoosable());
    EXPECT_TRUE(test3->IsChoosable());
  }
  
  
 
}

