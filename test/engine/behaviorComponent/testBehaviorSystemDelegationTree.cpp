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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/boundedWhile.h"


using namespace Anki::Cozmo;

void RecursiveDelegation(Robot& robot,
                         TestBehaviorFramework& testFramework,
                         std::map<IBehavior*,std::set<IBehavior*>>& delegateMap)
{
  robot.GetActionList().Update();
  if(delegateMap.empty()){
    return;
  }else{
    auto& bsm = testFramework.GetBehaviorSystemManager();
    IBehavior* topOfStack = bsm._behaviorStack->GetTopOfStack();
    auto iter = delegateMap.find(topOfStack);
    if(iter != delegateMap.end()){
      for(auto& delegate: iter->second){
        delegate->WantsToBeActivated();

        // cancel all delegates (including actions) because the behaviors OnActivated may have delegated to
        // something
        auto& delegationComponent = testFramework.GetBehaviorExternalInterface().GetDelegationComponent();
        delegationComponent.CancelDelegates(topOfStack);

        bsm.Delegate(topOfStack, delegate);
        RecursiveDelegation(robot, testFramework, delegateMap);
      }
      delegateMap.erase(iter);
      bsm.CancelSelf(topOfStack);
    }else{
      std::set<IBehavior*> tmpDelegates;
      topOfStack->GetAllDelegates(tmpDelegates);
      delegateMap.insert(std::make_pair(topOfStack, std::move(tmpDelegates)));
      RecursiveDelegation(robot, testFramework, delegateMap);
    }
  }
}


TEST(DelegationTree, FullTreeWalkthrough)
{
  // Read in the current behavior system configuration
  // and then walk through the full tree appropirately activating
  // and deactivating all delegates to ensure the tree is valid
  TestBehaviorFramework testFramework(1, nullptr);
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
  bsm._behaviorStack->InitBehaviorStack(baseBehavior);
  IBehavior* bottomOfStack = bsm._behaviorStack->GetTopOfStack();
  
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  bottomOfStack->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(bottomOfStack, tmpDelegates));
  RecursiveDelegation(testFramework.GetRobot(), testFramework, delegateMap);
}



TEST(DelegationTree, DesignedControlTest)
{
  // TODO: Read in the current behavior system configuration
  // then walk through the tree and ensure that all design requirements
  // are met. E.G. every behavior must be able to transition to V.C. when
  // necessary.
  
}

TEST(DelegationTree, DumpBehaviorTransitionsToFile)
{
  // the accompanying python script will be looking for this file
  std::string outFilename;
  char* szFilename = getenv("ANKI_TEST_BEHAVIOR_FILE");
  if( szFilename != nullptr ) {
    outFilename = szFilename;
  } else {
    return;
  }
  
  TestBehaviorFramework testFramework(1, nullptr);
  testFramework.InitializeStandardBehaviorComponent();
  
  const auto* dataLoader = testFramework.GetRobot().GetContext()->GetDataLoader();
  ASSERT_NE( dataLoader, nullptr ) << "Cannot test behaviors if no data loader exists";
  
  const auto& bc = testFramework.GetBehaviorContainer();
  const auto& behaviorMap = bc.GetBehaviorMap();
  
  std::stringstream ss;
  
  for( const auto& behPair : behaviorMap ) {
    
    std::string id = Anki::Cozmo::BehaviorTypesWrapper::BehaviorIDToString( behPair.first );
    const ICozmoBehaviorPtr behavior = behPair.second;
    
    std::set<IBehavior*> delegates;
    behavior->GetAllDelegates( delegates );
    for( const auto* delegate : delegates ) {
      
      std::string outId = delegate->GetDebugLabel();
      // skip trailing digits that were added to make labels unique
      for( auto it = outId.rbegin(); it != outId.rend(); ) {
        if( std::isdigit(*it) ) {
          ++it;
          outId.pop_back();
        } else {
          break;
        }
      }
      ss << id << " " << outId << std::endl;
      
    }
    
  }
  
  auto res = Anki::Util::FileUtils::WriteFile( outFilename, ss.str() );
  EXPECT_EQ(res, true) << "Error writing file " << outFilename;
}
