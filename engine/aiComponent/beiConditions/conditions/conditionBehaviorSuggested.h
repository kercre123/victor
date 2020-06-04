/**
 * File: conditionBehaviorSuggested.h
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

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionBehaviorSuggested_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionBehaviorSuggested_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"

namespace Anki {
namespace Vector {

class ConditionBehaviorSuggested : public IBEICondition
{
public:
  explicit ConditionBehaviorSuggested(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;
  
private:
  PostBehaviorSuggestions _suggestion;
  int _maxTicksForSuggestion;
};

}
}

#endif
