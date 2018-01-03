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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevPettingTestSimple.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/stateConceptStrategyFactory.h"
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevPettingTestSimple::BehaviorDevPettingTestSimple(const Json::Value& config)
: ICozmoBehavior(config)
{
  _configArray = config[kGestureToAnimationKey];
  assert(_configArray.isArray());
  

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevPettingTestSimple::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  Json::Value::const_iterator it = _configArray.begin();
  for(;it!=_configArray.end();++it) {
    auto anim = JsonTools::ParseString(*it, kAnimationNameKey, "Failed to parse animation name");
    auto rate = JsonTools::ParseFloat(*it, kAnimationRateKey, "Failed to parse animation rate");
    
    _tgAnimConfigs.emplace_back(StateConceptStrategyFactory::CreateStateConceptStrategy(
                        behaviorExternalInterface, 
                        robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                        *it),
                                anim,
                                rate,
                                0.0f);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDevPettingTestSimple::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus BehaviorDevPettingTestSimple::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  // placeholder animations for a variety of reactions to touch
  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // find the touch gesture being received
  // note: a touch gesture strategy is True when the touch is recognized as that particular gesture
  decltype(_tgAnimConfigs)::iterator gotAnim;
  for(gotAnim = _tgAnimConfigs.begin(); gotAnim != _tgAnimConfigs.end(); ++gotAnim) {
    ANKI_VERIFY(gotAnim->strategy.get()!=nullptr, "BehaviorDevPettingTestSimple.NullTouchStrategy", "");
    if(gotAnim->strategy->AreStateConditionsMet(behaviorExternalInterface)) {
      break;
    }
  }
  
  if(gotAnim!=_tgAnimConfigs.end() && !IsActing()) {
    std::string animToPlay = gotAnim->animationName;
    float rate             = gotAnim->animationRate_s;
    float timeLastPlayed_s = gotAnim->timeLastPlayed_s;
    if((now-timeLastPlayed_s) > rate) {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      PlayAnimationAction* action = new PlayAnimationAction(robot, animToPlay, 1, true, (u8)AnimTrackFlag::BODY_TRACK);
      DelegateIfInControl(action);
      gotAnim->timeLastPlayed_s = now;
    }
  }
  
  return BehaviorStatus::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevPettingTestSimple::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // Cozmo
} // Anki
