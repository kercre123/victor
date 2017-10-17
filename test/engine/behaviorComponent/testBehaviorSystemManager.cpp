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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"
#include "gtest/gtest.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/helpers/boundedWhile.h"

using namespace Anki::Cozmo;



TEST(BehaviorSystemManager, TestDelegationVariants)
{
  std::unique_ptr<TestSuperPoweredRunnable> baseRunnable = std::make_unique<TestSuperPoweredRunnable>();
  TestBehaviorFramework testFramework;
  auto initializeRunnable = [&baseRunnable](const BehaviorComponent::ComponentsPtr& comps){
    baseRunnable->SetBehaviorContainer(comps->_behaviorContainer);
  };
  testFramework.InitializeStandardBehaviorComponent(baseRunnable.get(),initializeRunnable);
  
  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  BehaviorExternalInterface& bei = testFramework.GetBehaviorExternalInterface();
  BehaviorContainer& behaviorContainer = testFramework.GetBehaviorContainer();

  // Check to make sure that the stack exists and control is appropriately delegated
  ASSERT_TRUE(bsm._runnableStack->IsInStack(baseRunnable.get()));
  IBehavior* runnableDelegating = bsm._runnableStack->GetTopOfStack();
  ASSERT_NE(runnableDelegating, nullptr);
  EXPECT_FALSE(bsm.IsControlDelegated(runnableDelegating));
  

  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredRunnable>> bunchOfDelegates;
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredRunnable>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated(bei);
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, bunchOfDelegates.back().get());
    
    EXPECT_TRUE(bsm.Delegate(bsm._runnableStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));
    
    // Assert control is delegated properly
    for(auto& entry: bunchOfDelegates){
      if(entry == bunchOfDelegates.back()){
        EXPECT_FALSE(bsm.IsControlDelegated(entry.get()));
      }else{
        EXPECT_TRUE(bsm.IsControlDelegated(entry.get()));
      }
    }
    
    runnableDelegating = bunchOfDelegates.back().get();
  }

  // clear the stack
  bsm.CancelSelf(baseRunnable.get());
  ASSERT_EQ(bsm._runnableStack->_runnableStack.size(), 1);
  
  // Ensure that injecting a behavior into the stack works properly
  ICozmoBehavior* waitBehavior = behaviorContainer.FindBehaviorByID(BehaviorID::Wait).get();
  waitBehavior->OnEnteredActivatableScope();

  InjectAndDelegate(testFramework.GetBehaviorSystemManager(),
                    bsm._runnableStack->GetTopOfStack(),
                    waitBehavior);
}



TEST(BehaviorSystemManager, TestCancelingDelegation)
{
  // TODO: Ensure that canceling delegates operates as expected
  std::unique_ptr<TestSuperPoweredRunnable> baseRunnable = std::make_unique<TestSuperPoweredRunnable>();
  TestBehaviorFramework testFramework;
  auto initializeRunnable = [&baseRunnable](const BehaviorComponent::ComponentsPtr& comps){
    baseRunnable->SetBehaviorContainer(comps->_behaviorContainer);
  };
  testFramework.InitializeStandardBehaviorComponent(baseRunnable.get(),initializeRunnable);
  
  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  BehaviorExternalInterface& bei = testFramework.GetBehaviorExternalInterface();
  BehaviorContainer& behaviorContainer = testFramework.GetBehaviorContainer();
  
  // Check to make sure that the stack exists and control is appropriately delegated
  ASSERT_TRUE(bsm._runnableStack->IsInStack(baseRunnable.get()));
  IBehavior* runnableDelegating = bsm._runnableStack->GetTopOfStack();
  ASSERT_NE(runnableDelegating, nullptr);
  EXPECT_FALSE(bsm.IsControlDelegated(runnableDelegating));
  
  
  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredRunnable>> bunchOfDelegates;
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredRunnable>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated(bei);
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, bunchOfDelegates.back().get());
    
    EXPECT_TRUE(bsm.Delegate(bsm._runnableStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));
    
    // Assert control is delegated properly
    for(auto& entry: bunchOfDelegates){
      if(entry == bunchOfDelegates.back()){
        EXPECT_FALSE(bsm.IsControlDelegated(entry.get()));
      }else{
        EXPECT_TRUE(bsm.IsControlDelegated(entry.get()));
      }
    }
    
    runnableDelegating = bunchOfDelegates.back().get();
  }
  
  // Cancel delegates from the middle of the stack and re-delegate
  const int arbitraryCancelNumber = 47;
  ASSERT_TRUE(arbitraryCancelNumber < arbitraryDelegationNumber);
  {
    IBehavior* canceler = bunchOfDelegates[arbitraryCancelNumber].get();
    bsm.CancelDelegates(canceler);
    ASSERT_TRUE(bsm._runnableStack->GetTopOfStack() == canceler);
    // Size is cancel + 3 for base runnable, dev runnable and the one still on top
    const int idxOnTop = arbitraryCancelNumber + 3;
    EXPECT_EQ(bsm._runnableStack->_runnableStack.size(), idxOnTop);
  }
  
  runnableDelegating = bsm._runnableStack->GetTopOfStack();
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredRunnable>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated(bei);
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, bunchOfDelegates.back().get());
    
    EXPECT_TRUE(bsm.Delegate(bsm._runnableStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));
    
    runnableDelegating = bunchOfDelegates.back().get();
  }
  
  // Cancel self from the middle of the stack and re-delegate
  const int arbitraryCancelNumber2 = 10;
  ASSERT_TRUE(arbitraryCancelNumber2 < arbitraryCancelNumber);
  {
    IBehavior* canceler = bunchOfDelegates[arbitraryCancelNumber2].get();
    bsm.CancelSelf(canceler);
    ASSERT_FALSE(bsm._runnableStack->IsInStack(canceler));
    // Size is cancel + 2 for base runnable and dev runnable
    const int idxOnTop = arbitraryCancelNumber2 + 2;
    EXPECT_EQ(bsm._runnableStack->_runnableStack.size(), idxOnTop);
  }
  
  runnableDelegating = bsm._runnableStack->GetTopOfStack();
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredRunnable>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated(bei);
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, bunchOfDelegates.back().get());
    
    EXPECT_TRUE(bsm.Delegate(bsm._runnableStack->GetTopOfStack(),
                             bunchOfDelegates.back().get()));
    
    runnableDelegating = bunchOfDelegates.back().get();
  }
  
  // Clear the stack
  bsm.CancelSelf(baseRunnable.get());
  ASSERT_EQ(bsm._runnableStack->_runnableStack.size(), 1);
}
