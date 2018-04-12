/**
* File: conditionEmotion.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Condition to compare against the value of an emotion
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionEmotion.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kMaxKey = "max";
static const char* kMinKey = "min";
static const char* kValueKey = "value";
static const char* kEmotionKey = "emotion";
static const char* kDebugKey = "ConditionEmotion";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionEmotion::ConditionEmotion(const Json::Value& config)
  : IBEICondition(config)
{
  LoadJson(config);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionEmotion::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  float value = 0.0f;
  if(behaviorExternalInterface.HasMoodManager()){
    auto& moodManager = behaviorExternalInterface.GetMoodManager();
    value = moodManager.GetEmotionValue( _emotion );
  } else {
    return false;
  }
  
  bool areConditionsMet;
  if( _useRange ) {
    areConditionsMet = (value >= _minScore) && (value <= _maxScore);
  } else {
    areConditionsMet = FLT_NEAR( value, _exactScore );
  }
  return areConditionsMet;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionEmotion::LoadJson(const Json::Value& config)
{
  if( !config[kMaxKey].isNull() || !config[kMinKey].isNull() ) {
    DEV_ASSERT( config[kValueKey].isNull(), "Can't specify both value and min/max" );
    _minScore = config.get( kMinKey, std::numeric_limits<float>::lowest() ).asFloat();
    _maxScore = config.get( kMaxKey, std::numeric_limits<float>::max() ).asFloat();
    _useRange = true;
  } else {
    _exactScore = JsonTools::ParseFloat( config, kValueKey, kDebugKey );
    _useRange = false;
  }
  std::string emotionStr = JsonTools::ParseString( config, kEmotionKey, kDebugKey );
  ANKI_VERIFY( EmotionTypeFromString( emotionStr, _emotion ),
               "ConditionEmotion.Ctor.Invalid",
               "Unknown emotion type %s",
               emotionStr.c_str() );
}

} // namespace Cozmo
} // namespace Anki
