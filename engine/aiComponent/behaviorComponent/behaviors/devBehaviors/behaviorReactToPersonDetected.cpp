/**
 * File: BehaviorReactToPersonDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2018-05-30
 *
 * Description: Reacts when a person is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToPersonDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::DynamicVariables::DynamicVariables()
{
  state = State::Starting;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::BehaviorReactToPersonDetected(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::~BehaviorReactToPersonDetected()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPersonDetected::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // TODO: the behavior is active now, time to do something!
  PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.OnBehaviorActivated", "I am active!");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::BehaviorUpdate() 
{

  if( ! IsActivated() ) {
    return;
  }

  PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "I am updated!");

  if (_dVars.state == State::Starting) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Starting the turn");
    TurnTowardsPoint();
  }
  else if (_dVars.state == State::Turning) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Waiting for turn to be completed");
    return;
  }
  else if (_dVars.state == State::Completed) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Finished turning, now what?");
  }
}

void BehaviorReactToPersonDetected::TurnTowardsPoint()
{
  _dVars.state = State::Turning;

  const float angle = 30; // degrees
  auto* action = new TurnInPlaceAction(DEG_TO_RAD(angle), false);
  action->SetAccel( MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2 );
  action->SetMaxSpeed( MAX_BODY_ROTATION_SPEED_RAD_PER_SEC );

  CancelDelegates(false);
  DelegateIfInControl(action, [this](ActionResult result) {
    FinishedTurning();
  });
}

void BehaviorReactToPersonDetected::FinishedTurning() {

  PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.FinishedTurning", "Finished turning");
  _dVars.state = State::Completed;
}

} // namespace Cozmo
} // namespace Anki
