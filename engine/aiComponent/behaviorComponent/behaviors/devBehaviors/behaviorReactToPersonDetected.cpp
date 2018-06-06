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


#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToPersonDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPersonDetected.h"
#include "engine/aiComponent/salientPointsDetectorComponent.h"
#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

namespace {
const BackpackLights kLightsOn = {
    .onColors               = {{NamedColors::YELLOW,NamedColors::RED,NamedColors::BLUE}},
    .offColors              = {{NamedColors::YELLOW,NamedColors::RED,NamedColors::BLUE}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
};

const BackpackLights kLightsOff = {
    .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{0,0,0}},
    .offPeriod_ms           = {{0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPersonDetected::DynamicVariables::DynamicVariables()
{
  Reset();
}

void BehaviorReactToPersonDetected::DynamicVariables::Reset()
{
  state = State::Starting;
  blinkOn = false;
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
  PRINT_CH_DEBUG("Behaviors", "BehaviorReactToPersonDetected.WantsToBeActivatedBehavior.Called", "Wake Up?");

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
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

  PRINT_CH_DEBUG("Behaviors", "BehaviorReactToPersonDetected.OnBehaviorActivated", "I am active!");

  auto& component = GetBEI().GetAIComponent().GetComponent<SalientPointsDetectorComponent>();
  component.GetLastPersonDetectedData(_dVars.lastPersonDetected);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPersonDetected::BehaviorUpdate() 
{
  PRINT_CH_DEBUG("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "I am being updated");


  const bool motorsMoving = GetBEI().GetRobotInfo().GetMoveComponent().IsMoving();
  const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

  // for the moment only handle the case where the robot is moving

  if (motorsMoving) {
    if (_dVars.state != State::Turning) {
      // the robot is moving not because we told it to do so

      TransitionToCompleted();
      return;
    }
  }
  if (pickedUp) {
    // definitively stop here
    TransitionToCompleted();
    return;
  }

  if( ! IsActivated() ) {
    PRINT_CH_DEBUG("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "I am actually not active :(");
    return;
  }

  if (_dVars.state == State::Starting) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Starting the turn");
    BlinkLight(true);
    TurnTowardsPoint();
  }
  else if (_dVars.state == State::Turning) {
    PRINT_CH_DEBUG("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Waiting for turn to be completed");
    return;
  }
  else if (_dVars.state == State::Completed) {
    PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.BehaviorUpdate", "Finished turning");
    BlinkLight(false);
    TransitionToCompleted();
  }
}

void BehaviorReactToPersonDetected::BlinkLight(bool on)
{
  _dVars.blinkOn = on;
  GetBEI().GetBodyLightComponent().SetBackpackLights( _dVars.blinkOn ? kLightsOn : kLightsOff );
}

void BehaviorReactToPersonDetected::TurnTowardsPoint()
{
  _dVars.state = State::Turning;

  PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.TurnTowardsPoint.TurningInfo", 
                "Turning towards %f, %f at timestamp %d",
                _dVars.lastPersonDetected.x_img,
                _dVars.lastPersonDetected.y_img,
                _dVars.lastPersonDetected.timestamp);
  auto* action = new TurnTowardsImagePointAction(_dVars.lastPersonDetected);

  CancelDelegates(false);
  DelegateIfInControl(action, [this](ActionResult result) {
    FinishedTurning();
  });
}

void BehaviorReactToPersonDetected::FinishedTurning() {

  PRINT_CH_INFO("Behaviors", "BehaviorReactToPersonDetected.FinishedTurning", "Finished turning");
  _dVars.state = State::Completed;
}


void BehaviorReactToPersonDetected::TransitionToCompleted()
{
  // for the moment just stop doing stuff
  _dVars.state = State::Completed;
  CancelDelegates(false);
  if (IsActivated()) {
    CancelSelf();
  }
}

} // namespace Cozmo
} // namespace Anki
