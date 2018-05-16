/**
 * File: conditionBeatDetected
 *
 * Author: Matt Michini
 * Created: 05/07/2018
 *
 * Description: Determine whether or not the beat detector algorithm running in the
 *              anim process is currently detecting a steady musical beat.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionBeatDetected.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/mics/beatDetectorComponent.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionBeatDetected::ConditionBeatDetected(const Json::Value& config)
: IBEICondition(config)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionBeatDetected::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  return bei.GetBeatDetectorComponent().IsBeatDetected();
}

}
}

