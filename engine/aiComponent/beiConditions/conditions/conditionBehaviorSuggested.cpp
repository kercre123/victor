/**
 * File: conditionBehaviorSuggested.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-04-19
 *
 * Description: True if the post-behavior suggestion specified in the config has been offered to the
 * AI Whiteboard within a specified number of engine ticks.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionBehaviorSuggested.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {
  
namespace {
  // Optional configuration keys
  const char* kPostBehaviorSuggestionNameKey = "postBehaviorSuggestion";
  const char* kMaxTicksForSuggestionKey      = "maxTicksForSuggestion";
}

ConditionBehaviorSuggested::ConditionBehaviorSuggested(const Json::Value& config)
  : IBEICondition(config)
{
  const std::string& missingKeyError = GetDebugLabel() + ".LoadConfig.MissingKey";
  const std::string suggestionName = JsonTools::ParseString(config, kPostBehaviorSuggestionNameKey,
                                                            missingKeyError.c_str());
  ANKI_VERIFY(PostBehaviorSuggestionsFromString(suggestionName, _suggestion),
              (GetDebugLabel() + ".LoadConfig.InvalidPostBehaviorSuggestionName").c_str(),
              "%s is not a valid PostBehaviorSuggestions value", suggestionName.c_str());
  
  _maxTicksForSuggestion = JsonTools::ParseInt32(config, kMaxTicksForSuggestionKey,
                                                 missingKeyError.c_str());
}

bool ConditionBehaviorSuggested::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  auto& whiteboard = bei.GetAIComponent().GetComponent<AIWhiteboard>();
  size_t tickSuggested = 0;
  if( whiteboard.GetPostBehaviorSuggestion( _suggestion, tickSuggested) ) {
    const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
    if( tickSuggested + _maxTicksForSuggestion >= currTick ) {
      return true;
    }
  }
  return false;
}

}
}
