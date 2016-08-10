/**
 * File: AIGoalStrategyFPSocialize
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay Socialize goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalStrategyFPSocialize.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalStrategyFPSocialize::AIGoalStrategyFPSocialize(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IAIGoalStrategy(config)
, _maxTimeWithoutInteractionSecs(-1.0f)
, _lastInteractionTimeStamp(0.0f)
{
  using namespace JsonTools;
  _maxTimeWithoutInteractionSecs = ParseFloat(config, "maxTimeWithoutInteractionSecs", "AIGoalStrategyFPSocialize");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalStrategyFPSocialize::WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const
{
  // check the timeout for interactions if it was set
  const bool hasInteractionTimeout = FLT_GT(_maxTimeWithoutInteractionSecs, 0.0f);
  if ( hasInteractionTimeout ) {
    // it's set, check if it's been too long since last interaction, so that we go do something else
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeElapsedSinceLastInteraction = curTimeSecs - _lastInteractionTimeStamp;
    const bool hasBeenLongWithoutInteraction = FLT_GT(timeElapsedSinceLastInteraction, _maxTimeWithoutInteractionSecs);
    if ( hasBeenLongWithoutInteraction ) {
      return true;
    }
  }

  // doesn't wanna end
  return false;
}

// TODO RSAM
// call RegisterInteraction from handlers like PickUp, SeeFace, CubeMoved, etc (identify what interactions are)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalStrategyFPSocialize::RegisterInteraction()
{
  // just set the timestamp
  const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastInteractionTimeStamp = curTimeSecs;
}

} // namespace
} // namespace
