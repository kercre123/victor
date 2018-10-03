/**
 * File: BehaviorReactToPutDown.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2018-09-26
 *
 * Description: Behavior for reacting when a user places the robot down on a flat surface in an upright position
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPutDown.h"

#include "anki/cozmo/robot/logging.h"
#include "clad/types/animationTrigger.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPutDown::InstanceConfig::InstanceConfig()
{
  waitInternalBehavior = nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPutDown::BehaviorReactToPutDown(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPutDown::~BehaviorReactToPutDown()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPutDown::WantsToBeActivatedBehavior() const
{
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPutDown::InitBehavior()
{
  // Waiting animation that cancels itself when the robot has been on its treads for at least 3 seconds,
  // or the animation has been played for at least 3 seconds.
  _iConfig.waitInternalBehavior = FindAnonymousBehaviorByName("PutDownWaitInternal");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPutDown::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.waitInternalBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPutDown::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  TransitionToPlayingPutDownAnimation();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPutDown::TransitionToPlayingPutDownAnimation()
{
  DEBUG_SET_STATE(PlayingPutDownAnimation);
  
  // Send DAS event for activation.
  {
    DASMSG(behavior_cliffreaction, "Behavior.PutDownReaction", "The robot reacted to being put down.");
    DASMSG_SEND();
  }
    
  // Get the appropriate action/animation to play
  auto action = new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToPutDown);
  DelegateIfInControl(action, &BehaviorReactToPutDown::TransitionToHeadCalibration);
}
  
void BehaviorReactToPutDown::TransitionToHeadCalibration()
{
  DEBUG_SET_STATE(CalibratingHead);

  // Force the head to recalibrate since it's possible that Vector may have been put down too aggressively,
  // resulting in gear slippage for the motor.
  DelegateIfInControl(new CalibrateMotorAction(true, false,
                                               EnumToString(MotorCalibrationReason::BehaviorReactToPutDown)),
                      &BehaviorReactToPutDown::TransitionToPlayingWaitAnimation);
}
  
void BehaviorReactToPutDown::TransitionToPlayingWaitAnimation()
{
  DEBUG_SET_STATE(Waiting);
 
  // Play the waiting animation.
  DelegateIfInControl(_iConfig.waitInternalBehavior.get());
}

}
}
