/**
 * File: BehaviorTurnTowardsPerson.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2018-05-30
 *
 * Description: Turns towards a person if detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorTurnTowardsPerson.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"

namespace Anki {
namespace Cozmo {

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnTowardsPerson::InstanceConfig::InstanceConfig(const Json::Value& config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnTowardsPerson::DynamicVariables::DynamicVariables()
{
  Reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::DynamicVariables::Reset()
{
  state = State::Starting;
  lastPersonDetected = Vision::SalientPoint();
  hasToStop = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnTowardsPerson::BehaviorTurnTowardsPerson(const Json::Value& config)
 : ICozmoBehavior(config), _iConfig(config)
{
  MakeMemberTunable(_iConfig.coolDownTime, "coolDownTime");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurnTowardsPerson::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::OnBehaviorActivated()
{

  LOG_DEBUG( "BehaviorReactToPersonDetected.OnBehaviorActivated", "I am active!");

  auto& component = GetBEI().GetAIComponent().GetComponent<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestPersons;

  //Get all the latest persons
  component.GetSalientPointSinceTime(latestPersons, Vision::SalientPointType::Person,
                                     _dVars.lastPersonDetected.timestamp);

  if (latestPersons.empty()) {
    LOG_DEBUG( "BehaviorReactToPersonDetected.OnBehaviorActivated.NoPersonDetected", ""
        "Activated but no person with a timestamp > %u", _dVars.lastPersonDetected.timestamp);
    _dVars.hasToStop = true;
    return;
  }

  // reset dynamic variables
  _dVars = DynamicVariables();

  // Select the best one
  // Choose the latest one, or the biggest if tie
  auto maxElementIter = std::max_element(latestPersons.begin(), latestPersons.end(),
                                         [](const Vision::SalientPoint& p1, const Vision::SalientPoint& p2) {
                                           if (p1.timestamp == p2.timestamp) {
                                             return p1.area_fraction < p2.area_fraction;
                                           }
                                           else {
                                             return  p1.timestamp < p2.timestamp;
                                           }
                                         }
  );
  _dVars.lastPersonDetected = *maxElementIter;


  DEV_ASSERT(_dVars.lastPersonDetected.salientType == Vision::SalientPointType::Person,
             "BehaviorReactToPersonDetected.OnBehaviorActivated.LastSalientPointMustBePerson");

  // Action!
  TransitionToTurnTowardsPoint();

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::BehaviorUpdate()
{
  LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate", "I am being updated");

  if (_dVars.hasToStop) {
    TransitionToCompleted();
    return;
  }

  const bool motorsMoving = GetBEI().GetRobotInfo().GetMoveComponent().IsMoving();
  const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

  // for the moment only handle the case where the robot is moving
  if (motorsMoving) {
    if (_dVars.state != State::Turning) {
      // the robot is moving not because we told it to do so
      LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate.Moving",
                     "Robot is moving but it wasn't me!");
      TransitionToCompleted();
      return;
    }
  }
  if (pickedUp) {
    // definitively stop here
    LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate.PickedUp",
                   "Robot has been picked up");
    TransitionToCompleted();
    return;
  }

  if( ! IsActivated() ) {
    LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate", "I am actually not active :(");
    return;
  }

}

void BehaviorTurnTowardsPerson::TransitionToTurnTowardsPoint()
{
  DEBUG_SET_STATE(Turning);

  _dVars.state = State::Turning;

  LOG_DEBUG( "BehaviorReactToPersonDetected.TransitionToTurnTowardsPoint.TurningInfo",
                "Turning towards %f, %f at timestamp %d",
                _dVars.lastPersonDetected.x_img,
                _dVars.lastPersonDetected.y_img,
                _dVars.lastPersonDetected.timestamp);
  auto* action = new TurnTowardsImagePointAction(_dVars.lastPersonDetected);

  CancelDelegates(false);
  DelegateIfInControl(action, &BehaviorTurnTowardsPerson::TransitionToFinishedTurning);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::TransitionToFinishedTurning() {

  DEBUG_SET_STATE(FinishedTurning);

  LOG_DEBUG( "BehaviorReactToPersonDetected.TransitionToFinishedTurning", "Finished turning");
  _dVars.state = State::FinishedTurning;

  // There might be some other actions here, but completing for the moment
  TransitionToCompleted();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnTowardsPerson::TransitionToCompleted()
{
  if (_dVars.state != State::Completed) { // avoid transitioning to complete over and over
    DEBUG_SET_STATE(Completed);

    // for the moment just stop doing stuff
    _dVars.state = State::Completed;
  }
}

} // namespace Cozmo
} // namespace Anki
