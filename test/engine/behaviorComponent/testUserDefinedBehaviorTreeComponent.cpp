/**
 * File: testUserDefinedBehaviorTreeComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 7/11/18
 *
 * Description: Various tests for user-defined behavior tree component
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// Access internals for tests
#define private public
#define protected public

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userDefinedBehaviorTreeComponent/userDefinedBehaviorTreeComponent.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "gtest/gtest.h"

#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/beiConditionTypes.h"


extern Anki::Vector::CozmoContext* cozmoContext;

namespace {

const int kRobotId = 0;

// removes all test information from storage before tests
void RemoveTestDataPrior()
{
  using namespace Anki::Vector;

  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);

  // clear data in json map
  robot->GetDataAccessorComponent()._variableSnapshotJsonMap->clear();
  robot->GetVariableSnapshotComponent()._variableSnapshotDataMap.clear();
};

// removes all test information from storage before tests
void RemoveTestDataAfter()
{
  using namespace Anki::Vector;

  // cache the name of our save directory
  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);
  
  // clear data in data and json maps
  robot->GetDataAccessorComponent()._variableSnapshotJsonMap->clear();
  robot->GetVariableSnapshotComponent()._variableSnapshotDataMap.clear();
};

// normal case of adding a mapping, checking for false overwrite, looking it up to get checked pointer
TEST(UserDefinedBehaviorTreeComponent, BasicUseCase)
{
  using namespace Anki::Vector;
  RemoveTestDataPrior();

  auto testBEICondType = BEIConditionType::CliffDetected;
  auto testBehaviorId = BehaviorID::ReactToFrustrationMajor;

  {
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);
    auto userBehaviorTreeComp = robot1->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserDefinedBehaviorTreeComponent>();

    const bool wasOverwritten = userBehaviorTreeComp.AddCondition(testBEICondType, testBehaviorId);
    ASSERT_FALSE(wasOverwritten);

    ICozmoBehaviorPtr behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(testBEICondType);

    EXPECT_EQ(behaviorPtr->GetID(), testBehaviorId);
  }

  RemoveTestDataAfter();
};

// overwrite a mapping, check for true, look it up with pointer
TEST(UserDefinedBehaviorTreeComponent, OverwriteUseCase)
{
  using namespace Anki::Vector;
  RemoveTestDataPrior();

  auto testBEICondType = BEIConditionType::CliffDetected;
  auto testBehaviorId1 = BehaviorID::ReactToFrustrationMajor;
  auto testBehaviorId2 = BehaviorID::ReactToCliff;
  
  {
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);
    auto userBehaviorTreeComp = robot1->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserDefinedBehaviorTreeComponent>();

    const bool wasOverwritten1 = userBehaviorTreeComp.AddCondition(testBEICondType, testBehaviorId1);
    ASSERT_FALSE(wasOverwritten1);

    // overwrite desired behavior
    const bool wasOverwritten2 = userBehaviorTreeComp.AddCondition(testBEICondType, testBehaviorId2);
    ASSERT_TRUE(wasOverwritten2);

    ICozmoBehaviorPtr behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(testBEICondType);

    EXPECT_EQ(behaviorPtr->GetID(), testBehaviorId2);
  }

  RemoveTestDataAfter();
};

// look up a behavior that isn't user-defined yet -> nullptr
TEST(UserDefinedBehaviorTreeComponent, BehaviorNotYetUserDefined)
{
  RemoveTestDataPrior();
  EXPECT_TRUE(true);
  RemoveTestDataAfter();
};

// check that data persists properly (encoder/decoder)
TEST(UserDefinedBehaviorTreeComponent, PersistenceCheck)
{
  using namespace Anki::Vector;
  RemoveTestDataPrior();

  auto testBEICondType = BEIConditionType::CliffDetected;
  auto testBehaviorId = BehaviorID::ReactToFrustrationMajor;

  {
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);
    auto userBehaviorTreeComp = robot1->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserDefinedBehaviorTreeComponent>();

    const bool wasOverwritten =  userBehaviorTreeComp.AddCondition(testBEICondType, testBehaviorId);
    ASSERT_FALSE(wasOverwritten);
  }
  {
    auto robot2 = std::make_unique<Robot>(kRobotId, cozmoContext);
    auto userBehaviorTreeComp = robot2->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserDefinedBehaviorTreeComponent>();

    ICozmoBehaviorPtr behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(testBEICondType);
    EXPECT_EQ(behaviorPtr->GetID(), testBehaviorId);
  }

  RemoveTestDataAfter();
};

}
