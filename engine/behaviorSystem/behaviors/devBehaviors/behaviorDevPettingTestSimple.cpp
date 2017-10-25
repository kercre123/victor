/**
 * File: BehaviorDevPettingTestSimple.cpp
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engine/behaviorSystem/behaviors/devBehaviors/behaviorDevPettingTestSimple.h"

#include "engine/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "engine/behaviorSystem/wantsToRunStrategies/wantsToRunStrategyFactory.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/jsonTools.h"

#include <vector>
#include <memory>

namespace Anki {
namespace Cozmo {

namespace{
  const std::string kGestureToAnimationKey = "gestureTriggerToAnimations";
  const char*       kAnimationNameKey      = "animationName";
  const char*       kAnimationRateKey      = "animRate_s";
}

  
BehaviorDevPettingTestSimple::BehaviorDevPettingTestSimple(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  const Json::Value& jsonArray = config[kGestureToAnimationKey];
  assert(jsonArray.isArray());
  
  Json::Value::const_iterator it = jsonArray.begin();
  for(;it!=jsonArray.end();++it) {
    auto anim = JsonTools::ParseString(*it, kAnimationNameKey, "Failed to parse animation name");
    auto rate = JsonTools::ParseFloat(*it, kAnimationRateKey, "Failed to parse animation rate");
    
    _tgAnimConfigs.emplace_back(WantsToRunStrategyFactory::CreateWantsToRunStrategy(robot, *it),
                                anim,
                                rate,
                                0.0f);
  }
}

bool BehaviorDevPettingTestSimple::IsRunnableInternal(const Robot& robot) const
{
  return true;
}
  

Result BehaviorDevPettingTestSimple::InitInternal(Robot& robot)
{
  // Disable all reactions
  SmartDisableReactionsWithLock(GetIDStr(), ReactionTriggerHelpers::GetAffectAllArray());
  return RESULT_OK;
}
  
BehaviorStatus BehaviorDevPettingTestSimple::UpdateInternal(Robot& robot)
{
  // placeholder animations for a variety of reactions to touch
  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // find the touch gesture being received
  // note: a touch gesture strategy is True when the touch is recognized as that particular gesture
  decltype(_tgAnimConfigs)::iterator gotAnim;
  for(gotAnim = _tgAnimConfigs.begin(); gotAnim != _tgAnimConfigs.end(); ++gotAnim) {
    ANKI_VERIFY(gotAnim->strategy.get()!=nullptr, "BehaviorDevPettingTestSimple.NullTouchStrategy", "");
    if(gotAnim->strategy->WantsToRun(robot)) {
      break;
    }
  }
  
  if(gotAnim!=_tgAnimConfigs.end() && !IsActing()) {
    std::string animToPlay = gotAnim->animationName;
    float rate             = gotAnim->animationRate_s;
    float timeLastPlayed_s = gotAnim->timeLastPlayed_s;
    if((now-timeLastPlayed_s) > rate) {
      PlayAnimationAction* action = new PlayAnimationAction(robot, animToPlay, 1, true, (u8)AnimTrackFlag::BODY_TRACK);
      StartActing(action);
      gotAnim->timeLastPlayed_s = now;
    }
  }
  
  return BehaviorStatus::Running;
}


void BehaviorDevPettingTestSimple::StopInternal(Robot& robot)
{
}

} // Cozmo
} // Anki
