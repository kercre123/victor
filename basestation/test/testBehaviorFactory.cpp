/**
 * File: testBehaviorFactory
 *
 * Author: Mark Wesley
 * Created: 12/03/15
 *
 * Description: Unit tests for Behavior Factory
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=BehaviorFactory*
 **/


#include "gtest/gtest.h"

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/cozmoContext.h"


using namespace Anki::Cozmo;


static const char* kTestBehaviorJson =
"{"
"   \"behaviorType\" : \"PlayAnim\","
"   \"name\" : \"UnitTestPlayAnim\","
"   \"animTriggers\" : [ \"UnitTestAnim\" ],"
"   \"minTimeBetweenRuns\" : 5.0,"
"   \"emotionScorers\" : ["
"     {"
"       \"emotionType\" : \"Calm\","
"       \"scoreGraph\" : {"
"          \"nodes\" : ["
"             {"
"                \"x\" : -1,"
"                \"y\" : -0.5"
"             },"
"             {"
"                \"x\" : 2.0,"
"                \"y\" : 1.5"
"             }"
"          ]"
"       },"
"       \"trackDelta\" : true"
"     },"
"     {"
"       \"emotionType\" : \"Excited\","
"       \"scoreGraph\" : {"
"          \"nodes\" : ["
"             {"
"                \"x\" : -1,"
"                \"y\" : -0.5"
"             },"
"             {"
"                \"x\" : 1.0,"
"                \"y\" : 0.5"
"             }"
"          ]"
"       },"
"       \"trackDelta\" : false"
"     }"
"   ],"
"   \"repetitionPenalty\" : {"
"      \"nodes\" : ["
"         {"
"            \"x\" : 3.0,"
"            \"y\" : 0.0"
"         },"
"         {"
"            \"x\" : 6,"
"            \"y\" : 1.0"
"         }"
"      ]"
"   },"
"    \"runningPenalty\": {"
"    \"nodes\": ["
"      {"
"        \"x\": 0.0,"
"        \"y\": 1.0"
"      },"
"      {"
"        \"x\": 10.0,"
"        \"y\": 1.0"
"      },"
"      {"
"        \"x\": 60.0,"
"        \"y\": 0.5"
"      }"
"    ]"
"  },"
"   \"behaviorGroups\" : ["
"      \"MiniGame\","
"      \"RequestSpeedTap\""
"   ]"
"}";


bool ShouldBehaviorGroupBeSetForTest(BehaviorGroup behaviorGroup)
{
  return (behaviorGroup == BehaviorGroup::MiniGame) || (behaviorGroup == BehaviorGroup::RequestSpeedTap);
}


static const char* kTestBehaviorName = "UnitTestPlayAnim";


