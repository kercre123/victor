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

TEST(DelegationTree, FullTreeWalkthrough)
{
  // Read in the current behavior system configuration
  // and then walk through the full tree appropirately activating
  // and deactivating all delegates to ensure the tree is valid
  TestBehaviorFramework testFramework(1, nullptr);
  testFramework.InitializeStandardBehaviorComponent();
  testFramework.SetDefaultBaseBehavior();

  BehaviorSystemManager& bsm = testFramework.GetBehaviorSystemManager();
  IBehavior* bottomOfStack = bsm._behaviorStack->_behaviorStack.front();
  auto& delegationComponent = testFramework.GetBehaviorExternalInterface().GetDelegationComponent();
  delegationComponent.CancelDelegates(bottomOfStack);
  
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  bottomOfStack->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(bottomOfStack, tmpDelegates));
  testFramework.FullTreeWalk(delegateMap);
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
