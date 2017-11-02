/**
* File: testBehaviorDleegationTree
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Set of test functions that walk through cozmo's data defined
* behavior tree that ensure all possible behavior states are valid and
* within design's constraints
*
* Copyright: Anki, Inc. 2017
*
* --gtest_filter=DelegationTree*
**/

#define private public
#define protected public

#include "gtest/gtest.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/helpers/boundedWhile.h"


using namespace Anki::Cozmo;

void RecursiveDelegation(Robot& robot,
                         BehaviorSystemManager& bsm,
                         std::map<IBehavior*,std::set<IBehavior*>>& delegateMap)
{
  robot.GetActionList().Update();
  if(delegateMap.empty()){
    return;
  }else{
    IBehavior* topOfStack = bsm._behaviorStack->GetTopOfStack();
    auto iter = delegateMap.find(topOfStack);
    if(iter != delegateMap.end()){
      for(auto& delegate: iter->second){
        bsm.Delegate(topOfStack, delegate);
        RecursiveDelegation(robot, bsm, delegateMap);
      }
      delegateMap.erase(iter);
      bsm.CancelSelf(iter->first);
    }else{
      std::set<IBehavior*> tmpDelegates;
      topOfStack->GetAllDelegates(tmpDelegates);
      delegateMap.insert(std::make_pair(topOfStack, std::move(tmpDelegates)));
      RecursiveDelegation(robot, bsm, delegateMap);
    }
  }
}


TEST(DelegationTree, FullTreeWalkthrough)
{
  // Read in the current behavior system configuration
  // and then walk through the full tree appropirately activating
  // and deactivating all delegates to ensure the tree is valid
  TestBehaviorFramework testFramework;
  testFramework.InitializeStandardBehaviorComponent();
  IBehavior* baseBehavior = nullptr;
  // Load base behavior in from data
  {
    auto dataLoader = testFramework.GetRobot().GetContext()->GetDataLoader();
    
    Json::Value blankActivitiesConfig;

    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
           dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;

    BehaviorID baseBehaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
    
    auto& bc = testFramework.GetBehaviorContainer();
    baseBehavior = bc.FindBehaviorByID(baseBehaviorID).get();
    DEV_ASSERT(baseBehavior != nullptr,
               "BehaviorComponent.Init.InvalidbaseBehavior");
  }
  
  // Clear out the default stack and put the base behavior on the stack
  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  bsm._behaviorStack->ClearStack();
  bsm._behaviorStack->InitBehaviorStack(testFramework.GetBehaviorExternalInterface(),
                                        baseBehavior);
  IBehavior* bottomOfStack = bsm._behaviorStack->GetTopOfStack();
  
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  bottomOfStack->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(bottomOfStack, tmpDelegates));
  RecursiveDelegation(testFramework.GetRobot(), bsm, delegateMap);
}



TEST(DelegationTree, DesignedControlTest)
{
  // TODO: Read in the current behavior system configuration
  // then walk through the tree and ensure that all design requirements
  // are met. E.G. every behavior must be able to transition to V.C. when
  // necessary.
  
}
