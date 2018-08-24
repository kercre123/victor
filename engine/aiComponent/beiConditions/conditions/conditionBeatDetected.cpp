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
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionBeatDetected::ConditionBeatDetected(const Json::Value& config)
: IBEICondition(config)
{
  JsonTools::GetValueOptional(config, "allowPotentialBeat", _allowPotentialBeat);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionBeatDetected::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const auto& beatDetector = bei.GetBeatDetectorComponent();
  return beatDetector.IsBeatDetected() || (_allowPotentialBeat && beatDetector.IsPossibleBeatDetected());
}

}
}

