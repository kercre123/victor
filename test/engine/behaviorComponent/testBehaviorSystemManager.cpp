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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"
#include "gtest/gtest.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki::Cozmo;

void InjectValidDelegateIntoBSM(BehaviorSystemManager& bsm,
                                IBehavior* delegator,
                                IBehavior* delegated){
  auto iter = bsm._runnableStack->_delegatesMap.find(delegator);
  if(iter != bsm._runnableStack->_delegatesMap.end()){
    iter->second.insert(delegated);
  }
}

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
  const int arbitraryDelegationNumber = 100;
  std::vector<std::unique_ptr<TestSuperPoweredRunnable>> bunchOfDelegates;
  for(int i = 0; i < arbitraryDelegationNumber; i++){
    bunchOfDelegates.push_back(std::make_unique<TestSuperPoweredRunnable>());
    bunchOfDelegates.back()->SetBehaviorContainer(behaviorContainer);
    bunchOfDelegates.back()->Init(bei);
    bunchOfDelegates.back()->OnEnteredActivatableScope();
    bunchOfDelegates.back()->WantsToBeActivated(bei);
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, bunchOfDelegates.back().get());
    
    bsm.Delegate(bsm._runnableStack->GetTopOfStack(),
                 bunchOfDelegates.back().get());
    
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

  // TODO: Set up a BSM and then delegate to vairous combinations of
  // runnables, helpers, actions etc.
}



TEST(BehaviorSystemManager, TestCancelingDelegation)
{
  // TODO: Ensure that canceling delegates operates as expected
}
