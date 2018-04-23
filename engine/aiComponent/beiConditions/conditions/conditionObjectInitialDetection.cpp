/**
 * File: ConditionObjectInitialDetection.cpp
 *
 * Author: Matt Michini
 * Created: 1/10/2018
 *
 * Description: Strategy for responding to an object being seen
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionObjectInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace{
  const char* kFirstTimeOnlyKey = "firstTimeOnly";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectInitialDetection::ConditionObjectInitialDetection(const Json::Value& config)
 : ConditionObjectKnown( config, 0 ) // 0 means "this tick"
{
  _firstTimeOnly = JsonTools::ParseBool(config,
                                        kFirstTimeOnlyKey,
                                        "ConditionObjectInitialDetection.ConfigError.FirstTimeOnly");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectInitialDetection::~ConditionObjectInitialDetection()
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectInitialDetection::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive)
{
  _reacted = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectInitialDetection::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if (_firstTimeOnly && _reacted) {
    return false;
  }
  
  const bool baseConditionsMet = ConditionObjectKnown::AreConditionsMetInternal( behaviorExternalInterface );
  if( baseConditionsMet ) {
    _reacted = true;
    return true;
  } else {
    return false;
  }
}


} // namespace Cozmo
} // namespace Anki
