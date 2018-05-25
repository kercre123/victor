/**
* File: IConditionUserIntent.h
*
* Author: ross
* Created: 2018 Feb 13
*
* Description: Base condition to check if one or more user intent(s) are pending or active
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IConditionUserIntent_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IConditionUserIntent_H__

#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "json/json-forwards.h"

#include <vector>
#include <list>

namespace Anki {
namespace Cozmo {

class IConditionUserIntent : public IBEICondition
{
public:
  
  virtual ~IConditionUserIntent();
  
  // the condition will be true if the tag matches
  void AddUserIntent( UserIntentTag tag );
  
  // the condition will be true if the lambda evaluates to true. Note that the lambda will only
  // be called if the tag matches, so your func need not check the tag. i.e., only worry about the params
  using EvalUserIntentFunc = std::function<bool(const UserIntent&)>;
  void AddUserIntent( UserIntentTag tag, EvalUserIntentFunc&& func );
  
  // the condition will be true if the pending intent fully matches the provided intent
  void AddUserIntent( UserIntent&& intent );
  
  // this works the same as how the constructor handles elements in its config's "list"
  void AddUserIntent( const Json::Value& json );
  
  // if this condition is true, then this condition should be able to return the tag that made it
  // true out of the list passed during construction
  UserIntentTag GetUserIntentTagSelected() const;

protected:
  
  static const char* const kList;
  
  // ctor for use with json and factory. provide a list of ways to compare to an intent. if any
  // are true, this condition will be true. See method for details
  explicit IConditionUserIntent( BEIConditionType type, const Json::Value& config );
  
  // special constructor for which you MUST immediately use one of the Add* methods below, or risk an assert
  explicit IConditionUserIntent( BEIConditionType type );
  
  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& behaviorExternalInterface ) const override;
  
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool isActive) override;
  
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
private:
  
  friend class TestBehaviorHighLevelAI; // unit test access
  
  // accesses the eval func from sMap by name, or returns nullptr if not found
  const EvalUserIntentFunc& GetFromName( const std::string& name ) const;
  
  bool _isInitialized = false;
  
  struct EvalStruct
  {
    explicit EvalStruct(UserIntentTag it);
    UserIntentTag tag; // always compared, and must be set
    const EvalUserIntentFunc* func = nullptr; // if set, will be evaluated with the pending intent
    std::unique_ptr<UserIntent> intent; // if set, tests for equality in all fields with the pending intent
  };
  
  // list of comparisons to be performed when checking a pending intent
  std::vector< EvalStruct > _evalList;
  
  // this holds eval funcs that were arguments in the AddUserIntent that accepts lambdas
  std::list< EvalUserIntentFunc > _ownedFuncs;
  
  // if any in _evalList matches, the tag is assigned here, so we know which out of _evalList was true
  mutable UserIntentTag _selectedTag;
  
  // list of named lambdas that evaluate UserIntent for its data (not tag)
  static std::unordered_map< std::string, EvalUserIntentFunc > sMap;
  
  BEIConditionType _type;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_IConditionUserIntent_H__
