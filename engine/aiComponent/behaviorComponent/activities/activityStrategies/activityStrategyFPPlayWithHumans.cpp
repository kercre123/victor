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
#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategyFPPlayWithHumans.h"

#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
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
ActivityStrategyFPPlayWithHumans::ActivityStrategyFPPlayWithHumans(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, config)
, _lastGameRequestTimestampSec(0.0f)
, _cooldownRejectionBaseSecs(0.0f)
, _cooldownRejectionExponent(1.0f)
, _numRejections(0)
, _whiteboardRef(behaviorExternalInterface.GetAIComponent().GetWhiteboard())
{
  
  auto robotExternalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  // register to receive notifications of game requests
  if (robotExternalInterface != nullptr) {
    auto helper = MakeAnkiEventUtil(*robotExternalInterface, *this, _eventHandles);
    helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::RequestGameStart>();
    helper.SubscribeGameToEngine<ExternalInterface::MessageGameToEngineTag::DenyGameStart>();
  }
  
  _cooldownRejectionBaseSecs = JsonTools::ParseFloat(config, kActivityCooldownRejectionBaseConfigKey, debugName);
  _cooldownRejectionExponent = JsonTools::ParseFloat(config, kActivityCooldownRejectionExponentConfigKey, debugName);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyFPPlayWithHumans::WantsToStartInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityRanSec) const
{
  // TODO if it requires a face, check that we have one registered
  // Check which behaviors need face, how the do it (specially IBehaviorRequestGame::HasFace), and make the proper
  // checks here for whether we should start
  
  // In any case, at least having games available is a requirement
  UnlockId nextRequest = behaviorExternalInterface.GetAIComponent().GetNonConstRequestGameComponent().IdentifyNextGameTypeToRequest(behaviorExternalInterface);
  return nextRequest != UnlockId::Count;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyFPPlayWithHumans::WantsToEndInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityStartedSec) const
{
  // check if we have requested a game since we started. Note that WantsToEndInternal can only be called for the
  // running activity strategy, so it's safe to check last time we started, since we are the running one
  const bool hasRequestedGame = FLT_GT(_lastGameRequestTimestampSec, lastTimeActivityStartedSec);
  if ( hasRequestedGame ) {
    return true;
  }

  UnlockId nextRequest = behaviorExternalInterface.GetAIComponent().GetNonConstRequestGameComponent().IdentifyNextGameTypeToRequest(behaviorExternalInterface);
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
  if(!_whiteboardRef.IsCurrentGameRequestUIRequest()){
    _numRejections++;
    const float cooldown = _cooldownRejectionBaseSecs * pow(_numRejections, _cooldownRejectionExponent);
    SetCooldown(cooldown);
  }
}

} // namespace
} // namespace
