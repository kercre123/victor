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

#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/cozmoContext.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
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
  
// class that is friends with UnitTestKey and ConditionUserIntentPending
class TestBehaviorHighLevelAI
{
public:
  explicit TestBehaviorHighLevelAI(std::shared_ptr<BehaviorHighLevelAI> behavior) {
    _transitionPairs = behavior->TESTONLY_GetAllTransitions({});
  }
  
  std::vector<std::pair<std::string, std::vector<IBEIConditionPtr>>> _transitionPairs;
                          
  bool AreStatesRespondingTo( UserIntentTag intentTag,
                              UserIntent* intent,
                              std::vector<std::string>& statesNotResponding )
  {
    bool anyResponded = false;
    statesNotResponding.clear();
    for( const auto& transitionPair : _transitionPairs ) {
      auto& stateName = transitionPair.first;
      bool handled = false;
      for( auto& transPtr : transitionPair.second ) {
        auto type = transPtr->GetConditionType();
        if( type == BEIConditionType::UserIntentPending ) {
          auto condition = std::dynamic_pointer_cast<ConditionUserIntentPending>( transPtr );
          EXPECT_TRUE( condition != nullptr ); 
          if( condition != nullptr ) {
            bool matchTag = false;
            for( const auto& evalStruct : condition->_evalList ) {
              if( evalStruct.tag == intentTag ) {
                matchTag = true;
                if( (intent != nullptr) && (evalStruct.intent != nullptr) ) {
                  handled |= (*intent == *evalStruct.intent.get());
                }
              }
            }
            if( (intent == nullptr) && matchTag ) {
              handled = true;
            }
          }
        }
        if( handled ) {
          break;
        }
      }
      if( handled ) {
        anyResponded = true;
      } else {
        statesNotResponding.push_back(stateName);
      }
    }
    return anyResponded;
  }
};
  
}
}

struct CompletedEntry {
  CompletedEntry(const std::string& s, const std::vector<std::string>& ex) : jsonStr(s), exceptions(ex) {}
  std::string jsonStr;
  Json::Value json;
  UserIntentTag tag = USER_INTENT(INVALID);
  std::vector<std::string> exceptions;
};

