/**
* File: IConditionUserIntent.cpp
*
* Author: ross
* Created: 2018 Feb 13
*
* Description: Condition to check if one or more user intent(s) are pending
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/iConditionUserIntent.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"
#include "util/logging/logging.h"



namespace Anki {
namespace Cozmo {
  
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// list of lambdas used in json
// XXX: these lambdas will only be called if the paired tag you provide (in json or in ctor) matches
// the pending intent, so there's no real need to check the tag

std::unordered_map< std::string, IConditionUserIntent::EvalUserIntentFunc > IConditionUserIntent::sMap =
{
  // PLEASE KEEP ALPHABETIZED
  
  {
    "habitat_play_specific_filter",
    [](const UserIntent& intent)->bool
    {
      auto playSpecific = intent.Get_play_specific();
      auto entityBehavior = playSpecific.entity_behavior;
      if( entityBehavior.compare("cube_spinner")==0 ||
         entityBehavior.compare("pick_up_cube")==0 ||
         entityBehavior.compare("roll_cube")==0) {
        return true;
      }
      return false;
    }
  },
  {
    // test_lambda. Only used in unit tests. keep this at the end
    "test_lambda", [](const UserIntent& intent)
    {
      DEV_ASSERT( intent.GetTag() == UserIntentTag::test_name, "Tag should have already been verified" );
      return (intent.Get_test_name().name == "Victor");
    }
  }
};
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
namespace {
  const char* kLambdaKey = "_lambda";
}
  
const char* const IConditionUserIntent::kList = "list";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IConditionUserIntent::IConditionUserIntent( BEIConditionType type, const Json::Value& config )
  : IBEICondition( config )
  , _type( type )
{
  
  ANKI_VERIFY( (_type == BEIConditionType::UserIntentPending) || (_type == BEIConditionType::UserIntentActive),
               "IConditionUserIntent.Ctor.InvalidBEICondType",
               "Condition type '%s' not supported",
               BEIConditionTypeToString(type) );
  
  // provide a list of ways to compare. if any entry is true, this condition will be true. you may:
  // (1) provide _only_ a UserIntentTag, like { "type": "mytag" }, and _only_ the tag will be compared
  // (2) provide the full definition, like { "type": "mytag", "parameter": "" }, and _all_ fields will be compared
  // (3) provide a lambda, like { "type": "mytag", "_lambda": "my_lambda" } which names a lambda above
  
  _evalList.reserve( config[kList].size() );
  for( const auto& elem : config[kList] ) {
    AddUserIntent( elem );
  }
  
  ANKI_VERIFY( !_evalList.empty(),
               "IConditionUserIntent.Ctor.NoElems",
               "Missing tags or lambdas!" );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IConditionUserIntent::IConditionUserIntent( BEIConditionType type )
  : IBEICondition{ IBEICondition::GenerateBaseConditionConfig( type ) }
  , _type( type )
{
  ANKI_VERIFY( (_type == BEIConditionType::UserIntentPending) || (_type == BEIConditionType::UserIntentActive),
               "IConditionUserIntent.Ctor2.InvalidBEICondType",
               "Condition type '%s' not supported",
               BEIConditionTypeToString(type) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IConditionUserIntent::~IConditionUserIntent()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IConditionUserIntent::AddUserIntent( const Json::Value& json )
{
  UserIntent intent;
  
  if( ANKI_VERIFY( intent.SetFromJSON( json ),
                  "IConditionUserIntent.Ctor.InvalidJson",
                  "Invalid intent in config" ) )
  {
    if( !ANKI_VERIFY( intent.GetTag() != USER_INTENT(INVALID),
                      "IConditionUserIntent.Ctor.InvalidTag",
                      "Invalid tag" ) )
    {
      return;
    }
    
    EvalStruct evals{ intent.GetTag() };
    
    // this is optionally set
    std::string evalFuncStr;
    JsonTools::GetValueOptional( json, kLambdaKey, evalFuncStr );
    if( !evalFuncStr.empty() ) {
      auto& tmp = GetFromName( evalFuncStr );
      if( ANKI_VERIFY( tmp != nullptr,
                       "IConditionUserIntent.Ctor.MissingLambda",
                       "Lambda '%s' is invalid",
                       evalFuncStr.c_str() ) )
      {
        evals.func = &tmp;
      }
      
      // if you provide a lambda, we don't check any other fields. just tag + lambda
      DEV_ASSERT( json.getMemberNames().size() == 2,
                 "Provide only '_lambda' and the 'type'." );
    } else if( json.size() > 1 ) {
      
      // do an inefficient check that the number of supplied params matches the number of
      // struct fields. Otherwise, we don't know if the absence of a param means it
      // should match the default struct field value or comparison shouldn't be performed
      DEV_ASSERT( json.size() == intent.GetJSON().size(), "param mismatch" );
      
      evals.intent.reset( new UserIntent{std::move(intent)} );
      
      
    }
    
    if( ANKI_DEVELOPER_CODE ) {
      // validate that for any one tag, there's only one full UserIntent specified, and only
      // one entry identified only by the tag (no func and no intent). 
      for( const auto& existing : _evalList ) {
        if( existing.tag == evals.tag ) {
          DEV_ASSERT( !((existing.intent != nullptr) && (evals.intent != nullptr)), "two intents per tag");
          DEV_ASSERT( !((existing.func == nullptr) && (evals.func == nullptr)
                        && (existing.intent == nullptr) && (evals.intent == nullptr)), "two tags found" );
        }
      }
    }
    
    _evalList.push_back( std::move(evals) );
    
    _isInitialized = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IConditionUserIntent::AddUserIntent( UserIntentTag tag )
{
  _evalList.emplace_back( tag );
  _isInitialized = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IConditionUserIntent::AddUserIntent( UserIntentTag tag, EvalUserIntentFunc&& func )
{
  const bool validFunc = ANKI_VERIFY( func != nullptr,
                                      "IConditionUserIntent.AddUserIntent.NullFunc",
                                      "The function provided for '%s' was null. Use an overloaded method",
                                      UserIntentTagToString( tag ) );
  const bool notInvalid = ANKI_VERIFY( tag != USER_INTENT(INVALID),
                                       "IConditionUserIntent.AddUserIntent.InvalidTag",
                                       "The tag is INVALID" );
  if( notInvalid && validFunc ) {
    _ownedFuncs.push_back( std::move(func) );
    const EvalUserIntentFunc* funcPtr = &_ownedFuncs.back();
    
    _evalList.emplace_back( tag );
    _evalList.back().func = funcPtr;
    
    _isInitialized = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IConditionUserIntent::AddUserIntent( UserIntent&& intent )
{
  auto tag = intent.GetTag();
  const bool notInvalid = ANKI_VERIFY( tag != USER_INTENT(INVALID),
                                       "IConditionUserIntent.AddUserIntent.InvalidTag2",
                                       "The tag is INVALID" );
  if( notInvalid ) {
    _evalList.emplace_back( tag );
    _evalList.back().intent.reset( new UserIntent{ std::move(intent) } );
    _isInitialized = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IConditionUserIntent::AreConditionsMetInternal( BehaviorExternalInterface& behaviorExternalInterface ) const
{
  if( !ANKI_VERIFY( _isInitialized,
                    "IConditionUserIntent.AreConditionsMetInternal.Uninitialized",
                    "Condition is being checked before initialization" ) )
  {
    return false;
  }
  
  const auto& uic = behaviorExternalInterface.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  
  const bool checkPending = (_type == BEIConditionType::UserIntentPending);
  if( checkPending ) {
    if( !uic.IsAnyUserIntentPending() ) {
      return false;
    }
  } else {
    if( uic.GetActiveUserIntent() == nullptr ) {
      return false;
    }
  }
  
  for( const auto& elem : _evalList ) {
    UserIntent pendingIntent;
    UserIntentPtr activeIntent;
    bool gotIntent = false;
    if( checkPending ) {
      gotIntent = uic.IsUserIntentPending( elem.tag, pendingIntent );
    } else {
      activeIntent = uic.GetUserIntentIfActive( elem.tag );
      gotIntent = (activeIntent != nullptr);
    }
    if( gotIntent ) {
      const auto& intent = checkPending ? pendingIntent : activeIntent->intent;
      if( elem.func != nullptr ) {
        // user provided a lambda that wants to check the intent data.
        // todo: enforce that the type of argument to the lambda matches the tag in the pair, i.e.,
        // lambda takes typename T as "const T& intent" (like T==UserIntent_MyType) with a clad
        // TagToType check against the param. Should be possible in the constructor versions, not json
        if( (*elem.func)( intent ) ) {
          _selectedTag = elem.tag;
          return true;
        }
      } else if( elem.intent != nullptr ) {
        if( *elem.intent == intent ) {
          _selectedTag = elem.tag;
          return true;
        }
      } else {
        _selectedTag = elem.tag;
        return true;
      }
    }
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IConditionUserIntent::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive)
{
  _selectedTag = USER_INTENT(INVALID);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UserIntentTag IConditionUserIntent::GetUserIntentTagSelected() const
{
  if( _selectedTag == USER_INTENT(INVALID) ) {
    PRINT_NAMED_WARNING("IConditionUserIntent.GetUserIntentTagSelected.Invalid",
                        "_selectedTag was never set" ); // this can happen in tests
  }
  return _selectedTag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IConditionUserIntent::EvalUserIntentFunc& IConditionUserIntent::GetFromName( const std::string& name ) const
{
  const auto it = sMap.find( name );
  if( ANKI_VERIFY( it != sMap.end(),
                   "IConditionUserIntent.GetFromName.MissingLambda",
                   "Could not find lambda '%s'",
                   name.c_str() ) )
  {
    return it->second;
  }
  static EvalUserIntentFunc nullfunc;
  return nullfunc;
}
  
void IConditionUserIntent::BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const
{
  for( const auto& elem : _evalList ) {
    factors.AddFactor( "tag", UserIntentTagToString(elem.tag) );
    if( elem.func != nullptr ) {
      factors.AddFactor( "lambda", true );
    } else if( elem.intent != nullptr ) {
      std::stringstream ss;
      ss << elem.intent->GetJSON();
      factors.AddFactor( "intent", ss.str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IConditionUserIntent::EvalStruct::EvalStruct(UserIntentTag it)
  : tag(it)
{
  
}

}
}
