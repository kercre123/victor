/**
 * File: behaviorReactToNewBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-21
 *
 * Description: React to a block which has never been seen before
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactToNewBlock.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

#define DEBUG_STACK_CHECKING 0
#define DEBUG_TAKE_BLOCK_FROM_HAND 0

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(f32, kBRTNB_ScoreIncreaseForReaction, "Behavior.ReactToBlock", 0.3f);
CONSOLE_VAR(f32, kBRTNB_ScoreIncreaseForPickup, "Behavior.ReactToBlock", 0.8f);
CONSOLE_VAR(f32, kBRTNB_minHeightForBigReact_mm, "Behavior.ReactToBlock", 45.0f);
CONSOLE_VAR(u32, kBRTNB_minAgeToIgnoreBlock_ms, "Behavior.ReactToBlock", 10000);
CONSOLE_VAR(u32, kBRTNB_minAgeToLookDown_ms, "Behavior.ReactToBlock", 700);
CONSOLE_VAR(f32, kBRTNB_lookDownHeadAngle_deg, "Behavior.ReactToBlock", -10.0f);
CONSOLE_VAR(f32, kBRTNB_lookUpHeadAngle_deg, "Behavior.ReactToBlock", 25.f);
CONSOLE_VAR(u32, kBRTNB_numImagesToWaitWhileLookingDown, "Behavior.ReactToBlock", 5);
CONSOLE_VAR(f32, kBRTNB_maxAgeToTrackWithHead_ms, "Behavior.ReactToBlock", 150);
CONSOLE_VAR(u32, kBRTNB_minTimeToSwitchBlocks_ms, "Behavior.ReactToBlock", 2000);
CONSOLE_VAR(u32, kBRTNB_waitForDispatchNumFrames, "Behavior.ReactToBlock", 3);
CONSOLE_VAR(f32, kBRTNB_maxHeightForPickupFromGround_mm, "Behavior.ReactToBlock", 30.f);
CONSOLE_VAR(f32, kBRTNB_maxHeightForTakeFromHand_mm, "Behavior.ReactToBlock", 70.f);
CONSOLE_VAR(f32, kBRTNB_maxHeightForStackedPickup_mm, "Behavior.ReactToBlock", 85.f);
CONSOLE_VAR(f32, kBRTNB_aboveOrBelowTol_mm, "Behavior.ReactToBlock", 30.f);   // large tolerance because we might not have a good pose yet

  
BehaviorReactToNewBlock::BehaviorReactToNewBlock(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("ReactToNewBlock");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotMarkedObjectPoseUnknown,
    EngineToGameTag::ObjectMoved,
  }});

}

Result BehaviorReactToNewBlock::InitInternal(Robot& robot)
{
  TransitionToDoingInitialReaction(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToNewBlock::StopInternal(Robot& robot)
{
  
}

bool BehaviorReactToNewBlock::IsRunnableInternal(const Robot& robot) const
{
  return _targetBlock.IsSet() && !robot.IsCarryingObject();
}

void BehaviorReactToNewBlock::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      break;
      
    case EngineToGameTag::ObjectMoved:
      HandleObjectChanged(robot, event.GetData().Get_ObjectMoved().objectID);
      break;
      
    case EngineToGameTag::RobotMarkedObjectPoseUnknown:
      HandleObjectChanged(robot, event.GetData().Get_RobotMarkedObjectPoseUnknown().objectID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorReactToNewBlock.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
}

void BehaviorReactToNewBlock::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
    {
      // If we are looking down and this is the object we are looking for, pick it up
      if(State::LookingDown == _state) {
        auto msg = event.GetData().Get_RobotObservedObject();
        if(msg.objectID == _targetBlock) {
          TransitionToPickingUp(robot);
        }
      }
      break;
    }
      
    default:
      // Nothing to do
      break;
  }
}

void BehaviorReactToNewBlock::HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  if( ! msg.markersVisible ) {
    return;
  }
  
  static const std::set<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };  
  if (familyList.count(msg.objectFamily) > 0)
  {
  
    // Always react to any block we "want" to pick up or that we have not yet reacted to
    bool wantToPickUp = false;
    CanPickUpBlock(robot, msg.objectID, wantToPickUp);
    
    if(wantToPickUp || _reactedBlocks.count(msg.objectID)==0)
    {
      // don't change target blocks unless we didn't have one before, or we haven't seen it in a while
      TimeStamp_t currTS = robot.GetLastMsgTimestamp();
      if( ! _targetBlock.IsSet() || currTS - _targetBlockLastSeen > kBRTNB_minTimeToSwitchBlocks_ms ) {     
        if( _targetBlock != msg.objectID ) {
          PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.UpdateTargetBlock",
                            "Setting target block to %d (was %d)",
                            msg.objectID,
                            _targetBlock.GetValue());
        }
        
        _targetBlock = msg.objectID;      
      }      
    }
  }
  
  if( _targetBlock == msg.objectID ) {
    _targetBlockLastSeen = msg.timestamp;
  }
}

  
void BehaviorReactToNewBlock::HandleObjectChanged(const Robot& robot, ObjectID objectID)
{
  if(objectID != _targetBlock &&
     objectID != robot.GetCarryingObject() &&
     objectID != robot.GetDockObject())
  {
    const size_t N = _reactedBlocks.erase(objectID);
    if(N > 0) {
      PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.HandleObjectChanged",
                        "Removing Object %d from reacted set because it moved or was marked unknown",
                        objectID.GetValue());
    }
  }
}


void BehaviorReactToNewBlock::TransitionToDoingInitialReaction(Robot& robot)
{
  SET_STATE(DoingInitialReaction);

  // First, always turns towards object to acknowledge it
  // Note: visually verify is true, so if we don't see the object anymore
  // this compound action will fail and we will not contionue this behavior.
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new TurnTowardsObjectAction(robot, _targetBlock, Radians(PI_F), true));

  // Then, three options
  // 1. If he can't reach the block or it's not pick-up-able, then ask for it
  // 2. If it's off the ground and he can pick it up, then play big react and go get it
  // 3. It's on the ground, so just react to it.

  bool wantToPickUp = false;
  const bool canPickUp = CanPickUpBlock(robot, _targetBlock, wantToPickUp);
  
  if(wantToPickUp)
  {
    if(canPickUp)
    {
      // React and then go get it!
      action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::ReactToNewBlockBig));
      StartActing(action, kBRTNB_ScoreIncreaseForReaction,
                  [this,&robot](ActionResult res) {
                    if(ActionResult::SUCCESS != res) {
                      _targetBlock.UnSet();
                    } else {
                      TransitionToPickingUp(robot);
                    }
                  });
    }
    else
    {
      // Ask for it and wait until we can pick it up
      action->AddAction(CreateAskForAction(robot));
                        
      StartActing(action, kBRTNB_ScoreIncreaseForReaction,
                  [this,&robot](ActionResult res) {
                    if(ActionResult::SUCCESS != res) {
                      _targetBlock.UnSet();
                    } else {
                      TransitionToAskingLoop(robot);
                    }
                  });
    }

  }
  else
  {
    // Simple case: not interested in picking up, so just react and move on
    action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::ReactToNewBlockSmall));
    
    StartActing(action, kBRTNB_ScoreIncreaseForReaction,
                [this](ActionResult res) {
                  if(ActionResult::SUCCESS == res) {
                    // we're done, just mark the block done and move on
                    _reactedBlocks.insert(_targetBlock);
                  }
                  // Whether or not we succeeded, unset the target block
                  _targetBlock.UnSet();
                });
  }

}



void BehaviorReactToNewBlock::TransitionToAskingLoop(Robot& robot)
{
  SET_STATE(AskingLoop);
  
  // Check if we can pick up the block from wherever it is and go grab it if so
  bool wantToPickUp = false;
  if( CanPickUpBlock(robot, _targetBlock, wantToPickUp) )
  {
    TransitionToPickingUp(robot);
    return;
  }

  // Check if the block should still be asked for
  if( !wantToPickUp ) {
    // re-start the state machine to handle this
    PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.BlockOnGround",
                      "block %d appears to have been put on the ground, reacting",
                      _targetBlock.GetValue());
    TransitionToDoingInitialReaction(robot);
    return;
  }

  ObservableObject* newBlock = robot.GetBlockWorld().GetObjectByID( _targetBlock );
  if( nullptr == newBlock ) {
    PRINT_NAMED_ERROR("BehaviorReactToNewBlock.NullBlock",
                      "couldnt get block %d",
                      _targetBlock.GetValue());
    _targetBlock.UnSet();
    return;
  }
  
  TimeStamp_t currTS = robot.GetLastImageTimeStamp();
  
  // if the block isn't moving, and we haven't seen it in a bit, look down
  if( ! newBlock->IsMoving() && currTS - _targetBlockLastSeen > kBRTNB_minAgeToLookDown_ms ) {

    static const float kHeadTolerance_deg = 5.0f;
    if( std::abs( robot.GetHeadAngle() - DEG_TO_RAD(kBRTNB_lookDownHeadAngle_deg) ) > DEG_TO_RAD(kHeadTolerance_deg) ) {
    
      PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.LookingDown",
                        "havent seen block and its not moving, so look down");
      
      _lookDownTime_ms = currTS;
      StartActing(new CompoundActionParallel(robot, {
                      new MoveHeadToAngleAction(robot, DEG_TO_RAD( kBRTNB_lookDownHeadAngle_deg )),
                      new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK)
                  }),
                  kBRTNB_ScoreIncreaseForReaction,
                  &BehaviorReactToNewBlock::TransitionToLookingDown);
      return;
    }
  }
  
  // check if it's been too long since we saw the block
  if( currTS - _targetBlockLastSeen > kBRTNB_minAgeToIgnoreBlock_ms ) {
    PRINT_NAMED_INFO("BehaviorReactToNewBlock.HaventSeenBlock",
                     "haven't seen block since ts %d (curr is %d), giving up",
                     _targetBlockLastSeen,
                     currTS);
    _targetBlock.UnSet();
    return;
  }
  
  // Note: don't increase score for the ask loop
  StartActing(CreateAskForAction(robot), &BehaviorReactToNewBlock::TransitionToAskingLoop);
  
} 

  
void BehaviorReactToNewBlock::TransitionToLookingDown(Robot& robot)
{
  SET_STATE(LookingDown);
  
  // Wait a few frames and then go back to asking. If the object is observed
  // during this, the HandleWhileRunning call will interrupt cause us to pick it up.
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    new WaitForImagesAction(robot, kBRTNB_numImagesToWaitWhileLookingDown, VisionMode::DetectingMarkers),
    new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBRTNB_lookUpHeadAngle_deg)),
  });
  
  StartActing(action, &BehaviorReactToNewBlock::TransitionToAskingLoop);
  
}
  
void BehaviorReactToNewBlock::TransitionToPickingUp(Robot& robot)
{
  SET_STATE(PickingUp);
  
  const Radians maxTurnToFaceAngle(DEG_TO_RAD_F32(30.f));
  const bool sayName = true;
  
  StartActing(new DriveToPickupObjectAction(robot, _targetBlock, false, 0, false,
                                            maxTurnToFaceAngle, sayName), kBRTNB_ScoreIncreaseForPickup,
              [this,&robot](const ExternalInterface::RobotCompletedAction& completionEvent) {
                if(ActionResult::SUCCESS == completionEvent.result)
                {
                  // Picked up block! Mark target block as reacted to, and unset target block,
                  // indicating this behavior is no longer runnable
                  _reactedBlocks.insert(_targetBlock);
                  _targetBlock.UnSet();
                  
                  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToBlockPickupSuccess),
                              kBRTNB_ScoreIncreaseForPickup);
                } else {
                  auto& completionInfo = completionEvent.completionInfo.Get_objectInteractionCompleted();
                  switch(completionInfo.result)
                  {
                    case ObjectInteractionResult::INCOMPLETE:
                    case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
                      // Did not start the pickup part of the action or failed to reach pre-action pose:
                      // don't play retry animation in this case.
                      // Target block will still be set.
                      break;
                      
                    default:
                      // Pickup failed, play retry animation, finish, and let behavior
                      // be selected again
                      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToBlockRetryPickup),
                                  kBRTNB_ScoreIncreaseForReaction);
                      break;
                  }
                }
              });
}
  
IActionRunner* BehaviorReactToNewBlock::CreateAskForAction(Robot& robot)
{
  CompoundActionParallel* moveHeadAndLift = new CompoundActionParallel(robot, {
    new MoveHeadToAngleAction(robot, 0),
    new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK),
  });
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    new TurnTowardsObjectAction(robot, _targetBlock, PI_F),                 // Look towards object
    new TriggerAnimationAction(robot, AnimationTrigger::ReactToNewBlockAsk), // Play "ask for" animation
    //new TriggerAnimationAction(robot, _askingLoopGroup),                  // Play "asksing" loop (?)
    moveHeadAndLift,                                                        // Make sure head and lift are down
    new WaitForImagesAction(robot, kBRTNB_numImagesToWaitWhileLookingDown, VisionMode::DetectingMarkers), // Wait for a few images to see the object
  });
  
  return action;
}

void BehaviorReactToNewBlock::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}


static bool HasPreActionPoses(const ObservableObject* object, const BlockWorld& blockWorld)
{
  std::vector<PreActionPose> preActionPoses;
  
  const ActionableObject* aObject = dynamic_cast<const ActionableObject*>(object);
  if(nullptr != aObject)
  {
    std::vector<std::pair<Quad2f, ObjectID> > obstacles;
    blockWorld.GetObstacles(obstacles);
    aObject->GetCurrentPreActionPoses(preActionPoses,
                                      {PreActionPose::DOCKING},
                                      std::set<Vision::Marker::Code>(),
                                      obstacles,
                                      nullptr);
  }
  
  if(DEBUG_TAKE_BLOCK_FROM_HAND) {
    PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.HasPreActionPoses",
                      "NumPreActionPoses:%zu", preActionPoses.size());
  }
  
  const bool hasPreActionPoses = !preActionPoses.empty();
  
  return hasPreActionPoses;
} // HasPreActionPoses()

  
bool BehaviorReactToNewBlock::CanPickUpBlock(const Robot& robot, ObjectID objectID, bool& wantToPickup)
{
  wantToPickup = false;
  
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(objectID);
  
  Pose3d poseWrtRobot;
  if(nullptr != object &&
     true == object->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot))
  {
    const bool isOnTopOfAnotherObject = robot.GetBlockWorld().FindObjectUnderneath(*object, kBRTNB_aboveOrBelowTol_mm) != nullptr;
    const f32 height_mm = poseWrtRobot.GetTranslation().z();
    
    const bool isTakeFromHandUnlocked = true; // TODO: robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::TakeFromHand);
    const bool isPickUpStackedBlockUnlocked = true; // TODO: robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::PickUpStackedBlock);
    
    if(height_mm <= kBRTNB_maxHeightForPickupFromGround_mm) {
      // "Can" always pick up a block from ground, but never "want" to.
      // NOTE: wantToPickup = false;
      return true;
    }
    else if(!isOnTopOfAnotherObject && isTakeFromHandUnlocked) {
      // Object is not on ground, not on top of another object, and we are allowed
      // to take from hand: we want to pick this up. Whether we can or not is
      // dependent on whether its height is low enough
      wantToPickup = true;
      return (height_mm <= kBRTNB_maxHeightForTakeFromHand_mm) && HasPreActionPoses(object, robot.GetBlockWorld());
    }
    else if(isOnTopOfAnotherObject && isPickUpStackedBlockUnlocked &&
            _reactedBlocks.count(object->GetID())) {
      // Object is stacked and we're allowed to pick up stacked blocks and we haven't
      // already reacted to this block: we want to pick this up. Whether we can
      // or not is dependent on its height
      wantToPickup = true;
      return (height_mm <= kBRTNB_maxHeightForStackedPickup_mm) && HasPreActionPoses(object, robot.GetBlockWorld());
    }
  }
  
  return false;
}


} // namespace Cozmo
} // namespace Anki

