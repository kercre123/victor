/**
 * File: testBehaviorHighLevelAI
 *
 * Author: ross
 * Created: 2018 feb 23
 *
 * Description: Tests related to the BehaviorHighLevelAI
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/cozmoContext.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/behaviorComponent/userIntentMap.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/unitTestKey.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Cozmo;


extern CozmoContext* cozmoContext;

namespace Anki {
namespace Cozmo {
  
using CompletedList = std::map<UserIntentTag, std::vector<std::string>>;
  
// class that is friends with UnitTestKey and ConditionUserIntentPending
class TestBehaviorHighLevelAI
{
public:
  
  // for each state in HL AI behavior, if the intent is triggered, is it handled? It should be
  // if the user intent is in the completedList, unless it is listed as an exception
  static void RunTest(std::shared_ptr<BehaviorHighLevelAI>& behavior,
                      UserIntentTag intent,
                      const CompletedList& completedList )
  {
    auto transitionPairs = behavior->TESTONLY_GetAllTransitions({});
    for( const auto& transitionPair : transitionPairs ) {
      auto& stateName = transitionPair.first;
      bool handled = false;
      for( auto& transPtr : transitionPair.second ) {
        auto type = transPtr->GetConditionType();
        if( type == BEIConditionType::UserIntentPending ) {
          auto condition = std::dynamic_pointer_cast<ConditionUserIntentPending>( transPtr );
          ASSERT_NE( condition, nullptr );
          for( const auto& evalStruct : condition->_evalList ) {
            if( evalStruct.tag == intent ) {
              // consider it handled, even if there are also intents or lambdas
              handled = true;
              break;
            }
          }
        }
        if( handled ) {
          break;
        }
      }
      
      bool expectHandled = false;
      const auto itCompleted = completedList.find(intent);
      if( itCompleted != completedList.end() ) {
        const auto itExempt = std::find( itCompleted->second.begin(), itCompleted->second.end(), stateName );
        expectHandled = (itExempt == itCompleted->second.end());
      }
      
      if( expectHandled ) {
        // you need to either fix it or add an explicit exception in completedTags in VoiceIntentsHandled
        EXPECT_TRUE( handled ) << stateName << " does not transition for " << UserIntentTagToString(intent);
      } else {
        // If you just added a new intent, add it to completedTags in VoiceIntentsHandled
        EXPECT_FALSE( handled ) << stateName << " transitions for " << UserIntentTagToString(intent) << " but is not listed here";
      }
      
    }
  }
};
  
}
}

TEST(BehaviorHighLevelAI, VoiceIntentsHandled)
{
  // this test: for each cloud intent and app intent, and for each state in HL AI, check that the
  // corresponding user intent is handled, unless there's an explicit exception here. This cares only
  // about the user intents for which there are cloud/app intents because that probably means they
  // were at least partially hooked up. Eventually these will be replaced by DialogFlow data. For now,
  // when you finish a new intent related to HL AI, add it to the list below
  
  // COMPLETED TAGS and EXCEPTIONS: the user intents you _think_ youre done hooking up, and the list
  // of state names that should intentionally not be responding to that intent
  CompletedList completedTags = {
    { USER_INTENT(play_specific), {} },
    { USER_INTENT(play_anytrick), {} },
    { USER_INTENT(system_charger), {"ObservingOnCharger",
                                    "ObservingOnChargerRecentlyPlaced",
                                    "Napping", // napping on charger is not a state yet. these
                                    "WakingUp",  // will
                                    "FailedToFindCharger" // change
                                    } }
  };
  
  
  TestBehaviorFramework testBehaviorFramework;
  testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true);
  
  std::shared_ptr<BehaviorHighLevelAI> behavior;
  const auto& BC = testBehaviorFramework.GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(HighLevelAI),
                                  BEHAVIOR_CLASS(HighLevelAI),
                                  behavior );
  
  UserIntentMap intentMap( cozmoContext->GetDataLoader()->GetUserIntentConfig() );
  auto cloudIntentsList = intentMap.DevGetCloudIntentsList();
  auto appIntentsList = intentMap.DevGetAppIntentsList();
  
  // this uses the class TestBehaviorHighLevelAI instead of making the implicit
  // test class BehaviorHighLevelAI_VoiceIntentsHandled_Test a friend in case
  // someone else wants to use that class.
  
  // cloud
  for( const auto& cloudIntent : cloudIntentsList ) {
    auto intent = intentMap.GetUserIntentFromCloudIntent(cloudIntent);
    ASSERT_NE( intent, USER_INTENT(unmatched_intent) );
    TestBehaviorHighLevelAI::RunTest( behavior, intent, completedTags );
  }
  
  // app
  for( const auto& appIntent : appIntentsList ) {
    auto intent = intentMap.GetUserIntentFromAppIntent( appIntent );
    ASSERT_NE( intent, USER_INTENT(unmatched_intent) );
    TestBehaviorHighLevelAI::RunTest( behavior, intent, completedTags );
  }
  
  
  
}