TEST(BehaviorHighLevelAI, VoiceIntentsHandled)
{
  // this test: for each cloud intent and app intent, and for each state in HL AI, check that the
  // corresponding user intent is handled, unless there's an explicit exception here. This cares only
  // about the user intents for which there are cloud/app intents because that probably means they
  // were at least partially hooked up. Eventually these will be replaced by DialogFlow data. For now,
  // when you finish a new intent related to HL AI, add it to the list below as a cloud intent, and
  // this test will compare HLAI, the list below, and user_intent_map for any discrepancies.
  
  using CompletedList = std::vector<CompletedEntry>;
  
  // *********************************************************
  // MODIFY THIS LIST WHEN YOU ADD AN INTENT TO HighLevelAI
  
  // some cloud intents that you _think_ youre done hooking up, each paired with the list of
  // state names that should intentionally not be responding to that intent
  CompletedList completedList = {
    {R"json({
      "intent": "intent_play_specific",
      "params": { "entity_behavior": "fist_bump" }
    })json", {"Napping", "NappingOnCharger"}},
    {R"json({
      "intent": "intent_play_specific",
      "params": { "entity_behavior": "keep_away" }
    })json", {"Napping", "NappingOnCharger"}}, 
    {R"json({
      "intent": "intent_play_specific",
      "params": { "entity_behavior": "roll_cube" }
    })json", {"Napping", "NappingOnCharger"}},
    {R"json({
      "intent": "intent_system_sleep"
    })json", {"Napping", "NappingOnCharger"}},
    {R"json({
      "intent": "intent_system_charger"
    })json", {"ObservingOnCharger",
              "ObservingOnChargerRecentlyPlaced",
              "Napping",
              "NappingOnCharger",
              "WakingUp",
              "FailedToFindCharger"
             }}
  };

  // OK YOUR JOB IS DONE
  // *********************************************************

  // convert to json and figure out tag

  UserIntentMap intentMap( cozmoContext->GetDataLoader()->GetUserIntentConfig() );
  for( auto& entry : completedList ) {
    Json::Reader reader;
    const bool parsedOK = reader.parse(entry.jsonStr, entry.json, false);
    ASSERT_TRUE( parsedOK );
    if( parsedOK ) {
      const auto& cloudIntentName = entry.json["intent"].asString();
      entry.tag = intentMap.GetUserIntentFromCloudIntent(cloudIntentName);
      ASSERT_TRUE( entry.tag != USER_INTENT(INVALID) );
    }
  }

  auto completedListContains = [&completedList](UserIntentTag intentTag) {
    for( const auto& entry : completedList ) {
      if( entry.tag == intentTag ) {
        return true;
      }
    }
    return false;
  };

  auto getExceptionsForTag = [&completedList](UserIntentTag intentTag) {
    std::vector<std::string> expectedExceptions;
    for( const auto& entry : completedList ) {
      if( entry.tag == intentTag ) {
        expectedExceptions.insert(expectedExceptions.end(), entry.exceptions.begin(), entry.exceptions.end());
      }
    }
    return expectedExceptions;
  };

  auto testExceptions = [](UserIntentTag intentTag, const std::vector<std::string>& actualExceptions, const std::vector<std::string>& expectedExceptions ) {
    for( const auto& actualException : actualExceptions ) {
      const auto itFound = std::find( expectedExceptions.begin(), expectedExceptions.end(), actualException );
      EXPECT_TRUE( itFound != expectedExceptions.end() )
        << "State " << actualException << " does not respond to " << UserIntentTagToString(intentTag);
      // you should add a transition or add this state as an exception in this test (see completedList)
    }
    // and the inverse
    for( const auto& expectedException : expectedExceptions ) {
      const auto itFound = std::find( actualExceptions.begin(), actualExceptions.end(), expectedException );
      EXPECT_TRUE( itFound != actualExceptions.end() )
        << "State " << expectedException << " responds to " << UserIntentTagToString(intentTag) << " but listed as an exception";
      // you should update this test to remove the exception (see completedList)
    }
  };

  TestBehaviorFramework testBehaviorFramework;
  testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true);
  
  std::shared_ptr<BehaviorHighLevelAI> behavior;
  const auto& BC = testBehaviorFramework.GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(HighLevelAI),
                                  BEHAVIOR_CLASS(HighLevelAI),
                                  behavior );

  TestBehaviorHighLevelAI testHLAI( behavior );
  
  auto cloudIntentsList = intentMap.DevGetCloudIntentsList();
  auto appIntentsList = intentMap.DevGetAppIntentsList();

  // cloud: use the sample list, but also fail if user intents in the user_intent_map are not
  // in the sample map but are hooked up. this means first testing just the tags in the user_intent_map
  // and failing if they are not in the sample list, then testing all the intents in the sample list.
  // this will all simplify once we have a list of sample intents to test.

  // testing just the tags in user_intent_map
  for( const auto& cloudIntent : cloudIntentsList ) {
    auto intentTag = intentMap.GetUserIntentFromCloudIntent( cloudIntent );
    ASSERT_NE( intentTag, USER_INTENT(unmatched_intent) );
    std::vector<std::string> statesNotResponding;
    const bool anyResponding = testHLAI.AreStatesRespondingTo( intentTag, nullptr, statesNotResponding );
    // if anyResponding, there should be an entry in completedList.
    const bool containsIntent = completedListContains( intentTag );
    if( anyResponding ) {
      EXPECT_TRUE( containsIntent ) << "State " << cloudIntent << " is hooked up but is not in the completed list";
    }
    
    // verify that the not-responding-list matches what's expected. since this for loop is over cloud
    // intent tags, assemble the exceptions for any sample with that tag
    std::vector<std::string> expectedExceptions = getExceptionsForTag(intentTag);
    
    // if this cloud intent is in the completed list, exceptions should match
    if( containsIntent ) {
      testExceptions(intentTag, statesNotResponding, expectedExceptions);
    }
  }
        
  // cloud: testing the samples
  for( const auto& completedEntry : completedList ) {
    auto& uic = testBehaviorFramework.GetBehaviorComponent().GetComponent<UserIntentComponent>();
    
    uic.SetCloudIntentPendingFromJSON( completedEntry.jsonStr );
    
    UserIntent intent;
    const bool isPending = uic.IsUserIntentPending( completedEntry.tag, intent );
    EXPECT_TRUE( isPending );
    uic.ClearUserIntent( completedEntry.tag );
    
    const auto& intentTag = intent.GetTag();
    EXPECT_NE( intentTag, USER_INTENT(INVALID) );
    
    // only supply an intent if there is data beyond the tag
    UserIntent* intentPtr = (intent.GetJSON().size() == 1) ? nullptr : &intent;
    
    std::vector<std::string> statesNotResponding;
    const bool anyResponding = testHLAI.AreStatesRespondingTo( intentTag, intentPtr, statesNotResponding );
    EXPECT_TRUE( anyResponding ) << "Completed list contains " << UserIntentTagToString(intent.GetTag()) << " but is not handled in HLAI";
    
    const auto& expectedExceptions = completedEntry.exceptions;
    testExceptions(intentTag, statesNotResponding, expectedExceptions);
  }
  
  // app: test the tags in user_intent_map
  for( const auto& appIntent : appIntentsList ) {
    auto intentTag = intentMap.GetUserIntentFromAppIntent( appIntent );
    ASSERT_NE( intentTag, USER_INTENT(unmatched_intent) );
    
    std::vector<std::string> statesNotResponding;
    const bool anyResponding = testHLAI.AreStatesRespondingTo( intentTag, nullptr, statesNotResponding );
    const bool containsIntent = completedListContains( intentTag );
    if( anyResponding ) {
      EXPECT_TRUE( containsIntent ) << "State " << appIntent << " is hooked up but is not in the completed list";
    }
    
    // verify that the not-responding-list matches what's expected. since this for loop is over cloud
    // intent tags, assemble the exceptions for any sample with that tag
    std::vector<std::string> expectedExceptions = getExceptionsForTag(intentTag);
    
    // if this app intent is in the completed list, exceptions should match
    if( containsIntent ) {
      testExceptions(intentTag, statesNotResponding, expectedExceptions);
    }
  }
  
  
  
}

