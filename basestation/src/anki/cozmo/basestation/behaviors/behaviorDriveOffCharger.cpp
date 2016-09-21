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
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageGameToEngine.h"



namespace Anki {
namespace Cozmo {

static const float kInitialDriveSpeed = 100.0f;
static const float kInitialDriveAccel = 40.0f;

static const char* const kExtraDriveDistKey = "extraDistanceToDrive_mm";

static const std::set<BehaviorType> kBehaviorsToDisable = {BehaviorType::ReactToCliff,
                                                           BehaviorType::ReactToUnexpectedMovement,
                                                           BehaviorType::AcknowledgeObject,
                                                           BehaviorType::AcknowledgeFace,
                                                           BehaviorType::ReactToCubeMoved,
                                                           BehaviorType::KnockOverCubes};
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::BehaviorDriveOffCharger(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _internalScore(0.0f)
{
  SetDefaultName("DriveOffCharger");
  float extraDist_mm = config.get(kExtraDriveDistKey, 0.0f).asFloat();
  _distToDrive_mm = Charger::GetLength() + extraDist_mm;

  SubscribeToTags({
    EngineToGameTag::ChargerEvent,
  });
  
  PRINT_NAMED_DEBUG("BehaviorDriveOffCharger.DriveDist",
                    "Driving %fmm off the charger (%f length + %f extra)",
                    _distToDrive_mm,
                    Charger::GetLength(),
                    extraDist_mm);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveOffCharger::IsRunnableInternal(const Robot& robot) const
{
  // assumes it's not possible to be OnCharger without being OnChargerPlatform
  //ASSERT_NAMED(robot.IsOnChargerPlatform() || !robot.IsOnCharger(),
  //             "BehaviorDriveOffCharger.IsRunnableInternal.InconsistentChargerFlags");
  
  // can run any time we are on a platform
  const bool onChargerPlatform = robot.IsOnChargerPlatform();
  return onChargerPlatform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveOffCharger::InitInternal(Robot& robot)
{
  TransitionToDrivingForward(robot);
  _timesResumed = 0;
  
  //Disable Cliff Reaction during behavior
  SmartDisableReactionaryBehavior(kBehaviorsToDisable);

  return Result::RESULT_OK;
}
  
void BehaviorDriveOffCharger::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
}


Result BehaviorDriveOffCharger::ResumeInternal(Robot& robot)
{
  // We hit the end of the charger, just keep driving.
  TransitionToDrivingForward(robot);
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  bool onCharger;
  
  switch(event.GetData().GetTag()){
    case EngineToGameTag::ChargerEvent:
      onCharger = event.GetData().Get_ChargerEvent().onCharger;
      if(onCharger){
        _internalScore = 1000.0f;
      }
    default:
      break;
  }
}
  
float BehaviorDriveOffCharger::EvaluateScoreInternal(const Robot& robot) const
{
  if(robot.GetOffTreadsState() != OffTreadsState::OnTreads){
    return 0.f;
  }
  
  return _internalScore;
}


  
IBehavior::Status BehaviorDriveOffCharger::UpdateInternal(Robot& robot)
{
  // Emergency counter for demo rare bug. Usually we just get the chargerplatform message.
  // HACK: figure out why IsOnChargerPlatform might be incorrect.
  if( robot.IsOnChargerPlatform() && _timesResumed <= 2)
  {
    if( ! IsActing() ) {
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
    const float curTime = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );
    robot.GetBehaviorManager().GetWhiteboard().GotOffChargerAtTime( curTime );
    _internalScore = 0.0f;

    return Status::Complete;
  }
}


void BehaviorDriveOffCharger::TransitionToDrivingForward(Robot& robot)
{
  DEBUG_SET_STATE(DrivingForward);
  if( robot.IsOnChargerPlatform() )
  {
    _timesResumed++;
    // Numbers shared with demoFearEdge but will move here since this is in freeplay
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::DriveStartLaunch,
                                                              AnimationTrigger::DriveLoopLaunch,
                                                              AnimationTrigger::DriveEndLaunch});
    // probably interrupted by getting off the charger platform
    DriveStraightAction* action = new DriveStraightAction(robot, _distToDrive_mm, kInitialDriveSpeed);
    action->SetAccel(kInitialDriveAccel);
    StartActing(action,[this](ActionResult res){
      if(res == ActionResult::SUCCESS){
        BehaviorObjectiveAchieved(BehaviorObjective::DroveAsIntended);
      }
    });
    // the Update function will transition back to this state (or out of the behavior) as appropriate

  }
}

}
}

