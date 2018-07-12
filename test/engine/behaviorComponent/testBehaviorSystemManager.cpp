/**
* File: testBehaviorSystemManager
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Ensure that the BehaviorSystemManager's public interface
* works as expected
*
* Copyright: Anki, Inc. 2017
*
* --gtest_filter=BehaviorSystemManager*
**/

// Access protected BSM functions for test purposes
#define private public
#define protected public

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "gtest/gtest.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/helpers/boundedWhile.h"

using namespace Anki::Cozmo;



TEST(BehaviorSystemManager, TestDelegationVariants)
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

  // Check to make sure that the stack exists and control is appropriately delegated
  ASSERT_TRUE(bsm._behaviorStack->IsInStack(baseBehavior.get()));
  IBehavior* behaviorDelegating = bsm._behaviorStack->GetTopOfStack();
  ASSERT_NE(behaviorDelegating, nullptr);
  EXPECT_FALSE(bsm.IsControlDelegated(behaviorDelegating));


  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredBehavior>> bunchOfDelegates;
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredBehavior>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated();
    InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, bunchOfDelegates.back().get());

    EXPECT_TRUE(bsm.Delegate(bsm._behaviorStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));

    // Assert control is delegated properly
    for(auto& entry: bunchOfDelegates){
      if(entry == bunchOfDelegates.back()){
        EXPECT_FALSE(bsm.IsControlDelegated(entry.get()));
      }else{
        EXPECT_TRUE(bsm.IsControlDelegated(entry.get()));
      }
    }

    behaviorDelegating = bunchOfDelegates.back().get();
  }

  // clear the stack
  bsm.CancelSelf(baseBehavior.get());
  ASSERT_EQ(bsm._behaviorStack->_behaviorStack.size(), 0);

  // Ensure that injecting a behavior into the stack works properly
  ICozmoBehavior* waitBehavior = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(Wait)).get();
  waitBehavior->OnEnteredActivatableScope();

  InjectAndDelegate(testFramework,
                    bsm._behaviorStack->GetTopOfStack(),
                    waitBehavior);
}



TEST(BehaviorSystemManager, TestCancelingDelegation)
{
  // TODO: Ensure that canceling delegates operates as expected
  std::unique_ptr<TestSuperPoweredBehavior> baseBehavior = std::make_unique<TestSuperPoweredBehavior>();
  TestBehaviorFramework testFramework(1, nullptr);
  auto initializeBehavior = [&baseBehavior](const BehaviorComponent::ComponentPtr& comps) {
    baseBehavior->SetBehaviorContainer(comps->GetComponent(BCComponentID::BehaviorContainer).GetComponent<BehaviorContainer>());
  };
  testFramework.InitializeStandardBehaviorComponent(baseBehavior.get(),initializeBehavior);

  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  BehaviorExternalInterface& bei = testFramework.GetBehaviorExternalInterface();
  BehaviorContainer& behaviorContainer = testFramework.GetBehaviorContainer();

  // Check to make sure that the stack exists and control is appropriately delegated
  ASSERT_TRUE(bsm._behaviorStack->IsInStack(baseBehavior.get()));
  IBehavior* behaviorDelegating = bsm._behaviorStack->GetTopOfStack();
  ASSERT_NE(behaviorDelegating, nullptr);
  EXPECT_FALSE(bsm.IsControlDelegated(behaviorDelegating));


  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredBehavior>> bunchOfDelegates;
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredBehavior>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated();
    InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, bunchOfDelegates.back().get());

    EXPECT_TRUE(bsm.Delegate(bsm._behaviorStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));

    // Assert control is delegated properly
    for(auto& entry: bunchOfDelegates){
      if(entry == bunchOfDelegates.back()){
        EXPECT_FALSE(bsm.IsControlDelegated(entry.get()));
      }else{
        EXPECT_TRUE(bsm.IsControlDelegated(entry.get()));
      }
    }

    behaviorDelegating = bunchOfDelegates.back().get();
  }

  // Cancel delegates from the middle of the stack and re-delegate
  const int arbitraryCancelNumber = 47;
  ASSERT_TRUE(arbitraryCancelNumber < arbitraryDelegationNumber);
  {
    IBehavior* canceler = bunchOfDelegates[arbitraryCancelNumber].get();
    bsm.CancelDelegates(canceler);
    ASSERT_TRUE(bsm._behaviorStack->GetTopOfStack() == canceler);
    // Size is cancel + 2 for base behavior and the one still on top
    const int idxOnTop = arbitraryCancelNumber + 2;
    EXPECT_EQ(bsm._behaviorStack->_behaviorStack.size(), idxOnTop);
  }

  behaviorDelegating = bsm._behaviorStack->GetTopOfStack();
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredBehavior>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated();
    InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, bunchOfDelegates.back().get());

    EXPECT_TRUE(bsm.Delegate(bsm._behaviorStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));

    behaviorDelegating = bunchOfDelegates.back().get();
  }

  // Cancel self from the middle of the stack and re-delegate
  const int arbitraryCancelNumber2 = 10;
  ASSERT_TRUE(arbitraryCancelNumber2 < arbitraryCancelNumber);
  {
    IBehavior* canceler = bunchOfDelegates[arbitraryCancelNumber2].get();
    bsm.CancelSelf(canceler);
    ASSERT_FALSE(bsm._behaviorStack->IsInStack(canceler));
    // Size is cancel + 1 for base behavior
    const int idxOnTop = arbitraryCancelNumber2 + 1;
    EXPECT_EQ(bsm._behaviorStack->_behaviorStack.size(), idxOnTop);
  }

  behaviorDelegating = bsm._behaviorStack->GetTopOfStack();
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredBehavior>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated();
    InjectValidDelegateIntoBSM(testFramework, behaviorDelegating, bunchOfDelegates.back().get());

    EXPECT_TRUE(bsm.Delegate(bsm._behaviorStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));

    behaviorDelegating = bunchOfDelegates.back().get();
  }

  // Clear the stack
  bsm.CancelSelf(baseBehavior.get());
  ASSERT_EQ(bsm._behaviorStack->_behaviorStack.size(), 0);
}