// verifies that behavior matches expected data based on the Json above and factory contains it correctly
void VerifyBehavior(const IBehavior* inBehavior, BehaviorFactory& behaviorFactory, size_t expectedBehaviorCount)
{
  EXPECT_EQ(inBehavior->GetName(), kTestBehaviorName);
  
  ASSERT_EQ(inBehavior->GetEmotionScorerCount(), 2);
  EXPECT_EQ(inBehavior->GetEmotionScorer(0).GetEmotionType(),  EmotionType::Calm);
  EXPECT_EQ(inBehavior->GetEmotionScorer(0).TrackDeltaScore(), true);
  EXPECT_EQ(inBehavior->GetEmotionScorer(1).GetEmotionType(),  EmotionType::Excited);
  EXPECT_EQ(inBehavior->GetEmotionScorer(1).TrackDeltaScore(), false);
  
  // Verify Behavior Groups
  {
    for (BehaviorGroup i = BehaviorGroup(0); i < BehaviorGroup::Count; ++i)
    {
      EXPECT_EQ(inBehavior->IsBehaviorGroup(i), ShouldBehaviorGroupBeSetForTest(i));
    }

    BehaviorGroupFlags groupFlags;
    EXPECT_FALSE(inBehavior->MatchesAnyBehaviorGroups(groupFlags));
    
    // Set every flag _but_ the set ones
    for (BehaviorGroup i = BehaviorGroup(0); i < BehaviorGroup::Count; ++i)
    {
      groupFlags.SetBitFlag(i, !ShouldBehaviorGroupBeSetForTest(i));
    }
    EXPECT_FALSE(inBehavior->MatchesAnyBehaviorGroups(groupFlags));
    
    // Set every expected flag, clear the rest
    for (BehaviorGroup i = BehaviorGroup(0); i < BehaviorGroup::Count; ++i)
    {
      groupFlags.SetBitFlag(i, ShouldBehaviorGroupBeSetForTest(i));
    }
    EXPECT_TRUE(inBehavior->MatchesAnyBehaviorGroups(groupFlags));

    // Set every flag
    for (BehaviorGroup i = BehaviorGroup(0); i < BehaviorGroup::Count; ++i)
    {
      groupFlags.SetBitFlag(i, true);
    }
    EXPECT_TRUE(inBehavior->MatchesAnyBehaviorGroups(groupFlags));
  }
  
  EXPECT_EQ(inBehavior->GetRepetionalPenalty().GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(inBehavior->GetRepetionalPenalty().EvaluateY(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRepetionalPenalty().EvaluateY(4.5f), 0.5f);

  EXPECT_EQ(inBehavior->GetRunningPenalty().GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(0.0f), 1.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(5.0f), 1.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(35.0f), 0.75f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(200.0f), 0.5f);
  
  EXPECT_EQ(behaviorFactory.FindBehaviorByName(kTestBehaviorName), inBehavior);
  EXPECT_EQ(behaviorFactory.GetBehaviorMap().size(), expectedBehaviorCount);
}


TEST(BehaviorFactory, CreateAndDestroyBehaviors)
{
  CozmoContext context{};
  Robot testRobot(0, &context);
  BehaviorFactory& behaviorFactory = testRobot.GetBehaviorFactory();
  
  const size_t kBaseBehaviorCount = behaviorFactory.GetBehaviorMap().size(); // some behaviors are added by default so likely >0
  
  Json::Value  testBehaviorJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestBehaviorJson, testBehaviorJson, false);
  ASSERT_TRUE(parsedOK);
  
  EXPECT_EQ(behaviorFactory.FindBehaviorByName(kTestBehaviorName), nullptr); // this behavior shouldn't exist by default

  IBehavior* newBehavior = behaviorFactory.CreateBehavior(testBehaviorJson, testRobot);
  ASSERT_NE(newBehavior, nullptr);
  
  VerifyBehavior(newBehavior, behaviorFactory, kBaseBehaviorCount + 1);
  
  // Creating duplicates - behavior depends on rule specified
  
  {
    IBehavior* newBehavior2 = behaviorFactory.CreateBehavior(testBehaviorJson, testRobot, BehaviorFactory::NameCollisionRule::ReuseOld);
    EXPECT_EQ(newBehavior2, newBehavior);
    VerifyBehavior(newBehavior, behaviorFactory, kBaseBehaviorCount + 1);
  }
  {
    IBehavior* newBehavior2 = behaviorFactory.CreateBehavior(testBehaviorJson, testRobot, BehaviorFactory::NameCollisionRule::OverwriteWithNew);
    ASSERT_NE(newBehavior2, nullptr);
    EXPECT_NE(newBehavior2, newBehavior);
    newBehavior = newBehavior2; // newBehavior will have been destroyed, use the new replacement now
    VerifyBehavior(newBehavior2, behaviorFactory, kBaseBehaviorCount + 1);
  }
  {
    IBehavior* newBehavior2 = behaviorFactory.CreateBehavior(testBehaviorJson, testRobot, BehaviorFactory::NameCollisionRule::Fail);
    EXPECT_EQ(newBehavior2, nullptr);
    VerifyBehavior(newBehavior, behaviorFactory, kBaseBehaviorCount + 1);
  }
  
  // Destroying a behavior will automatically remove it from the factory
  
  behaviorFactory.SafeDestroyBehavior(newBehavior);
  
  EXPECT_EQ(behaviorFactory.FindBehaviorByName(kTestBehaviorName), nullptr);
  EXPECT_EQ(behaviorFactory.GetBehaviorMap().size(), kBaseBehaviorCount);
  EXPECT_EQ(newBehavior, nullptr);
}
