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
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

#define DEBUG_STACK_CHECKING 0
#define DEBUG_TAKE_BLOCK_FROM_HAND 0

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(f32, kBRTNB_ScoreIncreaseForReaction, "Behavior.ReactToBlock", 0.3f);
CONSOLE_VAR(f32, kBRTNB_ScoreIncreaseForPickup, "Behavior.ReactToBlock", 0.8f);
CONSOLE_VAR(f32, kBRTNB_MinHeightForBigReact_mm, "Behavior.ReactToBlock", 40.0f);
CONSOLE_VAR(u32, kBRTNB_minAgeToIgnoreBlock_ms, "Behavior.ReactToBlock", 10000);
CONSOLE_VAR(u32, kBRTNB_minAgeToLookDown_ms, "Behavior.ReactToBlock", 700);
CONSOLE_VAR(f32, kBRTNB_lookDownHeadAngle_deg, "Behavior.ReactToBlock", -10.0f);
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
  }});

}

Result BehaviorReactToNewBlock::InitInternal(Robot& robot)
{
  TransitionToDoingInitialReaction(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToNewBlock::StopInternal(Robot& robot)
{
  if( _state == State::AskingLoop ) {
    // if we get interrupted while we were acting, then we are done reacting to the given block
    _reactedBlocks.insert(_targetBlock);
    _targetBlock.UnSet();
  }
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
      
    case EngineToGameTag::RobotMarkedObjectPoseUnknown:
      HandleObjectChanged(event.GetData().Get_RobotMarkedObjectPoseUnknown().objectID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorReactToNewBlock.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
}

void BehaviorReactToNewBlock::HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  if( ! msg.markersVisible ) {
    return;
  }
  
  static const std::set<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };  
  if (familyList.count(msg.objectFamily) > 0) {

    const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID( msg.objectID );
    if( nullptr == obj ) {
      PRINT_NAMED_ERROR("BehaviorReactToNewBlock.HandleObjectObserved.NullObject",
                        "Robot observed object %d, but can't get pointer",
                        msg.objectID);
      return;
    }
    
    // always react to a high block or a block we haven't seen that's on the ground
    if( ShouldAskForBlock(robot, msg.objectID) ||
        ( _reactedBlocks.count( msg.objectID ) == 0 && robot.CanPickUpObjectFromGround( *obj ) ) ) {

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

  
void BehaviorReactToNewBlock::HandleObjectChanged(ObjectID objectID)
{
  if(objectID != _targetBlock)
  {
    size_t N = _reactedBlocks.erase(objectID);
    if(N > 0) {
      PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.HandleObjectChanged",
                        "Removing Object %d from reacted set because it moved",
                        objectID.GetValue());
    }
  }
}
  
  
bool BehaviorReactToNewBlock::ShouldAskForBlock(const Robot& robot, ObjectID blockID)
{
  
  const ObservableObject* block = robot.GetBlockWorld().GetObjectByID( blockID );
  if( nullptr == block ) {
    PRINT_NAMED_ERROR("BehaviorReactToNewBlock.NullBlock",
                      "couldnt get block %d",
                      blockID.GetValue());
    return false;
  }

  Pose3d relPos;
  if ( !block->GetPose().GetWithRespectTo(robot.GetPose(), relPos) ) {
    PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.ShouldAskForBlock.NoPoseWRTRobot",
                      "couldnt get pose with respect to robot for block %d",
                      blockID.GetValue());
    return false;
  }

  const float topZ = relPos.GetTranslation().z();

  if( topZ > kBRTNB_MinHeightForBigReact_mm ) {
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({{ObjectFamily::Block, ObjectFamily::LightCube}});
    const bool hasObjectUnderneath = robot.GetBlockWorld().FindObjectUnderneath(*block, kBRTNB_aboveOrBelowTol_mm, filter);
    return !hasObjectUnderneath;
  }
  else {
    // block is too low to request
    return false;
  }
}


void BehaviorReactToNewBlock::TransitionToDoingInitialReaction(Robot& robot)
{
  SET_STATE(DoingInitialReaction);

  CompoundActionSequential* action = new CompoundActionSequential(robot);

  action->AddAction(new TurnTowardsObjectAction(robot, _targetBlock, PI_F));

  ObservableObject* newBlock = robot.GetBlockWorld().GetObjectByID( _targetBlock );
  if( nullptr == newBlock ) {
    PRINT_NAMED_ERROR("BehaviorReactToNewBlock.NullBlock",
                      "couldnt get block %d",
                      _targetBlock.GetValue());
    _targetBlock.UnSet();
    return;
  }

  Pose3d relPos;
  if ( !newBlock->GetPose().GetWithRespectTo(robot.GetPose(), relPos) ) {
    PRINT_NAMED_WARNING("BehaviorReactToNewBlock.NoPoseWRTRobot",
                        "couldnt get pose with respect to robot for block %d",
                        _targetBlock.GetValue());
    _targetBlock.UnSet();
    return;
  }

  const float topZ = relPos.GetTranslation().z();  

  if(topZ > kBRTNB_MinHeightForBigReact_mm) {
    action->AddAction(new PlayAnimationGroupAction(robot, _bigReactAnimGroup));
    // look back at the object after the big reaction
    action->AddAction(new TurnTowardsObjectAction(robot, _targetBlock, PI_F));
  }
  else {
    // do a smaller reaction for blocks which are low
    action->AddAction(new PlayAnimationGroupAction(robot, _smallReactAnimGroup));    
  }

  action->AddAction(new WaitForImagesAction(robot, kBRTNB_waitForDispatchNumFrames));

  StartActing(action, [this,&robot](ActionResult res) {
      if( res == ActionResult::SUCCESS ) {
        TransitionToPostReactionDispatch(robot);
      }});

  IncreaseScoreWhileActing( kBRTNB_ScoreIncreaseForReaction );
}

void BehaviorReactToNewBlock::TransitionToPostReactionDispatch(Robot& robot)
{
  SET_STATE(PostReactionDispatch);
  
  if( ShouldAskForBlock( robot, _targetBlock ) ) {
    TransitionToAskingForBlock(robot);
  }
  else {
    // we're done, just mark the block done and move on
    _reactedBlocks.insert(_targetBlock);
    _targetBlock.UnSet();
  }
}

void BehaviorReactToNewBlock::TransitionToAskingForBlock(Robot& robot)
{
  SET_STATE(AskingForBlock);

  StartActing(new PlayAnimationGroupAction(robot, _askForBlockAnimGroup),
              &BehaviorReactToNewBlock::TransitionToAskingLoop);
  IncreaseScoreWhileActing( kBRTNB_ScoreIncreaseForReaction );
}

void BehaviorReactToNewBlock::TransitionToAskingLoop(Robot& robot)
{
  SET_STATE(AskingLoop);

  ObservableObject* newBlock = robot.GetBlockWorld().GetObjectByID( _targetBlock );
  if( nullptr == newBlock ) {
    PRINT_NAMED_ERROR("BehaviorReactToNewBlock.NullBlock",
                      "couldnt get block %d",
                      _targetBlock.GetValue());
    _targetBlock.UnSet();
    return;
  }
  
  // Check if we can pick up the block from wherever it is and go grab it if so
  if( CanPickUpBlock(robot, newBlock) )
  {
    StartActing(new PickupObjectAction(robot, _targetBlock), kBRTNB_ScoreIncreaseForPickup,
                [this,&robot](ActionResult res) {
                  if(ActionResult::SUCCESS == res) {
                    // Picked up block! Unset target block, indicating this behavior is
                    // no longer runnable
                    _targetBlock.UnSet();
                    
                    StartActing(new PlayAnimationGroupAction(robot, _pickupSuccessAnimGroup),
                                kBRTNB_ScoreIncreaseForPickup);
                    
                  } else {
                    // Pickup failed, play retry animation, finish, and let behavior
                    // be selected again
                    StartActing(new PlayAnimationGroupAction(robot, _retryPickeupAnimGroup),
                                kBRTNB_ScoreIncreaseForPickup);
                  }
                });
    return;
  }

  // Check if the block should still be asked for
  if( ! ShouldAskForBlock(robot, _targetBlock) ) {
    // re-start the state machine to handle this
    PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.BlockOnGround",
                      "block %d appears to have been put on the ground, reacting",
                      _targetBlock.GetValue());
    TransitionToDoingInitialReaction(robot);
    return;
  }

  TimeStamp_t currTS = robot.GetLastImageTimeStamp();
  
  // if the block isn't moving, and we haven't seen it in a bit, look down
  if( ! newBlock->IsMoving() && currTS - _targetBlockLastSeen > kBRTNB_minAgeToLookDown_ms ) {

    static const float kHeadTolerance_deg = 5.0f;
    if( std::abs( robot.GetHeadAngle() - DEG_TO_RAD(kBRTNB_lookDownHeadAngle_deg) ) > DEG_TO_RAD(kHeadTolerance_deg) ) {
    
      PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.LookingDown",
                        "havent seen block and its not moving, so look down");

      StartActing(new CompoundActionSequential(robot, {
            new MoveHeadToAngleAction(robot, DEG_TO_RAD( kBRTNB_lookDownHeadAngle_deg )),
            new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::OUT_OF_FOV) }),
                  &BehaviorReactToNewBlock::TransitionToAskingLoop);
      return;
    }
    // else we are already looking down
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
  
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  const bool doTurnAction = currTS - _targetBlockLastSeen < kBRTNB_maxAgeToTrackWithHead_ms;
  
  if( doTurnAction ) {
    action->AddAction( new TurnTowardsObjectAction(robot, _targetBlock, PI_F) );
  }
  
  action->AddAction( new PlayAnimationGroupAction(robot, _askingLoopGroup) );

  // do another turn after the animation is done as well
  if( doTurnAction ) {
    action->AddAction( new TurnTowardsObjectAction(robot, _targetBlock, PI_F) );
  }

  StartActing(action, [this](Robot& robot) {
      // check if the lift might be blocking the camera
      // for now, just always lower it if the head angle is low. Ideally we'd do a real FOV check
      const float minHeadAngleToMoveLift_deg = 15.0f;
      if( robot.GetHeadAngle() < DEG_TO_RAD( minHeadAngleToMoveLift_deg ) ) {
        StartActing(new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK),
                    &BehaviorReactToNewBlock::TransitionToAskingLoop);
      }
      else {
        TransitionToAskingLoop(robot);
      }
    });
  // don't increase score for the ask loop
} 

