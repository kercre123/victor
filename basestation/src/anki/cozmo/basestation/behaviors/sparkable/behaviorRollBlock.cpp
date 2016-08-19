/**
 * File: behaviorRollBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-05
 *
 * Description: This behavior rolls blocks when it see's them facing a direction specified in config
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorRollBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "util/console/consoleInterface.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"


namespace Anki {
namespace Cozmo {

static const char* const kPutDownAnimGroupKey = "put_down_animation_group";

CONSOLE_VAR(f32, kBRB_ScoreIncreaseForAction, "Behavior.RollBlock", 0.8f);
CONSOLE_VAR(f32, kBRB_MaxTowardFaceAngle_deg, "Behavior.RollBlock", 90.f);
CONSOLE_VAR(s32, kBRB_MaxRollRetries,         "Behavior.RollBlock", 1);
  
BehaviorRollBlock::BehaviorRollBlock(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _blockworldFilter( new BlockWorldFilter )
  , _robot(robot)
{
  SetDefaultName("RollBlock");

  JsonTools::GetValueOptional(config,kPutDownAnimGroupKey,_putDownAnimTrigger);

  // set up the filter we will use for finding blocks we care about
  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &BehaviorRollBlock::FilterBlocks, this, std::placeholders::_1) );
}

bool BehaviorRollBlock::IsRunnableInternal(const Robot& robot) const
{
  UpdateTargetBlock(robot);
  
  return _targetBlock.IsSet() || IsActing();
}

Result BehaviorRollBlock::InitInternal(Robot& robot)
{
  if (robot.IsCarryingObject())
  {
    TransitionToSettingDownBlock(robot);
  }
  else
  {
    if(!_shouldStreamline){
      TransitionToReactingToBlock(robot);
    }else{
      TransitionToPerformingAction(robot);
    }
  }
  return Result::RESULT_OK;
}
  
void BehaviorRollBlock::StopInternal(Robot& robot)
{
  ResetBehavior(robot);
}

void BehaviorRollBlock::UpdateTargetBlock(const Robot& robot) const
{
  if (!robot.IsCarryingObject())
  {
    const ObservableObject* closestObj = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(), *_blockworldFilter);
    if( nullptr != closestObj ) {
      _targetBlock = closestObj->GetID();
    }
    else {
      _targetBlock.UnSet();
    }
  }
  else
  {
    const ObservableObject* carriedObj = robot.GetBlockWorld().GetObjectByID(robot.GetCarryingObject());
    if (nullptr != carriedObj && carriedObj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS)
    {
      _targetBlock = carriedObj->GetID();
    }
    else
    {
      _targetBlock.UnSet();
    }
  }
}

bool BehaviorRollBlock::FilterBlocks(const ObservableObject* obj) const
{
  return (!obj->IsPoseStateUnknown() &&
          _robot.CanPickUpObjectFromGround(*obj) &&
          obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS);
}

void BehaviorRollBlock::TransitionToSettingDownBlock(Robot& robot)
{
  DEBUG_SET_STATE(SettingDownBlock);
  
  if( _putDownAnimTrigger == AnimationTrigger::Count) {
    constexpr float kAmountToReverse_mm = 90.f;
    IActionRunner* actionsToDo = new CompoundActionSequential(robot, {
      new DriveStraightAction(robot, -kAmountToReverse_mm, DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
      new PlaceObjectOnGroundAction(robot)});
    StartActing(actionsToDo, &BehaviorRollBlock::TransitionToReactingToBlock);
  }
  else {
    StartActing(new TriggerAnimationAction(robot, _putDownAnimTrigger),
                [this](Robot& robot) {
                  // use same logic as put down block behavior
                  StartActing(BehaviorPutDownBlock::CreateLookAfterPlaceAction(robot, false),
                              [this, &robot]() {
                                if(robot.IsCarryingObject()) {
                                  // No matter what, even if we didn't see the object we were
                                  // putting down for some reason, mark the robot as not carrying
                                  // anything so we don't get stuck in a loop of trying to put
                                  // something down (i.e. assume the object is no longer on our lift)
                                  // TODO: We should really be using some kind of PlaceOnGroundAction instead of raw animation (see COZMO-2192)
                                  PRINT_NAMED_WARNING("BehaviorRollBlock.TransitionToSettingDownBlock.DidNotSeeBlock",
                                                      "Forcibly setting carried objects as unattached (See COZMO-2192)");
                                  robot.SetCarriedObjectAsUnattached();
                                }
                                TransitionToReactingToBlock(robot);
                              });
                });
  }
}

  
  
void BehaviorRollBlock::TransitionToReactingToBlock(Robot& robot)
{
  DEBUG_SET_STATE(ReactingToBlock);
  
  if( robot.IsCarryingObject() ) {
    PRINT_NAMED_ERROR("BehaviorRollBlock.ReactWhileHolding",
                      "block should be put down at this point. Bailing from behavior");
    _targetBlock.UnSet();
    return;
  }
  
  // Turn towards the object and then react to it before performing the roll action
  StartActing(new CompoundActionSequential(robot, {
                new TurnTowardsObjectAction(robot, _targetBlock, PI_F),
    new TriggerAnimationAction(robot, AnimationTrigger::RollBlockInitial),
              }),
              [this,&robot]{ this->TransitionToPerformingAction(robot); });
}

void BehaviorRollBlock::TransitionToPerformingAction(Robot& robot, bool isRetry)
{
  DEBUG_SET_STATE(PerformingAction);
  
  if( ! _targetBlock.IsSet() ) {
    PRINT_NAMED_WARNING("BehaviorRollBlock.NoBlockID",
                        "%s: Transitioning to action state, but we don't have a valid block ID",
                        GetName().c_str());
    return;
  }
  
  if(isRetry) {
    ++_numRollActionRetries;
    PRINT_NAMED_INFO("BehaviorRollBlock.TransitionToPerformingAction.Retrying",
                     "Retry %d of %d", _numRollActionRetries, kBRB_MaxRollRetries);
  } else {
    _numRollActionRetries = 0;
  }

  // Only turn towards face if this is _not_ a retry
  const Radians maxTurnToFaceAngle( (isRetry ? 0 : DEG_TO_RAD(90)) );
  const bool sayNameBefore = !_shouldStreamline;
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetBlock,
                                                                    false, 0, false,
                                                                    maxTurnToFaceAngle, sayNameBefore);
  rollAction->RollToUpright();
  rollAction->SetSayNameAnimationTrigger(AnimationTrigger::RollBlockPreActionNamedFace);
  rollAction->SetNoNameAnimationTrigger(AnimationTrigger::RollBlockPreActionUnnamedFace);
      
  // Roll the object and then look at a person
  StartActing(rollAction,
              [&,this](const ExternalInterface::RobotCompletedAction& msg) {
                switch(msg.result)
                {
                  case ActionResult::SUCCESS:
                    if(!_shouldStreamline){
                      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::RollBlockSuccess));
                    }
                    IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
                    BehaviorObjectiveAchieved(BehaviorObjective::BlockRolled);
                    break;
                  
                  case ActionResult::FAILURE_RETRY:
                    if(_numRollActionRetries < kBRB_MaxRollRetries)
                    {
                      SetupRetryAction(robot, msg);
                      break;
                    }
                    
                    // else: too many retries, fall through to failure abort
                    
                  case ActionResult::FAILURE_ABORT:
                    // assume that failure is because we didn't visually verify (although it could be due to an
                    // error)
                    PRINT_NAMED_INFO("BehaviorRollBlock.FailedAbort",
                                     "Failed to verify roll, searching for block");
                    StartActing(new SearchSideToSideAction(robot),
                                [this](Robot& robot) {
                                  if( IsRunnable(robot) ) {
                                    TransitionToPerformingAction(robot);
                                  }
                                  // TODO:(bn) if we actually succeeded here, we should play the success anim,
                                  // or at least a recognition anim
                                });
                    break;
                    
                  default:
                    // other failure, just end
                    PRINT_NAMED_INFO("BehaviorRollBlock.FailedRollAction",
                                     "action failed with %s, behavior ending",
                                     EnumToString(msg.result));
                } // switch(msg.result)
                
              });
  IncreaseScoreWhileActing( kBRB_ScoreIncreaseForAction );
}
  
  
void BehaviorRollBlock::SetupRetryAction(Robot& robot, const ExternalInterface::RobotCompletedAction& msg)
{
  // Pick which retry animation, if any, to play. Then try performing the action again,
  // with "isRetry" set to true.
  
  IActionRunner* animAction = nullptr;
  switch(msg.completionInfo.Get_objectInteractionCompleted().result)
  {
    case ObjectInteractionResult::INCOMPLETE:
    case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RollBlockRealign);
      break;
    }
      
    default: {
      animAction = new TriggerAnimationAction(robot, AnimationTrigger::RollBlockRetry);
      break;
    }
  }
  
  if( nullptr != animAction ) {
    StartActing(animAction, [this,&robot]() {
      this->TransitionToPerformingAction(robot, true);
    });
  }
  else {
    TransitionToPerformingAction(robot, true);
  }

}

void BehaviorRollBlock::ResetBehavior(Robot& robot)
{
  _targetBlock.UnSet();
}

}
}
