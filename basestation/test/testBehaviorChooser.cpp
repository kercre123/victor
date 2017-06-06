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

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/scoringBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"


using namespace Anki::Cozmo;


static const char* kTestBehavior1Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test1\""
"}";

static const char* kTestBehavior2Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test2\""
"}";

static const char* kTestBehavior3Json =
"{"
"   \"behaviorClass\" : \"NoneBehavior\","
"   \"name\" : \"Test3\""
"}";


bool LoadTestBehaviors(Robot& testRobot, ScoringBehaviorChooser& behaviorChooser)
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
