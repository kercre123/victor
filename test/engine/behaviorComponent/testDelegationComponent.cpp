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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
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

TEST(DelegationComponent, TestDelegationVariants)
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
  ASSERT_TRUE(bei.HasDelegationComponent());
  DelegationComponent& delegationComp = bei.GetDelegationComponent();
  
  // Check to make sure that the stack exists and control is appropriately delegated
  IBehavior* runnableDelegating = bsm._runnableStack->GetTopOfStack();
  ASSERT_NE(runnableDelegating, nullptr);
  EXPECT_FALSE(delegationComp.IsControlDelegated(runnableDelegating));
  
  
  // Build up an arbitrarily large stack of delegates
  const int arbitraryDelegationNumber = 50;
  std::vector<std::unique_ptr<TestSuperPoweredRunnable>> bunchOfBehaviors;
  std::vector<std::unique_ptr<ICozmoBehavior>> bunchOfCozmoBehaviors;
  std::vector<HelperHandle> bunchOfHelpers;

  {
    for(int i = 0; i < arbitraryDelegationNumber; i++){
      bunchOfBehaviors.push_back(std::make_unique<TestSuperPoweredRunnable>());
      auto& toDelegate = bunchOfBehaviors.back();
      toDelegate->SetBehaviorContainer(behaviorContainer);
      toDelegate->Init(bei);
      toDelegate->OnEnteredActivatableScope();
      toDelegate->WantsToBeActivated(bei);
      InjectValidDelegateIntoBSM(bsm, runnableDelegating, toDelegate.get());
      
      ASSERT_TRUE(delegationComp.HasDelegator(runnableDelegating));
      auto& delegatorComp = delegationComp.GetDelegator(runnableDelegating);
      
      EXPECT_TRUE(delegatorComp.Delegate(runnableDelegating,
                                         toDelegate.get()));
      
      // Assert control is delegated properly
      for(auto& entry: bunchOfBehaviors){
        if(entry == bunchOfBehaviors.back()){
          EXPECT_FALSE(delegationComp.IsControlDelegated(entry.get()));
        }else{
          EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
        }
      }
      
      runnableDelegating = bunchOfBehaviors.back().get();
    }
  }
  
  // Delegate an arbitrarily large number of times to cozmoBehaviors
  {
    Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
    for(int i = 0; i < arbitraryDelegationNumber; i++){
      bunchOfCozmoBehaviors.push_back(std::make_unique<TestBehavior>(empty));
      auto& toDelegate = bunchOfCozmoBehaviors.back();
      toDelegate->Init(bei);
      toDelegate->OnEnteredActivatableScope();
      toDelegate->WantsToBeActivated(bei);
      InjectValidDelegateIntoBSM(bsm, runnableDelegating, toDelegate.get());
      
      ASSERT_TRUE(delegationComp.HasDelegator(runnableDelegating));
      auto& delegatorComp = delegationComp.GetDelegator(runnableDelegating);
      
      EXPECT_TRUE(delegatorComp.Delegate(runnableDelegating,
                                         toDelegate.get()));
      
      // Assert control is delegated properly
      for(auto& entry: bunchOfCozmoBehaviors){
        if(entry == bunchOfCozmoBehaviors.back()){
          EXPECT_FALSE(delegationComp.IsControlDelegated(entry.get()));
        }else{
          EXPECT_TRUE(delegationComp.IsControlDelegated(entry.get()));
        }
      }
      
      runnableDelegating = bunchOfCozmoBehaviors.back().get();
    }
  }

  // Delegate to helper
  {
    auto& factory = testFramework.GetBehaviorComponent()._behaviorHelperComponent->GetBehaviorHelperFactory();
    bunchOfHelpers.push_back(factory.CreatePlaceBlockHelper(testFramework.GetBehaviorExternalInterface(),
                                                            *bunchOfCozmoBehaviors.back().get()));
    HelperHandle toDelegate = bunchOfHelpers.back();
    InjectValidDelegateIntoBSM(bsm, runnableDelegating, toDelegate.get(), false);
    
    ASSERT_TRUE(delegationComp.HasDelegator(runnableDelegating));
    auto& delegatorComp = delegationComp.GetDelegator(runnableDelegating);
    
    EXPECT_TRUE(delegatorComp.Delegate(runnableDelegating,
                                       testFramework.GetBehaviorExternalInterface(),
                                       toDelegate,
                                       nullptr,
                                       nullptr));
    
    // Cancel the action that the helper just delegated to in its init
    delegationComp.CancelDelegates(runnableDelegating);
  }
  
  // Delegate to an action
  {
    DriveStraightAction* action = new DriveStraightAction(testFramework.GetRobot(), 0);
    ASSERT_TRUE(delegationComp.HasDelegator(runnableDelegating));
    auto& delegatorComp = delegationComp.GetDelegator(runnableDelegating);
    EXPECT_TRUE(delegatorComp.Delegate(runnableDelegating,
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
  ASSERT_FALSE(testFramework.GetBehaviorSystemManager()._runnableStack->IsInStack(bunchOfBehaviors.back().get()));
  std::unique_ptr<IBehavior> keepBehaviorValid = std::move(bunchOfBehaviors.back());
  bunchOfBehaviors.pop_back();
  // Delegate to an action and cancel it
  {
    DriveStraightAction* action = new DriveStraightAction(testFramework.GetRobot(), 0);
    ASSERT_TRUE(delegationComp.HasDelegator(bunchOfBehaviors.back().get()));
    auto& delegatorComp = delegationComp.GetDelegator(bunchOfBehaviors.back().get());
    EXPECT_TRUE(delegatorComp.Delegate(bunchOfBehaviors.back().get(),
                                       action));
    ASSERT_TRUE(delegationComp.IsControlDelegated(bunchOfBehaviors.back().get()));
    delegationComp.CancelDelegates(bunchOfBehaviors.back().get());
    ASSERT_FALSE(delegationComp.IsControlDelegated(bunchOfBehaviors.back().get()));
  }
  
  // Cancel the bottom of the stack
  std::vector<IBehavior*>& runnableStack = testFramework.GetBehaviorSystemManager()._runnableStack->_runnableStack;
  IBehavior* bottomElement = runnableStack.front();
  delegationComp.CancelSelf(bottomElement);
  ASSERT_TRUE(runnableStack.empty());
}
