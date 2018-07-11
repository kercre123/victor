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
#include "coretech/common/engine/utils/timer.h"
#include "engine/cozmoContext.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/behaviorComponent/userIntentMap.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCompound.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/unitTestKey.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "test/engine/behaviorComponent/testIntentsFramework.h"

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
                              const UserIntent* intent,
                              std::vector<std::string>& statesNotResponding )
  {
    bool anyResponded = false;
    statesNotResponding.clear();
    for( const auto& transitionPair : _transitionPairs ) {
      auto& stateName = transitionPair.first;
      bool handled = false;
      for( auto transPtr : transitionPair.second ) {
        // check compound conditions to see if they contain UserIntentPending conditions
        auto type = transPtr->GetConditionType();
        if( type == BEIConditionType::Compound ) {
          auto condition = std::dynamic_pointer_cast<ConditionCompound>( transPtr );
          EXPECT_TRUE( condition != nullptr );
          const auto& operands = condition->TESTONLY_GetOperands({});
          for( auto& operand : operands ) {
            if( operand->GetConditionType() == BEIConditionType::UserIntentPending ) {
              type = BEIConditionType::UserIntentPending;
              transPtr = operand;
              break;
            }
          }
        }
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
  
  static bool IsStateRunning(std::shared_ptr<BehaviorHighLevelAI> beh, const std::string& stateName ) {
    return beh->TESTONLY_IsStateRunning({}, stateName);
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

TEST(BehaviorHighLevelAI, UserIntentsHandled)
{
  // this test: for each cloud intent and app intent, and for each state in HL AI, check that the
  // corresponding user intent is handled, unless there's an explicit exception here. This cares only
  // about the user intents for which there are cloud/app intents because that probably means they
  // were at least partially hooked up. Eventually these will be replaced by DialogFlow data. For now,
  // when you finish a new intent related to HL AI, add it to the list below as a cloud intent, and
  // this test will compare HLAI, the list below, and user_intent_map for any discrepancies.
  
  using ExceptionsList = std::vector<std::string>;
  using LabeledExceptions = std::unordered_map<std::string, ExceptionsList>;
  
  // *********************************************************
  // MODIFY THIS LIST WHEN YOU ADD AN INTENT TO HighLevelAI
  // There should be one entry per completed intent in completedUserIntents.json
  
  LabeledExceptions exceptionsList = {
    { "system_sleep",      {"Napping", "NappingOnCharger"} },
    { "system_charger",    {"ObservingOnCharger",
                            "ObservingOnChargerRecentlyPlaced",
                            "Napping",
                            "NappingOnCharger",
                            "WakingUp"} },
    { "explore_start",     {"Napping", "NappingOnCharger"} },
  };
  // OK YOUR JOB IS DONE
  // *********************************************************
  
  TestIntentsFramework tih;
  
  // use the intents framework to match the exceptions list to an actual intent
  std::unordered_map<std::string ,std::pair<UserIntent,ExceptionsList>> completedIntents;
  auto labels = tih.GetCompletedLabels( TestIntentsFramework::EHandledLocation::HighLevelAI );
  for( const auto& label : labels ) {
    const auto it = exceptionsList.find(label);
    ASSERT_FALSE( it == exceptionsList.end() ) << "Completed intent " << label << " not listed in exceptionsList";
    const auto& exceptions = it->second;
    completedIntents[label] = std::make_pair( tih.GetCompletedIntent(label), exceptions );
  }

  TestBehaviorFramework testBehaviorFramework;
  testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true);
  
  std::shared_ptr<BehaviorHighLevelAI> behavior;
  const auto& BC = testBehaviorFramework.GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(HighLevelAI),
                                  BEHAVIOR_CLASS(HighLevelAI),
                                  behavior );

  TestBehaviorHighLevelAI testHLAI( behavior );
  
  for( const auto& completedEntry : completedIntents ) {
    
    const auto& label = completedEntry.first;
    const auto& intent = completedEntry.second.first;
    const auto& expectedExceptions = completedEntry.second.second;
    
    auto intentTag = intent.GetTag();
    const UserIntent* intentPtr = (intent.GetJSON().size() == 1) ? nullptr : &intent;
    
    std::vector<std::string> statesNotResponding;
    const bool anyResponding = testHLAI.AreStatesRespondingTo( intentTag, intentPtr, statesNotResponding );
    EXPECT_TRUE( anyResponding ) << "Completed list contains " << label << "(" << UserIntentTagToString(intentTag) << ") but is not handled in HLAI";
    
    // states that arent responding should be listed as exceptions
    for( const auto& actualException : statesNotResponding ) {
      const auto itFound = std::find( expectedExceptions.begin(), expectedExceptions.end(), actualException );
      EXPECT_TRUE( itFound != expectedExceptions.end() )
        << "State " << actualException
        << " does not respond to " << UserIntentTagToString(intentTag)
        << " (" << label << ")";
      // you should add a transition or add this state as an exception in this test (see completedList)
    }
    // and the inverse: exceptions should not have states that are responding
    for( const auto& expectedException : expectedExceptions ) {
      const auto itFound = std::find( statesNotResponding.begin(), statesNotResponding.end(), expectedException );
      EXPECT_TRUE( itFound != statesNotResponding.end() )
        << "State " << expectedException
        << " responds to " << UserIntentTagToString(intentTag)
        << " (" << label << ") but is listed as an exception";
      // you should update this test to remove the exception (see exceptionsList, above)
    }
  }
  
  for( const auto& exceptionEntry : exceptionsList ) {
    const auto& label = exceptionEntry.first;
    const auto it = std::find_if( completedIntents.begin(), completedIntents.end(), [&](const auto& entry) {
      return entry.first == label;
    });
    EXPECT_TRUE( it != completedIntents.end() ) << "In addition to adding exceptions to the above list, add to completedUserIntents.json";
  }
  
}

TEST(BehaviorHighLevelAI, PostBehaviorSuggestionsConsidered)
{
  // tests that when voice commands that offer post-behavior suggestions end, and then HLAI resumes,
  // that the state is set properly. Typically post-behavior suggestions are non-binding, but we want
  // this ones to occur so they are enforced through this test.
  
  // a list of the named completedUserIntents(.json), the behavior that responds to it, and the HLAI state that should follow
  std::vector< std::tuple<std::string, BehaviorID, std::string> > expected = {
    { "fist_bump",       BEHAVIOR_ID(FistBumpVoiceCommand),   "Socializing" },
    { "keep_away",       BEHAVIOR_ID(KeepawayVoiceCommand),   "Socializing" },
    { "roll_cube",       BEHAVIOR_ID(RollCubeVoiceCommand),   "Observing"   },
    { "meet_victor",     BEHAVIOR_ID(MeetVictor),             "Socializing" },
  //{ "imperative_come", BEHAVIOR_ID(ComeHereVoiceCommand),   "Socializing" }, // todo: come here needs a look for face in order to activate
  };
  
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  TestIntentsFramework tih;
  auto& bei = tbf.GetBehaviorExternalInterface();
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  
  auto tick = [&tbf](unsigned int numTimes = 1) {
    AICompMap emptyMap;
    for( unsigned int i=0; i<numTimes; ++i ) {
      tbf.GetBehaviorComponent().UpdateDependent(emptyMap);
      IncrementBaseStationTimerTicks();
    }
  };
  
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec(5.0) );
  
  for( const auto& tup : expected ) {
    
    const auto& intentName = std::get<0>(tup);
    const auto& behaviorID = std::get<1>(tup);
    const auto& nextState  = std::get<2>(tup);
    
    tbf.SetBehaviorStackByName( "highLevelAI_stack" );
    
    // get HLAI running
    tick(3);
    
    // set an intent pending
    auto intent = tih.GetCompletedIntent( intentName );
    auto intentTag = intent.GetTag();
    // meet victor needs a special name to activate and deactivate again
    if( intentTag == USER_INTENT(meet_victor) ) {
      intent.Set_meet_victor( UserIntent_MeetVictor{"Special name for unit tests to end enrollment"} );
    }
    uic.DevSetUserIntentPending( std::move(intent), UserIntentSource::Unknown );
    
    for( int i=0; i<10; ++i ) {
      if( !uic.IsUserIntentPending(intentTag) ) {
        break;
      }
      tick();
    }
    
    // Check the result
    auto stack = tbf.GetCurrentBehaviorStack();
    ASSERT_TRUE( !stack.empty() );
    ICozmoBehavior* behaviorToCancel = nullptr;
    const auto itFound = std::find_if( stack.begin(), stack.end(), [&behaviorID,&behaviorToCancel](auto* behavior) {
      auto* castBeh = dynamic_cast<ICozmoBehavior*>(behavior);
      if( castBeh->GetID() == behaviorID ) {
        behaviorToCancel = castBeh;
        return true;
      } else {
        return false;
      }
    });
    const bool handled = itFound != stack.end();
    EXPECT_TRUE( handled ) << intentName << " not handled";
    
    // now cancel that behavior
    if( behaviorToCancel != nullptr && bei.HasDelegationComponent() ) {
      auto& delegationComponent = bei.GetDelegationComponent();
      delegationComponent.CancelSelf( behaviorToCancel );
    }
    
    // get HLAI running again
    bool hlaiRunning = false;
    for( int i=0; i<10; ++i ) {
      stack = tbf.GetCurrentBehaviorStack();
      const auto itFound = std::find_if( stack.begin(), stack.end(), [](auto* behavior) {
        auto* castBeh = dynamic_cast<ICozmoBehavior*>(behavior);
        return castBeh->GetID() == BEHAVIOR_ID(HighLevelAI);
      });
      if( itFound != stack.end() ) {
        hlaiRunning = true;
        break;
      }
      tick();
    }
    EXPECT_TRUE( hlaiRunning );
    
    // check the state
    std::shared_ptr<BehaviorHighLevelAI> behHLAI;
    const auto& BC = tbf.GetBehaviorContainer();
    BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(HighLevelAI),
                                    BEHAVIOR_CLASS(HighLevelAI),
                                    behHLAI );
    const bool newStateRunning
      = TestBehaviorHighLevelAI::IsStateRunning( behHLAI, nextState );
    EXPECT_TRUE( newStateRunning ) << nextState << " not running after " << intentName << " ended";
  }
  
}

