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

// Access protected factory functions for test purposes
#define protected public

#include "gtest/gtest.h"

#include "engine/behaviorSystem/behaviorChoosers/scoringBehaviorChooser.h"
#include "engine/behaviorSystem/behaviorContainer.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/robot.h"
#include "engine/cozmoContext.h"



using namespace Anki::Cozmo;


static const char* kTestBehavior1Json =
"{"
"   \"behaviorClass\" : \"Wait\","
"   \"name\" : \"Test1\""
"}";

static const char* kTestBehavior2Json =
"{"
"   \"behaviorClass\" : \"Wait\","
"   \"name\" : \"Test2\""
"}";

static const char* kTestBehavior3Json =
"{"
"   \"behaviorClass\" : \"Wait\","
"   \"name\" : \"Test3\""
"}";


bool LoadTestBehaviors(Robot& testRobot, ScoringBehaviorChooser& behaviorChooser)
{
  bool allAddedOk = true;
  BehaviorContainer& behaviorContainer = testRobot.GetBehaviorManager().GetBehaviorContainer();

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
      
      IBehaviorPtr newBehavior = behaviorContainer.CreateBehavior(testBehaviorJson, testRobot);
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
