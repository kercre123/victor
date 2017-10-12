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

// Access protected factory functions for test purposes
#define protected public

#include "gtest/gtest.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/cozmoContext.h"


using namespace Anki::Cozmo;


static const char* kTestBehaviorJson =
"{"
"   \"behaviorClass\" : \"PlayAnim\","
"   \"behaviorID\" : \"Wait\","
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
"  }"
"}";

static const BehaviorID expectedID = BehaviorID::Wait;

// verifies that behavior matches expected data based on the Json above and factory contains it correctly
void VerifyBehavior(const ICozmoBehaviorPtr inBehavior, const BehaviorContainer& behaviorContainer, size_t expectedBehaviorCount)
{
  EXPECT_EQ(inBehavior->GetID(), expectedID);
  
  ASSERT_EQ(inBehavior->GetEmotionScorerCount(), 2);
  EXPECT_EQ(inBehavior->GetEmotionScorer(0).GetEmotionType(),  EmotionType::Calm);
  EXPECT_EQ(inBehavior->GetEmotionScorer(0).TrackDeltaScore(), true);
  EXPECT_EQ(inBehavior->GetEmotionScorer(1).GetEmotionType(),  EmotionType::Excited);
  EXPECT_EQ(inBehavior->GetEmotionScorer(1).TrackDeltaScore(), false);
  
  EXPECT_EQ(inBehavior->GetRepetitionPenalty().GetNumNodes(), 2);
  EXPECT_FLOAT_EQ(inBehavior->GetRepetitionPenalty().EvaluateY(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRepetitionPenalty().EvaluateY(4.5f), 0.5f);

  EXPECT_EQ(inBehavior->GetRunningPenalty().GetNumNodes(), 3);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(0.0f), 1.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(5.0f), 1.0f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(35.0f), 0.75f);
  EXPECT_FLOAT_EQ(inBehavior->GetRunningPenalty().EvaluateY(200.0f), 0.5f);
  
  EXPECT_EQ(behaviorContainer.FindBehaviorByID(expectedID), inBehavior);
  EXPECT_EQ(behaviorContainer.GetBehaviorMap().size(), expectedBehaviorCount);
}


TEST(BehaviorFactory, CreateAndDestroyBehaviors)
{
  CozmoContext context{};
  Robot testRobot(0, &context);
  BehaviorContainer& behaviorContainer = testRobot.GetAIComponent().GetBehaviorContainer();
  
  const size_t kBaseBehaviorCount = behaviorContainer.GetBehaviorMap().size(); // some behaviors are added by default so likely >0
  
  Json::Value  testBehaviorJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestBehaviorJson, testBehaviorJson, false);
  ASSERT_TRUE(parsedOK);
  
  EXPECT_EQ(behaviorContainer.FindBehaviorByID(expectedID), nullptr); // this behavior shouldn't exist by default
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(testRobot,
                                  testRobot.GetAIComponent(),
                                  behaviorContainer,
                                  testRobot.GetBlockWorld(),
                                  testRobot.GetFaceWorld(),
                                  stateChangeComp);
  
  ICozmoBehaviorPtr newBehavior = behaviorContainer.CreateBehavior(testBehaviorJson);
  newBehavior->Init(*behaviorExternalInterface);
  ASSERT_NE(newBehavior, nullptr);
  
  VerifyBehavior(newBehavior, behaviorContainer, kBaseBehaviorCount + 1);
}
