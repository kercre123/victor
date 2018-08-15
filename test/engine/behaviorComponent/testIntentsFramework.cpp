/**
* File: testIntentsFramework.cpp
*
* Author: ross
* Created: Mar 20 2018
*
* Description: Framework to help with testing user/cloud/app intents that we consider "done"
*
* Copyright: Anki, Inc. 2018
*
**/

#include "test/engine/behaviorComponent/testIntentsFramework.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/cozmoContext.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "util/logging/logging.h"


namespace Anki{
namespace Vector{

namespace{
  // Used to assert that intents are handled quickly enough. Should match the value set in userIntentComponent.cpp 
  static const size_t kMaxTicksToClear = 3;
}
  
std::unordered_map<std::string, TestIntentsFramework::IntentInfo> TestIntentsFramework::_completedUserIntents;
  
TestIntentsFramework::TestIntentsFramework()
{
  // only load once
  if( _completedUserIntents.empty() ) {
    LoadCompletedIntents();
  }
}

bool TestIntentsFramework::TestUserIntentTransition( TestBehaviorFramework& tbf,
                                                     const std::vector<IBehavior*>& initialStack,
                                                     UserIntent intentToSend,
                                                     BehaviorID expectedIntentHandlerID,
                                                     bool onlyCheckInStack )
{
  tbf.ReplaceBehaviorStack(initialStack);
  
  // Send user intent
  auto& uic = tbf.GetBehaviorComponent().GetComponent<UserIntentComponent>();
  UserIntentTag intentTag = intentToSend.GetTag();
  uic.DevSetUserIntentPending(std::move(intentToSend), UserIntentSource::Unknown);
  // TODO:(bn) add support for intent sources

  // Tick the behavior component until the intent is consumed, so that the behavior can respond to the intent
  AICompMap emptyMap;
  uint8_t tics = 0;
  while(uic.IsUserIntentPending(intentTag)){
    tbf.GetBehaviorComponent().UpdateDependent(emptyMap);
    IncrementBaseStationTimerTicks();
    ASSERT_NAMED_EVENT(++tics < kMaxTicksToClear,
                       "TestIntentsFramework.IntentNotConsumed",
                       "Intent '%s' is still pending after the tick limit",
                       UserIntentTagToString(intentTag));
  }
  // Check the result
  const auto& stack = tbf.GetCurrentBehaviorStack();
  if( !stack.empty() ) {
    if( onlyCheckInStack ) {
      for( const auto* stackElem : stack ) {
        const auto* castTopOfStack = dynamic_cast<const ICozmoBehavior*>(stackElem);
        if( castTopOfStack->GetID() == expectedIntentHandlerID ) {
          return true;
        }
      }
      return false;
    } else {
      const IBehavior* topOfStack = stack.back();
      const auto* castTopOfStack = dynamic_cast<const ICozmoBehavior*>(topOfStack);

      return (castTopOfStack->GetID() == expectedIntentHandlerID);
    }
  } else {
    return false;
  }
}

bool TestIntentsFramework::TryParseCloudIntent( TestBehaviorFramework& tbf,
                                                const std::string& cloudIntent,
                                                UserIntent* intent,
                                                bool requireMatch )
{
  auto& uic = tbf.GetBehaviorComponent().GetComponent<UserIntentComponent>();
  
  const bool parsed = uic.SetCloudIntentPendingFromJSON( cloudIntent );
  const bool pending = uic.IsAnyUserIntentPending();
  const bool unmatched = uic.IsUserIntentPending( USER_INTENT(unmatched_intent) );
  
  if( intent != nullptr ) {
    // find out what intent it is (complicated bc we purposefully don't expose a GetIntent() without knowing the intent)
    using Type = std::underlying_type<UserIntentTag>::type;
    for( Type idx = 0; idx != static_cast<Type>(UserIntentTag::test_SEPARATOR); ++idx ) {
      if( uic.IsUserIntentPending( static_cast<UserIntentTag>(idx), *intent ) ) {
        break;
      }
    }
  }
  
  return parsed && pending && (!requireMatch || !unmatched);
}
  
const std::string& TestIntentsFramework::GetLabelForIntent( const UserIntent& intent ) const
{
  const Json::Value& intentJson = intent.GetJSON();
  // an intent maps to a label if all json keys in _completedUserIntents are found in intentJson
  for( const auto& entry : _completedUserIntents ) {
    bool match = !entry.second.def.empty();
    for( const auto& key : entry.second.def.getMemberNames() ) {
      const auto& intentValue = intentJson[key];
      if( intentValue.isNull() || (intentValue.asString() != entry.second.def[key].asString()) ) {
        match = false;
        break;
      }
    }
    if( match ) {
      return entry.first;
    }
  }
  static std::string sEmpty;
  return sEmpty;
}
  
std::set<std::string> TestIntentsFramework::GetCompletedLabels( EHandledLocation location ) const
{
  std::set<std::string> ret;
  for( const auto& entry : _completedUserIntents ) {
    if( (location == EHandledLocation::Anywhere) || (entry.second.location == location) ) {
      ret.insert( entry.first );
    }
  }
  return ret;
}
  
UserIntent TestIntentsFramework::GetCompletedIntent( const std::string& label ) const
{
  UserIntent intent;
  const auto it = _completedUserIntents.find( label );
  if( it != _completedUserIntents.end() ) {
    // turn json into an intent
    const bool parsed = intent.SetFromJSON( it->second.def );
    ASSERT_NAMED( parsed, ("Could not parse " + label).c_str() );
  } else {
    ASSERT_NAMED( false, ("Could not find label " + label).c_str() );
  }
  return intent;
}
  
void TestIntentsFramework::LoadCompletedIntents()
{
  ASSERT_NAMED( _completedUserIntents.empty(), "Only call this once" );
  
  Json::Value json;
  const std::string jsonFilename = "test/aiTests/completedUserIntents.json";
  const bool success = cozmoContext->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources,
                                                                   jsonFilename,
                                                                   json);
  ASSERT_NAMED( success, ("Could not find " + jsonFilename).c_str() );
  
  auto saveToMap = [&](const std::string& key, EHandledLocation loc) {
    const auto& blob = json[key];
    ASSERT_NAMED( !blob.isNull(), ("Could not find key " + key).c_str() );
    
    for( const auto& label : blob.getMemberNames() ) {
      const auto it = _completedUserIntents.find( label );
      ASSERT_NAMED( it == _completedUserIntents.end(), ("Label already exists: " + label).c_str());
      
      _completedUserIntents[label].def = blob[label];
      _completedUserIntents[label].location = loc;
    }
  };
  
  saveToMap( "handledByHighLevelAI", EHandledLocation::HighLevelAI );
  saveToMap( "handledElsewhere", EHandledLocation::Elsewhere );
  
}
  
}
}
