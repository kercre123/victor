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
#include "engine/behaviorSystem/activities/activityStrategies/activityStrategyFPPlayWithHumans.h"

#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
  
  namespace {
  static const char* kActivityCooldownRejectionBaseConfigKey = "cooldownRejectionBaseSecs";
  static const char* kActivityCooldownRejectionExponentConfigKey = "cooldownRejectionExponent";
  static const char* debugName = "ActivityStrategyFPPlayWithHumans";
  };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyFPPlayWithHumans::ActivityStrategyFPPlayWithHumans(Robot& robot, const Json::Value& config)
: IActivityStrategy(robot, config)
, _lastGameRequestTimestampSec(0.0f)
, _cooldownRejectionBaseSecs(0.0f)
, _cooldownRejectionExponent(1.0f)
, _numRejections(0)
{
  

  // register to receive notifications of game requests
  if ( robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _eventHandles);
    helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RequestGameStart>();
    helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::DenyGameStart>();
  }
  
  _cooldownRejectionBaseSecs = JsonTools::ParseFloat(config, kActivityCooldownRejectionBaseConfigKey, debugName);
  _cooldownRejectionExponent = JsonTools::ParseFloat(config, kActivityCooldownRejectionExponentConfigKey, debugName);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyFPPlayWithHumans::WantsToStartInternal(const Robot& robot, float lastTimeActivityRanSec) const
{
  // TODO if it requires a face, check that we have one registered
  // Check which behaviors need face, how the do it (specially IBehaviorRequestGame::HasFace), and make the proper
  // checks here for whether we should start
  
  // In any case, at least having games available is a requirement
  UnlockId nextRequest = robot.GetAIComponent().GetNonConstRequestGameComponent().IdentifyNextGameTypeToRequest(robot);
  return nextRequest != UnlockId::Count;
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

  UnlockId nextRequest = robot.GetAIComponent().GetNonConstRequestGameComponent().IdentifyNextGameTypeToRequest(robot);
  return nextRequest == UnlockId::Count;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivityStrategyFPPlayWithHumans::HandleMessage(const ExternalInterface::RequestGameStart& msg)
{
  // set timestamp
  _lastGameRequestTimestampSec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

template<>
void ActivityStrategyFPPlayWithHumans::HandleMessage(const ExternalInterface::DenyGameStart& msg)
{
  _numRejections++;
  const float cooldown = _cooldownRejectionBaseSecs * pow(_numRejections, _cooldownRejectionExponent);
  SetCooldown(cooldown);
}

} // namespace
} // namespace
