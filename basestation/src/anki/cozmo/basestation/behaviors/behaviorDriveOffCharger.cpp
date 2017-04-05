/**
 * File: behaviorDriveOffCharger.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to the edge off a charger and deal with the firmware cliff stop
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviors/behaviorDriveOffCharger.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageGameToEngine.h"


namespace Anki {
namespace Cozmo {

namespace{
static const float kInitialDriveSpeed = 100.0f;
static const float kInitialDriveAccel = 40.0f;

static const char* const kExtraDriveDistKey = "extraDistanceToDrive_mm";

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersDriveOffChargerArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              true},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersDriveOffChargerArray),
              "Reaction triggers duplicate or non-sequential");

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::BehaviorDriveOffCharger(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("DriveOffCharger");
  float extraDist_mm = config.get(kExtraDriveDistKey, 0.0f).asFloat();
  _distToDrive_mm = Charger::GetLength() + extraDist_mm;
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorDriveOffCharger.DriveDist",
                 "Driving %fmm off the charger (%f length + %f extra)",
                 _distToDrive_mm,
                 Charger::GetLength(),
                 extraDist_mm);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveOffCharger::IsRunnableInternal(const BehaviorPreReqRobot& preReqData ) const
{
  // assumes it's not possible to be OnCharger without being OnChargerPlatform
  const Robot& robot = preReqData.GetRobot();
  DEV_ASSERT(robot.IsOnChargerPlatform() || !robot.IsOnCharger(),
             "BehaviorDriveOffCharger.IsRunnableInternal.InconsistentChargerFlags");

  // can run any time we are on the charger platform
  
  // bn: this used to be "on charger platform", but that caused weird issues when the robot thought it drove
  // over the platform later due to mis-localization. Then this changed to "on charger" to avoid those issues,
  // but that caused other issues (if the robot was bumped during wakeup, it wouldn't drive off the
  // charger). Now, we've gone back to OnChargerPlatofrm but fixed it to work better (see the comments in
  // robot.h)
  const bool onChargerPlatform = robot.IsOnChargerPlatform();
  return onChargerPlatform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveOffCharger::InitInternal(Robot& robot)
{
  //Disable Cliff Reaction during behavior
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersDriveOffChargerArray);

  robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::DriveStartLaunch,
                                                            AnimationTrigger::DriveLoopLaunch,
                                                            AnimationTrigger::DriveEndLaunch});

  const bool onTreads = robot.GetOffTreadsState() == OffTreadsState::OnTreads;
  if( onTreads ) {
    TransitionToDrivingForward(robot);
  }
  else {
    // otherwise we'll just wait in Update until we are on the treads (or removed from the charger)
    DEBUG_SET_STATE(WaitForOnTreads);
  }
  
  return Result::RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
}
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorDriveOffCharger::UpdateInternal(Robot& robot)
{

  if( robot.IsOnChargerPlatform() ) {
    const bool onTreads = robot.GetOffTreadsState() == OffTreadsState::OnTreads;
    if( !onTreads ) {
      // if we aren't on the treads anymore, but we are on the charger, then the user must be holding us
      // inside the charger.... so just sit in this behavior doing nothing until we get put back down
      StopActing(false);
      DEBUG_SET_STATE(WaitForOnTreads);
      // TODO:(bn) play an idle here?
    }
    else if( ! IsActing() ) {
      // if we finished the last action, but are still on the charger, queue another one
      TransitionToDrivingForward(robot);
    }
    
    return Status::Running;
  }

  if( IsActing() ) {
    // let the action finish
    return Status::Running;
  }
  else {  
    // store in whiteboard our success
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    robot.GetAIComponent().GetWhiteboard().GotOffChargerAtTime( curTime );

    return Status::Complete;
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingForward(Robot& robot)
{
  DEBUG_SET_STATE(DrivingForward);
  if( robot.IsOnChargerPlatform() )
  {
    // probably interrupted by getting off the charger platform
    DriveStraightAction* action = new DriveStraightAction(robot, _distToDrive_mm, kInitialDriveSpeed);
    action->SetAccel(kInitialDriveAccel);
    StartActing(action,[this, &robot](ActionResult res){
      if(res == ActionResult::SUCCESS){
        BehaviorObjectiveAchieved(BehaviorObjective::DroveAsIntended);
        robot.GetMoodManager().TriggerEmotionEvent("DriveOffCharger", MoodManager::GetCurrentTimeInSeconds());
      }
    });
    // the Update function will transition back to this state (or out of the behavior) as appropriate
  }
}

}
}

