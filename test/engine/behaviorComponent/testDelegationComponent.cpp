/**
* File: testDelegationComponent
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Ensure that the BehaviorSystemManager's public interface
* works as expected
*
* Copyright: Anki, Inc. 2017
*
* --gtest_filter=DelegationComponent*
**/

#define private public
#define protected public

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "gtest/gtest.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/helpers/boundedWhile.h"

using namespace Anki::Vector;

TEST(DelegationComponent, TestDelegationVariants)
{
  std::unique_ptr<TestSuperPoweredBehavior> baseBehavior = std::make_unique<TestSuperPoweredBehavior>();
  TestBehaviorFramework testFramework(1, nullptr);
  auto initializeBehavior = [&baseBehavior](const BehaviorComponent::ComponentPtr& comps) {
    baseBehavior->SetBehaviorContainer(comps->GetComponent(BCComponentID::BehaviorContainer).GetComponent<BehaviorContainer>());
  };
  testFramework.InitializeStandardBehaviorComponent(baseBehavior.get(),initializeBehavior);

  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  BehaviorExternalInterface& bei = testFramework.GetBehaviorExternalInterface();
  BehaviorContainer& behaviorContainer = testFramework.GetBehaviorContainer();
  ASSERT_TRUE(bei.HasDelegationComponent());
  DelegationComponent& delegationComp = bei.GetDelegationComponent();

  // Check to make sure that the stack exists and control is appropriately delegated
  IBehavior* behaviorDelegating = bsm._behaviorStack->GetTopOfStack();
  ASSERT_NE(behaviorDelegating, nullptr);
  EXPECT_FALSE(delegationComp.IsControlDelegated(behaviorDelegating));


  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredBehavior>> bunchOfBehaviors;
  std::vector<std::unique_ptr<ICozmoBehavior>> bunchOfCozmoBehaviors;

  {
    for(int i = 0; i < arbitraryDelegationNumber; i++){
      bunchOfBehaviors.push_back(std::make_unique<TestSuperPoweredBehavior>());
      auto& toDelegate = bunchOfBehaviors.back();
      toDelegate->SetBehaviorContainer(behaviorContainer);
      toDelegate->Init(bei);
      toDelegate->OnEnteredActivatableScope();
      toDelegate->WantsToBeActivated();
      InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, toDelegate.get());

      ASSERT_TRUE(delegationComp.HasDelegator(behaviorDelegating));
      auto& delegatorComp = delegationComp.GetDelegator(behaviorDelegating);

      EXPECT_TRUE(delegatorComp.Delegate(behaviorDelegating,
                                         toDelegate.get()));

      // Assert control is delegated properly
      for(auto& entry: bunchOfBehaviors){
        if(entry == bunchOfBehaviors.back()){
          EXPECT_FALSE(delegationComp.IsControlDelegated(entry.get()));
        }else{
          EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
        }
      }

      behaviorDelegating = bunchOfBehaviors.back().get();
    }
  }

  // Delegate an arbitrarily large number of times to cozmoBehaviors
  {
    Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BEHAVIOR_CLASS(Wait), BEHAVIOR_ID(Wait));
    for(int i = 0; i < arbitraryDelegationNumber; i++){
      bunchOfCozmoBehaviors.push_back(std::make_unique<TestBehavior>(empty));
      auto& toDelegate = bunchOfCozmoBehaviors.back();
      toDelegate->Init(bei);
      toDelegate->OnEnteredActivatableScope();
      toDelegate->WantsToBeActivated();
      InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, toDelegate.get());

      ASSERT_TRUE(delegationComp.HasDelegator(behaviorDelegating));
      auto& delegatorComp = delegationComp.GetDelegator(behaviorDelegating);

      EXPECT_TRUE(delegatorComp.Delegate(behaviorDelegating,
                                         toDelegate.get()));

      // Assert control is delegated properly
      for(auto& entry: bunchOfCozmoBehaviors){
        if(entry == bunchOfCozmoBehaviors.back()){
          EXPECT_FALSE(delegationComp.IsControlDelegated(entry.get()));
        }else{
          EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
        }
      }

      behaviorDelegating = bunchOfCozmoBehaviors.back().get();
    }
  }

  // Delegate to an action
  {
    DriveStraightAction* action = new DriveStraightAction(0);
    ASSERT_TRUE(delegationComp.HasDelegator(behaviorDelegating));
    auto& delegatorComp = delegationComp.GetDelegator(behaviorDelegating);
    EXPECT_TRUE(delegatorComp.Delegate(behaviorDelegating,
                                       action));
    // Make sure that nothing thinks it has control
    for(auto& entry: bunchOfBehaviors){
      EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
    }
    for(auto& entry: bunchOfCozmoBehaviors){
      EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
    }
  }

  // Test Canceling a behavior in the middle of the stack's delegates
  delegationComp.CancelDelegates(bunchOfCozmoBehaviors.front().get());
  ASSERT_FALSE(delegationComp.IsControlDelegated(bunchOfCozmoBehaviors.back().get()));

  // Cancel a behavior in the middle of the stack
  delegationComp.CancelSelf(bunchOfBehaviors.back().get());
  ASSERT_FALSE(delegationComp.IsControlDelegated(bunchOfBehaviors.back().get()));
  ASSERT_FALSE(testFramework.GetBehaviorSystemManager()._behaviorStack->IsInStack(bunchOfBehaviors.back().get()));
  std::unique_ptr<IBehavior> keepBehaviorValid = std::move(bunchOfBehaviors.back());
  bunchOfBehaviors.pop_back();
  // Delegate to an action and cancel it
  {
    DriveStraightAction* action = new DriveStraightAction(0);
    ASSERT_TRUE(delegationComp.HasDelegator(bunchOfBehaviors.back().get()));
    auto& delegatorComp = delegationComp.GetDelegator(bunchOfBehaviors.back().get());
    EXPECT_TRUE(delegatorComp.Delegate(bunchOfBehaviors.back().get(),
                                       action));
    ASSERT_TRUE(delegationComp.IsControlDelegated(bunchOfBehaviors.back().get()));
    delegationComp.CancelDelegates(bunchOfBehaviors.back().get());
    ASSERT_FALSE(delegationComp.IsControlDelegated(bunchOfBehaviors.back().get()));
  }

  // Cancel the bottom of the stack
  std::vector<IBehavior*>& behaviorStack = testFramework.GetBehaviorSystemManager()._behaviorStack->_behaviorStack;
  IBehavior* bottomElement = behaviorStack.front();
  delegationComp.CancelSelf(bottomElement);
  ASSERT_TRUE(behaviorStack.empty());
}
