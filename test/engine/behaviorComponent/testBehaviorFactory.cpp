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

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki::Cozmo;


static const char* kTestBehaviorJson =
"{"
"   \"behaviorClass\" : \"AnimSequence\","
"   \"behaviorID\" : \"Wait_TestInjectable\","
"   \"animTriggers\" : [ \"UnitTestAnim\" ]"
"}";

static const BehaviorID expectedID = BEHAVIOR_ID(Wait_TestInjectable);

// verifies that behavior matches expected data based on the Json above and factory contains it correctly
void VerifyBehavior(const ICozmoBehaviorPtr inBehavior, const BehaviorContainer& behaviorContainer, size_t expectedBehaviorCount)
{
  EXPECT_EQ(inBehavior->GetID(), expectedID);
  
  EXPECT_EQ(behaviorContainer.FindBehaviorByID(expectedID), inBehavior);
  EXPECT_EQ(behaviorContainer.GetBehaviorMap().size(), expectedBehaviorCount);
}


TEST(BehaviorFactory, CreateAndDestroyBehaviors)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, container);
  }
  
  BehaviorContainer& behaviorContainer = testBehaviorFramework.GetBehaviorContainer();
  
  const size_t kBaseBehaviorCount = behaviorContainer.GetBehaviorMap().size(); // some behaviors are added by default so likely >0
  
  Json::Value  testBehaviorJson;
  Json::Reader reader;
  const bool parsedOK = reader.parse(kTestBehaviorJson, testBehaviorJson, false);
  ASSERT_TRUE(parsedOK);
  
  EXPECT_EQ(behaviorContainer.FindBehaviorByID(expectedID), nullptr); // this behavior shouldn't exist by default
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  

  const bool createdOK = behaviorContainer.CreateAndStoreBehavior(testBehaviorJson);
  ASSERT_TRUE(createdOK);
  ICozmoBehaviorPtr newBehavior = behaviorContainer.FindBehaviorByID( BEHAVIOR_ID(Wait_TestInjectable) );
  ASSERT_TRUE(newBehavior != nullptr);
  newBehavior->Init(behaviorExternalInterface);
  ASSERT_NE(newBehavior, nullptr);
  
  VerifyBehavior(newBehavior, behaviorContainer, kBaseBehaviorCount + 1);
}
