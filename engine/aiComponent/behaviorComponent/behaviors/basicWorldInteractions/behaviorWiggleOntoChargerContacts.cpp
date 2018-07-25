/**
 * File: BehaviorWiggleOntoChargerContacts.cpp
 *
 * Author: Brad
 * Created: 2018-04-26
 *
 * Description: Perform a few small motions or wiggles to get back on the charger contacts if we got bumped off
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorWiggleOntoChargerContacts.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {

#define CONSOLE_GROUP "Behavior.InteractWithFaces"
#define SET_STATE(s) { _dVars.state = State::s; SetDebugStateName(#s); }

const float kMaxPitchToRun_degs = 10.0f;
const float kMinPitchToRun_degs = -25.0f;

CONSOLE_VAR_RANGED(f32, kWiggle_ForwardDist_mm,     CONSOLE_GROUP, 6.0f, 0.0f, 20.0f);
CONSOLE_VAR_RANGED(f32, kWiggle_ForwardSpeed_mmps,  CONSOLE_GROUP, 120.0, 0.0f, 200.0f);
CONSOLE_VAR_RANGED(f32, kWiggle_BackupDist_mm,      CONSOLE_GROUP, 15.0f, 0.0f, 20.0f);
CONSOLE_VAR_RANGED(f32, kWiggle_BackupSpeed_mmps,   CONSOLE_GROUP, 100.0f, 0.0f, 200.0f);
CONSOLE_VAR_RANGED(f32, kWiggle_VerifyWaitTime_s,   CONSOLE_GROUP, 0.25f, 0.0f, 2.0f);
CONSOLE_VAR_RANGED(f32, kWiggle_BackupSettleTime_s, CONSOLE_GROUP, 0.4f, 0.0f, 2.0f);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWiggleOntoChargerContacts::InstanceConfig::InstanceConfig()
  : maxAttempts(1)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWiggleOntoChargerContacts::DynamicVariables::DynamicVariables()
  : state(State::BackingUp)
  , attempts(0)
  , contactTime_s(-1.0f)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWiggleOntoChargerContacts::BehaviorWiggleOntoChargerContacts(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWiggleOntoChargerContacts::~BehaviorWiggleOntoChargerContacts()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorWiggleOntoChargerContacts::WantsToBeActivatedBehavior() const
{
  if( GetBEI().GetRobotInfo().IsOnChargerContacts() ) {
    // no need to run if we're already on the contacts
    return false;
  }
  
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads ) {
    return true;
  }
  else if( GetBEI().GetOffTreadsState() == OffTreadsState::InAir ) {

    // only want to attempt this if we are backed up a bit onto the charger, so only if the pitch is
    // reasonable (if the robot is rolled too much, it should get counted as on-side
    const Radians& pitch =  GetBEI().GetRobotInfo().GetPitchAngle();
    if( pitch <= DEG_TO_RAD(kMaxPitchToRun_degs) && pitch >= DEG_TO_RAD(kMinPitchToRun_degs) ) {
      return true;
    }    
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;

  // allow off treads in case the robot drives up onto the back of the charger and thinks it's in the air
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  StartStateMachine();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::StartStateMachine()
{
  _dVars.attempts++;

  if( _dVars.attempts > _iConfig.maxAttempts ) {
    return;
  }

  TransitionToBackingUp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::TransitionToBackingUp()
{
  SET_STATE(BackingUp);

  const bool playAnimation = false;
  auto* action = new DriveStraightAction(-kWiggle_BackupDist_mm, kWiggle_BackupSpeed_mmps, playAnimation);
  action->SetCanMoveOnCharger(true);

  // if we hit the contacts during the option, the update will stop us for verification
  DelegateIfInControl(action, &BehaviorWiggleOntoChargerContacts::TransitionToMovingForward);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::TransitionToMovingForward()
{
  SET_STATE(MovingForward);

  const bool playAnimation = false;
  auto* action = new DriveStraightAction(kWiggle_ForwardDist_mm, kWiggle_ForwardSpeed_mmps, playAnimation);
  action->SetCanMoveOnCharger(true);

  // after moving forward, if we don't get interrupted because of the contacts, try looping the state machine
  // again
  DelegateIfInControl(action, &BehaviorWiggleOntoChargerContacts::StartStateMachine);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::TransitionToVerifyingContacts()
{
  SET_STATE(VerifyingContacts);
  
  DelegateIfInControl(new WaitAction(kWiggle_VerifyWaitTime_s), [this]() {
      if( !GetBEI().GetRobotInfo().IsOnChargerContacts() ) {
        // came off contacts, try again
        StartStateMachine();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWiggleOntoChargerContacts::BehaviorUpdate() 
{
  if( IsActivated() ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool onContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();
    if( onContacts && _dVars.contactTime_s < 0.0f ) {
      // first contact with the charger, update time
      _dVars.contactTime_s = currTime_s;
    }
    else if( !onContacts && _dVars.contactTime_s >= 0.0f ) {
      // no longer on contacts, clear time
      _dVars.contactTime_s = -1.0f;
    }
    
    switch(_dVars.state) {
      case State::BackingUp: {
        // let the keep driving a bit to make sure we are "fully" on the contacts
        if( onContacts && currTime_s - _dVars.contactTime_s > kWiggle_BackupSettleTime_s ) {
          CancelDelegates(false);
          TransitionToVerifyingContacts();
        }
        break;
      }
        
      case State::MovingForward: {
        // stop immediately since this means we were up on the back wall and now "dropped" into place
        CancelDelegates(false);
        TransitionToVerifyingContacts();
        break;
      }

      case State::VerifyingContacts: break;
    }
  }
}

}
}
