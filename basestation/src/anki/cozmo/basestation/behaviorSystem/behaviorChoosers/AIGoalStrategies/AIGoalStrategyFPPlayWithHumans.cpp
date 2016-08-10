/**
 * File: AIGoalStrategyFPPlayWithHumans
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay PlayWithHumans goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalStrategyFPPlayWithHumans.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalStrategyFPPlayWithHumans::AIGoalStrategyFPPlayWithHumans(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IAIGoalStrategy(config)
, _lastGameRequestTimestampSec(0.0f)
{

  // register to receive notifications of game requests
  if ( robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _eventHandles);
    helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RequestGameStart>();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalStrategyFPPlayWithHumans::WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const
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
bool AIGoalStrategyFPPlayWithHumans::WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const
{
  // check if we have requested a game since we started. Note that WantsToEndInternal can only be called for the
  // running goal strategy, so it's safe to check last time we started, since we are the running one
  const bool hasRequestedGame = FLT_GT(_lastGameRequestTimestampSec, lastTimeGoalStartedSec);
  if ( hasRequestedGame ) {
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIGoalStrategyFPPlayWithHumans::HandleMessage(const ExternalInterface::RequestGameStart& msg)
{
  // set timestamp
  _lastGameRequestTimestampSec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

} // namespace
} // namespace
