/**
 * File: behaviorReactToHand.cpp
 *
 * Author:  Andrew Stein
 * Created: 2018-11-14
 *
 * Description: Animation sequence with drive-to based on prox distance for reacting to a hand.
 *              Note that this is JUST the reaction sequence: it does not check for a hand and
 *              always wants to be activated.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToHand.h"

#include "clad/types/animationTrigger.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {
  CONSOLE_VAR_RANGED( float, kHandReaction_DriveForwardSpeed_mmps, "Behavior.ReactToHand", 100.0f, 0.f, MAX_SAFE_WHEEL_SPEED_MMPS);
  CONSOLE_VAR_RANGED( float, kReactToHand_DriveDistanceFraction,   "Behavior.ReactToHand", 1.f, 0.f, 1.f);
  CONSOLE_VAR_RANGED( float, kReactToHand_PitchAngleThresh_deg,    "Behavior.ReactToHand", 2.f, 0.f, 10.f);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToHand::BehaviorReactToHand(const Json::Value& config)
  : ICozmoBehavior(config)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToHand::~BehaviorReactToHand()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToHand::IsRobotLifted() const
{
  const Radians& pitchAngle_rad = this->GetBEI().GetRobotInfo().GetPitchAngle();
  const float pitchDelta_rad = (pitchAngle_rad - _pitchAngleStart_rad).ToFloat();
  const bool isLifted = Util::IsFltGT(pitchDelta_rad, DEG_TO_RAD(kReactToHand_PitchAngleThresh_deg));
  LOG_DEBUG("BehaviorReactToHand.IsRobotLifted", "Angle:%.1fdeg = %s",
            RAD_TO_DEG(pitchDelta_rad), (isLifted ? "YES" : "NO"));
  return isLifted;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToHand::OnBehaviorActivated()
{
  // Get distance to hand:
  const auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  float driveDist_mm = 0.f;
  if( !proxData.foundObject )
  {
    PRINT_NAMED_WARNING("BehaviorReactToHand.OnBehaviorActivated.InvalidProxReadingToHand", "");
    
    // TODO: should probably handle this failure differently (and have it impact which hand reaction we choose
    //       for now (for review/test) just fake a distance
    driveDist_mm = 10.f;
  }
  else
  {
    driveDist_mm = kReactToHand_DriveDistanceFraction * (float)proxData.distance_mm;
  }
  
  CompoundActionParallel* driveAction = new CompoundActionParallel({
    new DriveStraightAction( driveDist_mm, kHandReaction_DriveForwardSpeed_mmps, false),
    new ReselectingLoopAnimationAction{AnimationTrigger::ExploringReactToHandDrive},
    new WaitForLambdaAction([](Robot& robot) {
      const bool detectedUnexpectedMovement = (robot.GetMoveComponent().IsUnexpectedMovementDetected());
      if(detectedUnexpectedMovement) {
        LOG_DEBUG("BehaviorReactToHand.OnBehaviorActivated.UnexpectedMovementInterruptingDrive", "");
      }
      return detectedUnexpectedMovement;
    }),
  });
  
  // The compound drive action will complete either when the DriveStraight finishes or unexpected movement
  // is detected (the looping anim will never finish).
  driveAction->SetShouldEndWhenFirstActionCompletes(true);

  const bool kIgnoreFailure = true;
  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction(new TriggerLiftSafeAnimationAction{AnimationTrigger::ExploringReactToHandGetIn});
  action->AddAction(driveAction, kIgnoreFailure);
  
  _pitchAngleStart_rad = GetBEI().GetRobotInfo().GetPitchAngle().ToFloat();
  
  SetDebugStateName("ActivatedAndDriving");
  DelegateIfInControl(action, [this]()
                      {
                        // If the robot has driven up on the hand, don't use the lift: just react
                        if(IsRobotLifted())
                        {
                          TransitionToReaction();
                        }
                        else
                        {
                          TransitionToLifting();
                        }
                      });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToHand::TransitionToLifting()
{
  SetDebugStateName("Lifting");
  DelegateIfInControl(new TriggerLiftSafeAnimationAction{AnimationTrigger::ExploringReactToHandLift}, [this]()
                      {
                        // If lifting didn't raise the robot, go straight to get out. If it did, react first.
                        if(IsRobotLifted())
                        {
                          TransitionToReaction();
                        }
                        else
                        {
                          TransitionToGetOut();
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToHand::TransitionToReaction()
{
  SetDebugStateName("Reacting");
  DelegateIfInControl(new TriggerLiftSafeAnimationAction{AnimationTrigger::ExploringReactToHandReaction},
                      &BehaviorReactToHand::TransitionToGetOut);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToHand::TransitionToGetOut()
{
  SetDebugStateName("GetOut");
  DelegateIfInControl(new TriggerLiftSafeAnimationAction{AnimationTrigger::ExploringReactToHandGetOut});
}

}
}
