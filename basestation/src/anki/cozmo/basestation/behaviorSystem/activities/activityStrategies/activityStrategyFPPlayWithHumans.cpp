/**
 * File: ActivityStrategyFPPlayWithHumans.cpp
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay PlayWithHumans activity.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyFPPlayWithHumans.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyFPPlayWithHumans::ActivityStrategyFPPlayWithHumans(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IActivityStrategy(config)
, _lastGameRequestTimestampSec(0.0f)
{

  // register to receive notifications of game requests
  if ( robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _eventHandles);
    helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RequestGameStart>();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyFPPlayWithHumans::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  // TODO if it requires a face, check that we have one registered
  // Check which behaviors need face, how the do it (specially IBehaviorRequestGame::HasFace), and make the proper
  // checks here for whether we should start
  
  // In any case, at least having games available is a requirement
  const bool isAnyGameAvailable = robot.GetBehaviorManager().IsAnyGameFlagAvailable( BehaviorGameFlag::All );
  if ( !isAnyGameAvailable ) {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyFPPlayWithHumans::WantsToEndInternal(const Robot& robot, float lastTimeActivityStartedSec) const
{
  // check if we have requested a game since we started. Note that WantsToEndInternal can only be called for the
  // running activity strategy, so it's safe to check last time we started, since we are the running one
  const bool hasRequestedGame = FLT_GT(_lastGameRequestTimestampSec, lastTimeActivityStartedSec);
  if ( hasRequestedGame ) {
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivityStrategyFPPlayWithHumans::HandleMessage(const ExternalInterface::RequestGameStart& msg)
{
  // set timestamp
  _lastGameRequestTimestampSec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

} // namespace
} // namespace
