/**
 * File: behaviorOnConfigSeen.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/2/16
 *
 * Description: Scored behavior which plays a specified animation when it sees
 * a new block configuration
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/behaviorSystem/behaviors/freeplay/behaviorOnConfigSeen.h"

#include "engine/robot.h"
#include "engine/actions/animActions.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/blockWorld/blockConfigTypeHelpers.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockWorld.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
using namespace ExternalInterface;
const constexpr char* const kConfigTriggersKey = "configTriggers";
const constexpr char* const kAnimTriggersKey = "animTriggers";
const constexpr float kMaxIntervalBetweenRunnableCheck_s = 5.f;
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnConfigSeen::BehaviorOnConfigSeen(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _animTriggerIndex(0)
, _lastRunnableCheck_s(0)
, _lastTimeNewConfigSeen_s(0)
{
  ReadJson(config);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnConfigSeen::ReadJson(const Json::Value& config)
{
  const Json::Value& configTriggers = config[kConfigTriggersKey];
  if(!configTriggers.isNull()){
    for(const auto& trigger : configTriggers){
      auto configurationType = BlockConfigurations::BlockConfigurationFromString(trigger.asCString());
      _configurationCountMap[configurationType] = 0;
    }
  }
  
  const Json::Value& animTriggers = config[kAnimTriggersKey];
  if(!animTriggers.isNull()){
    for(const auto& trigger : animTriggers){
      _animTriggers.emplace_back(AnimationTriggerFromString(trigger.asCString()));
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnConfigSeen::IsRunnableInternal(const Robot& robot) const
{
  // if this is the first update in a long time consider that the config was already known
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool initialUpdateCheck = FLT_GE(currentTime - _lastRunnableCheck_s, kMaxIntervalBetweenRunnableCheck_s);
  _lastRunnableCheck_s = currentTime;
  
  for(auto& mapEntry: _configurationCountMap){
    const auto& configs = robot.GetBlockWorld().
                           GetBlockConfigurationManager().GetCacheByType(mapEntry.first);
    if(configs.ConfigurationCount() > mapEntry.second){
      _lastTimeNewConfigSeen_s = currentTime;
    }
    mapEntry.second = configs.ConfigurationCount();
  }
  
  return FLT_NEAR(_lastTimeNewConfigSeen_s, currentTime) && !initialUpdateCheck;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorOnConfigSeen::InitInternal(Robot& robot)
{
  _animTriggerIndex = 0;
  TransitionToPlayAnimationSequence(robot);
  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnConfigSeen::TransitionToPlayAnimationSequence(Robot& robot)
{
  if(_animTriggerIndex < _animTriggers.size()){
    const AnimationTrigger animTrigger = _animTriggers[0];
    StartActing(new TriggerLiftSafeAnimationAction(robot, animTrigger));
  }
}

} // namespace Cozmo
} // namespace Anki