void BehaviorReactToNewBlock::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorReactToNewBlock.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}

static bool IsHeightValid(const Robot& robot, f32 height_mm, bool isOnTopOfAnotherObject)
{
  // Make sure the object's height is ok:
  // 1. It's on (or near) the ground
  // 2. It's off the ground but not too high, not on top of another object, and "TakeBlockFromHand" is unlocked
  // 3. It's on top of another object and "PickUpStackedBlock" is unlocked
  
  const bool isTakeFromHandUnlocked = true; // TODO: robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::TakeFromHand);
  const bool isPickUpStackedBlockUnlocked = true; // TODO: robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::PickUpStackedBlock);
  
  if(height_mm <= kBRTNB_maxHeightForPickupFromGround_mm) {
    return true;
  }
  else if(!isOnTopOfAnotherObject && height_mm <= kBRTNB_maxHeightForTakeFromHand_mm && isTakeFromHandUnlocked) {
    return true;
  }
  else if(isOnTopOfAnotherObject && height_mm <= kBRTNB_maxHeightForStackedPickup_mm && isPickUpStackedBlockUnlocked) {
    return true;
  }
  
  return false;
} // IsHeightValid()


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

  
bool BehaviorReactToNewBlock::CanPickUpBlock(const Robot& robot, const ObservableObject* object)
{
  if(nullptr == object) {
    return false;
  }
  
  Pose3d poseWrtRobot;
  if(true == object->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot))
  {
    const bool isOnTopOfAnotherBlock = robot.GetBlockWorld().FindObjectUnderneath(*object, kBRTNB_aboveOrBelowTol_mm) != nullptr;
    const f32 height_mm = poseWrtRobot.GetTranslation().z();
    if(IsHeightValid(robot, height_mm, isOnTopOfAnotherBlock) &&
       HasPreActionPoses(object, robot.GetBlockWorld()))
    {
      return true;
    }
  }
  
  return false;
}


} // namespace Cozmo
} // namespace Anki

