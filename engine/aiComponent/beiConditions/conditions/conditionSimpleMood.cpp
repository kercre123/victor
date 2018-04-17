/**
* File: conditionSimpleMood.h
*
* Author:  ross
* Created: apr 11 2018
*
* Description: True if the SimpleMood takes on a given value, or if it transitions from one value
*              to another this tick, based on config
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionSimpleMood.h"

#include "clad/types/simpleMoodTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kMoodKey   = "mood";
static const char* kFromKey   = "from";
static const char* kToKey     = "to";
static const char* kDebugName = "ConditionSimpleMood";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionSimpleMood::ConditionSimpleMood(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
  if( !config[kFromKey].isNull() || !config[kToKey].isNull() ) {
    _useTransition = true;
    
    DEV_ASSERT( config[kMoodKey].isNull(), "Can't specify both mood and from/to" );
    
    _from = SimpleMoodType::Count;
    _to = SimpleMoodType::Count;
    
    std::string fromStr = config.get( kFromKey, "" ).asString();
    std::string toStr = config.get( kToKey, "" ).asString();
    
    if( !fromStr.empty() ) {
      ANKI_VERIFY( SimpleMoodTypeFromString( fromStr, _from ),
                   "ConditionSimpleMood.Ctor.InvalidFrom",
                   "Unknown mood type %s",
                   fromStr.c_str() );
    }
    if( !toStr.empty() ) {
      ANKI_VERIFY( SimpleMoodTypeFromString( toStr, _to ),
                   "ConditionSimpleMood.Ctor.InvalidTo",
                   "Unknown mood type %s",
                   toStr.c_str() );
    }
  } else {
    _useTransition = false;
    std::string moodStr = JsonTools::ParseString( config, kMoodKey, kDebugName );
    ANKI_VERIFY( SimpleMoodTypeFromString( moodStr, _mood ),
                 "ConditionSimpleMood.Ctor.InvalidMood",
                 "Unknown mood type %s",
                 moodStr.c_str() );
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionSimpleMood::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  bool ret = false;
  if( behaviorExternalInterface.HasMoodManager() ) {
    auto& moodManager = behaviorExternalInterface.GetMoodManager();
    SimpleMoodType currMood = moodManager.GetSimpleMood();
    
    if( !_useTransition ) {
      ret = (currMood == _mood);
    } else {
      if( (_from == SimpleMoodType::Count) && (_to != SimpleMoodType::Count) ) {
        // specified a "to" but not "from"
        ret = (currMood == _to) && moodManager.DidSimpleMoodTransitionThisTick();
      } else if( (_from != SimpleMoodType::Count) && (_to == SimpleMoodType::Count) ) {
        // specified a "from" but not "to"
        ret = moodManager.DidSimpleMoodTransitionThisTickFrom( _from );
      } else if( (_from != SimpleMoodType::Count) && (_to != SimpleMoodType::Count) ) {
        // specified "from" and "to". (short circuit here when possible)
        ret = (currMood == _to) && moodManager.DidSimpleMoodTransitionThisTick( _from, _to );
      } else {
        DEV_ASSERT( false && "from and to both null", "Should not be possible given json loading" );
      }
    }
  }
  return ret;
}

} // namespace Cozmo
} // namespace Anki
