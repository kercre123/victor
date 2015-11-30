/**
 * File: behaviorBlockPlay.cpp
 *
 * Author: Kevin Yoon
 * Date:   11/24/15
 *
 * Description: Implements a block play behavior
 *
 *              Init conditions: 
 *                - Cozmo sees a block that's above ground level and has very recently seen a face.
 *
 *              Behavior:
 *                - player presents block and cozmo tracks it
 *                - player places block on ground sideways
 *                - cozmo rolls it upright and it lights up
 *                - cozmo celebrates and picks up block
 *                - cozmo looks to player and begs for another block
 *                - player places block on ground upright
 *                - cozmo stacks carried block on top of it and checks out stack
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorBlockPlay.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#define DEBUG_BLOCK_PLAY_BEHAVIOR 0

namespace Anki {
namespace Cozmo {
  
  BehaviorBlockPlay::BehaviorBlockPlay(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::TrackingFace)
  , _lastHandlerResult(RESULT_OK)
  //, _inactionStartTime(0)
  , _faceID(Face::UnknownFace)
  , _hasValidLastKnownFacePose(false)
  {
    _name = "BlockPlay";
    
    SubscribeToTags({
      EngineToGameTag::RobotCompletedAction,
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedObject,
      EngineToGameTag::BlockPlaced,
      EngineToGameTag::RobotObservedFace,
      EngineToGameTag::RobotDeletedFace
    });

  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorBlockPlay::IsRunnable(const Robot& robot, double currentTime_sec) const
  {
    return (_faceID != Face::UnknownFace) || _trackedObject.IsSet() || _objectToPickUp.IsSet() || _objectToPlaceOn.IsSet();
  }
  
  Result BehaviorBlockPlay::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
  {
    Result lastResult = RESULT_OK;
    
    _interrupted = false;
    
    robot.GetActionList().Cancel();
    _animActionTags.clear();
    
    // Go to the appropriate state
    InitState(robot);
    
    return lastResult;
  } // Init()

  
  
  void BehaviorBlockPlay::InitState(const Robot& robot)
  {
    // Check if carrying a block
    if (robot.IsCarryingObject()) {
      if (_objectToPickUp != robot.GetCarryingObject()) {
        PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateState.UnexpectedCarryObject", "Expected %d, but carrying %d.",
                         _objectToPickUp.GetValue(), robot.GetCarryingObject().GetValue());
        _objectToPickUp = robot.GetCarryingObject();
      }
      
      // Check if there's a block to set it on
      if (_objectToPlaceOn.IsSet()) {
        SetCurrState(State::PlacingBlock);
      } else {
        SetCurrState(State::TrackingFace);
      }
    } else if (_objectToPickUp.IsSet()) {
      SetCurrState(State::RollingBlock);
    } else {
      SetCurrState(State::TrackingFace);
    }
  }
  
  IBehavior::Status BehaviorBlockPlay::UpdateInternal(Robot& robot, double currentTime_sec)
  {
    // Check to see if we had any problems with any handlers
    if(_lastHandlerResult != RESULT_OK) {
      PRINT_NAMED_ERROR("BehaviorBlockPlay.Update.HandlerFailure",
                        "Event handler failed, returning Status::FAILURE.");
      return Status::Failure;
    }
    
    if(_interrupted) {
      return Status::Complete;
    }
    
    if (_isActing || !_animActionTags.empty()) {
      return Status::Running;
    }
      
    switch(_currentState)
    {
      case State::TrackingFace:
      {
        // If not already tracking face...
        if (robot.GetMoveComponent().GetTrackToFace() == Face::UnknownFace) {
          
          if (_faceID != Face::UnknownFace) {
            
            // If we have a valid faceID, track it.
            robot.GetMoveComponent().EnableTrackToFace(_faceID, false);
            
          } else if (_hasValidLastKnownFacePose) {
            
            // Otherwise, if had previously seen a valid face, turn to face its last known pose
            robot.GetActionList().Cancel(_lastActionTag);
            IActionRunner* lookAtFaceAction = new FacePoseAction(_lastKnownFacePose, 0, PI_F);
            _lastActionTag = lookAtFaceAction->GetTag();
            robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, lookAtFaceAction);
            _isActing = true;
            
            _hasValidLastKnownFacePose = false;
          } else {
            
            // There's no face visible.
            // Wait a few seconds for a face to appear, otherwise quit.
            if (_noFacesStartTime == 0) {
              _noFacesStartTime = currentTime_sec;
            }
            if (currentTime_sec - _noFacesStartTime > 3) {
              PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.NoFacesSeen", "Aborting behavior");
              return Status::Complete;
            }
          }
        }

        break;
      }
      case State::TrackingBlock:
      {
        ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
        if (nullptr == obj) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: TrackingBlock");
          break;
        }
        
        // If block falls below certain angle, raise the block and look forward
        Vec3f diffVec = ComputeVectorBetween(obj->GetPose(), robot.GetPose());
        
        if (robot.IsCarryingObject()) {
          
          // Compute angle to object above horizon
          f32 horDistToBlock = sqrtf(diffVec.x()*diffVec.x() + diffVec.y()*diffVec.y());
          f32 viewAngle = atan2f(diffVec.z(), horDistToBlock);
          
          if (viewAngle < kTrackedObjectViewAngleThreshForRaisingCarriedBlock) {
            // Raise block and look straight ahead
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.RaisingBlockToSeeOtherBlock", "");
            robot.GetMoveComponent().MoveLiftToHeight(LIFT_HEIGHT_CARRY, 2, 5);
            robot.GetMoveComponent().MoveHeadToAngle(0, 2, 5);
          }
        }
        
        
        // Check that block is on the ground and not moving
        TimeStamp_t t;
        if (!obj->IsMoving(&t)) {
          // Check that is hasn't been moving for a few seconds
          s32 notMovingTime_ms = robot.GetLastMsgTimestamp() - t;
          if (notMovingTime_ms > 2000 &&
              diffVec.z() < 0.75 * obj->GetSize().z()) {
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.BlockOnGround", "State: TrackingBlock");
            SetCurrState(State::InspectingBlock);
            robot.GetMoveComponent().DisableTrackToObject();
          }
        }
        
        break;
      }
      case State::InspectingBlock:
      {
        ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
        if (nullptr == obj) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: InspectingBlock");
          SetCurrState(State::TrackingFace);
          break;
        }
        
        // Check if it's sideways
        AxisName upAxis = obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
        if (upAxis == AxisName::Z_POS) {
          if (robot.IsCarryingObject()) {
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.PlacingBlock", "");
            SetCurrState(State::PlacingBlock);
            _objectToPlaceOn = _trackedObject;
          } else {
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.PickingUpBlock", "");
            SetBlockLightState(robot, _trackedObject, BlockLightState::Upright);
            SetCurrState(State::PickingUpBlock);
            _objectToPickUp = _trackedObject;
          }

          _trackedObject.UnSet();
        } else {
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.RollingBlock", "UpAxis %d", upAxis);
          SetCurrState(State::RollingBlock);
        }
        
        break;
      }
      case State::RollingBlock:
      {
        // Verify that object still exists
        if (!_trackedObject.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: RollingBlock");
          SetCurrState(State::TrackingFace);
          break;
        }

        // Get world obstacles
        const BlockWorld& blockWorld = robot.GetBlockWorld();
        std::vector<std::pair<Quad2f, ObjectID> > obstacles;
        blockWorld.GetObstacles(obstacles);
        
        // Compute approach angle so that rolling rights the block
        ActionableObject* obj = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_trackedObject));
        std::vector<PreActionPose> preActionPoses;
        obj->GetCurrentPreActionPoses(preActionPoses,
                                      {PreActionPose::DOCKING},
                                      std::set<Vision::Marker::Code>(),
                                      obstacles);
        
        // Execute roll
        robot.GetActionList().Cancel(_lastActionTag);
        IActionRunner* rollAction = nullptr;
        if (preActionPoses.size() > 1) {
          // Block must be upside down so choose any roll action
          rollAction = new DriveToRollObjectAction(_trackedObject);
        } else {
          // Block is sideways so pick the approach angle from the 'docking' preActionPose
          // (The bottom of the cube is the only dockable side when the cube is sideways and also
          // the side from which it should be rolled to make the cube upright.)
          Vec3f approachVec = ComputeVectorBetween(obj->GetPose(), preActionPoses[0].GetPose());
          
          f32 approachAngle_rad = atan2f(approachVec.y(), approachVec.x());
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.ApproachForRoll", "%f rad", approachAngle_rad);
          rollAction = new DriveToRollObjectAction(_trackedObject,
                                                   DEFAULT_PATH_MOTION_PROFILE,
                                                   true,
                                                   approachAngle_rad);
        }
        _lastActionTag = rollAction->GetTag();
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, rollAction);
        _isActing = true;
        break;
      }
      case State::PickingUpBlock:
      {
        // Verify that object still exists
        if (!_objectToPickUp.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: PickingUpBlock");
          SetCurrState(State::TrackingFace);
          break;
        }
        
        robot.GetActionList().Cancel(_lastActionTag);
        IActionRunner* pickupAction = new DriveToPickupObjectAction(_objectToPickUp);
        _lastActionTag = pickupAction->GetTag();
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, pickupAction);
        _isActing = true;
        break;
      }
      case State::PlacingBlock:
      {
        // Verify that object still exists
        if (!_objectToPlaceOn.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: PlacingBlock");
          SetCurrState(State::TrackingFace);
          break;
        }
        
        robot.GetActionList().Cancel(_lastActionTag);
        IActionRunner* placementAction = new DriveToPlaceOnObjectAction(robot, _objectToPlaceOn);
        _lastActionTag = placementAction->GetTag();
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, placementAction);
        _isActing = true;
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("BehaviorBlockPlay.Update.UnknownState",
                          "Reached unknown state %d.", _currentState);
        return Status::Failure;
    }

    
    /*
    // Check if inactive for a while
    if (robot.GetActionList().IsEmpty()) {
      if (_inactionStartTime == 0) {
        _inactionStartTime = currentTime_sec;
      } else if (currentTime_sec - _inactionStartTime > kInactionFailsafeTimeoutSec) {
        PRINT_NAMED_WARNING("BehaviorBlockPlay.Update.InactionFailsafe", "Was doing nothing for some reason so starting a pick or place action");
        if (robot.IsCarryingObject()) {
          SelectNextPlacement(robot);
        } else {
          SelectNextObjectToPickUp(robot);
        }
        _inactionStartTime = 0;
      }
    }
     */
    
    
    return Status::Running;
  }

  Result BehaviorBlockPlay::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
  {
    _interrupted = true;
    return RESULT_OK;
  }
  
  void BehaviorBlockPlay::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
      case EngineToGameTag::RobotObservedObject:
        // Handled by other WhileRunning / WhileNotRunning
        break;
        
      case EngineToGameTag::RobotDeletedObject:
        _lastHandlerResult= HandleDeletedObject(event.GetData().Get_RobotDeletedObject(), event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotObservedFace:
        _lastHandlerResult = HandleObservedFace(robot, event.GetData().Get_RobotObservedFace(), event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotDeletedFace:
        _lastHandlerResult = HandleDeletedFace(event.GetData().Get_RobotDeletedFace());
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorBlockPlay.AlwaysHandle.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
  
  void BehaviorBlockPlay::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotDeletedObject:
      case EngineToGameTag::RobotObservedFace:
      case EngineToGameTag::RobotDeletedFace:
        // Handled by AlwaysHandle()
        break;
        
      case EngineToGameTag::RobotCompletedAction:
        _lastHandlerResult= HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction(),
                                                  event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult= HandleObservedObjectWhileRunning(robot, event.GetData().Get_RobotObservedObject(), event.GetCurrentTime());
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorBlockPlay.HandleWhileRunning.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
  
  // TODO: Get rid of this if it's doing nothing
  void BehaviorBlockPlay::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
      case EngineToGameTag::RobotObservedObject:
      case EngineToGameTag::RobotDeletedObject:
      case EngineToGameTag::BlockPlaced:
      case EngineToGameTag::RobotObservedFace:
      case EngineToGameTag::RobotDeletedFace:
        // Handled by AlwaysHandle() / HandleWhileRunning
        break;

      default:
        PRINT_NAMED_ERROR("BehaviorBlockPlay.HandleWhileRunning.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
    
  
  
#pragma mark -
#pragma mark BlockPlay-Specific Methods
  
  void BehaviorBlockPlay::SetCurrState(State s)
  {
    _currentState = s;
    
    _stateName = "";
    switch(_currentState)
    {
      case State::TrackingFace:
        _stateName += "-TRACKFACE";
        break;
      case State::TrackingBlock:
        _stateName += "-TRACKBLOCK";
        break;
      case State::InspectingBlock:
        _stateName += "-INSPECTING";
        break;
      case State::RollingBlock:
        _stateName += "-ROLLING";
        break;
      case State::PickingUpBlock:
        _stateName += "-PICKING";
        break;
      case State::PlacingBlock:
        _stateName += "-PLACING";
        break;        
      default:
        PRINT_NAMED_WARNING("BehaviorBlockPlay.SetCurrState.InvalidState", "");
    }
  }
  


  // TODO: Get color and on/off settings from config
  void BehaviorBlockPlay::SetBlockLightState(Robot& robot, const ObjectID& objID, BlockLightState state)
  {
    switch(state) {
      case BlockLightState::None:
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL, ::Anki::NamedColors::BLACK, ::Anki::NamedColors::BLACK, 10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      case BlockLightState::Visible:
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL, ::Anki::NamedColors::CYAN, ::Anki::NamedColors::BLACK, 10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      case BlockLightState::Upright:
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL, ::Anki::NamedColors::BLUE, ::Anki::NamedColors::BLACK, 200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      case BlockLightState::Complete:
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL, ::Anki::NamedColors::GREEN, ::Anki::NamedColors::GREEN, 200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      default:
        break;
    }
  }
  
  
  Result BehaviorBlockPlay::HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction &msg, double currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!IsRunning()) {
      // Ignore action completion messages while not running
      return lastResult;
    }
    
    // Check if this was an animation
    if (msg.actionType == RobotActionType::PLAY_ANIMATION)
    {
      if (_animActionTags.count(msg.idTag) > 0) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.AnimCompleted", "%s (result %d)", msg.completionInfo.animName.c_str(), msg.result);
        
        // Erase this animation action and resume pickOrPlace if there are no more animations pending
        _animActionTags.erase(msg.idTag);
        
      } else {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.UnknownAnimCompleted", "");
      }
    }
    
    // Delete anim action tag, in case we somehow missed it.
    _animActionTags.erase(msg.idTag);


    
    if (msg.idTag != _lastActionTag) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.HandleActionCompleted.ExternalAction", "Ignoring");
      return lastResult;
    }
    
    switch(_currentState)
    {
      case State::TrackingFace:
      case State::TrackingBlock:
      case State::InspectingBlock:
        _isActing = false;
        break;
      case State::RollingBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::ROLL_OBJECT_LOW:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.RollSuccessful", "");
              
              // We're done picking up the block
              SetCurrState(State::InspectingBlock);
              _isActing = false;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              
              PRINT_NAMED_ERROR("BehaviorBlockPlay.Rolling.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");

              // We failed to pick up or place the last block, try again
              SetCurrState(State::RollingBlock);
              _isActing = false;
              break;
              
            default:
              // Simply ignore other action completions?
              break;
              
          } // switch(actionType)
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.PickupFailure", "Trying again");
          // We failed to pick up or place the last block, try again
          PlayAnimation(robot, "Demo_OCD_PickUp_Fail");

          SetCurrState(State::RollingBlock);
          _isActing = false;
        }
        break;
      }
      case State::PickingUpBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PICKUP_OBJECT_LOW:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.PickupSuccessful", "");
              
              // We're done picking up the block.
              // Lower lift so that we can see the player.
              robot.GetMoveComponent().MoveLiftToHeight(kLowCarryHeightMM, 2, 5);
              SetCurrState(State::TrackingFace);
              _isActing = false;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              
              PRINT_NAMED_ERROR("BehaviorBlockPlay.PickingUp.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");
              
              // We failed to pick up or place the last block, try again
              SetCurrState(State::PickingUpBlock);
              _isActing = false;
              break;

            default:
              // Simply ignore other action completions?
              break;
              
          } // switch(actionType)
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.PickupFailure", "Trying again");
          // We failed to pick up or place the last block, try again
          
          // This isn't foolproof, but use lift height to check if this failure occured
          // during pickup verification.
          if (robot.GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK) {
            PlayAnimation(robot, "Demo_OCD_PickUp_Fail");
          } else {
            SetCurrState(State::PickingUpBlock);
            _isActing = false;
          }
        }
        break;
      } // case PickingUpBlock
        
      case State::PlacingBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PLACE_OBJECT_HIGH:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.PlacementSuccessful",
                               "Placed object %d on object %d",
                               _objectToPickUp.GetValue(), _objectToPlaceOn.GetValue());
              
              SetBlockLightState(robot, _objectToPlaceOn, BlockLightState::Complete);
              SetBlockLightState(robot, _objectToPickUp, BlockLightState::Complete);
              
              PlayAnimation(robot, "Demo_OCD_All_Blocks_Neat_Celebration");
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              PRINT_NAMED_ERROR("BehaviorBlockPlay.Placing.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");
              
              // We failed to place the last block, try again?
              SetCurrState(State::PlacingBlock);
              _isActing = false;
              break;
              
            default:
              // Simply ignore other action completions?
              break;
          }
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.PlacementFailure", "Trying again");
          // We failed to place the last block, try again
          SetCurrState(State::PlacingBlock);
          _isActing = false;
        }
        break;
      } // case PlacingBlock
      default:
        break;
    } // switch(_currentState)
    
    
    return lastResult;
  } // HandleActionCompleted()
  

  
  Result BehaviorBlockPlay::HandleObservedObjectWhileRunning(Robot& robot, const ExternalInterface::RobotObservedObject &msg, double currentTime_sec)
  {
    ObjectID objectID;
    objectID = msg.objectID;
    
    // Make sure this is actually a block
    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
    if (nullptr == oObject) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandeObservedObject.InvalidObject", "How'd this happen? (ObjectID %d)", objectID.GetValue());
      return RESULT_OK;
    }
    
    if(&oObject->GetPose().FindOrigin() != robot.GetWorldOrigin()) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandleObservedObject.OriginMismatch",
                          "Ignoring object %d because it does not share an origin "
                          "with the robot.", oObject->GetID().GetValue());
      return RESULT_OK;
    }
    
    if(oObject->IsActive() && oObject->GetIdentityState() != ActiveIdentityState::Identified) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandleObservedObject.UnidentifiedActiveObject",
                          "How'd this happen? (ObjectID %d, idState=%s)",
                          objectID.GetValue(), EnumToString(oObject->GetIdentityState()));
      return RESULT_OK;
    }
    
    // Only care about light cubes
    if (oObject->GetFamily() != ObjectFamily::LightCube) {
      return RESULT_OK;
    }
    

    // Get height of the object.
    // Only track the block if it's above a certain height.
    Vec3f diffVec = ComputeVectorBetween(oObject->GetPose(), robot.GetPose());
    
    // If this is observed while tracking face, then switch to tracking this object
    if ((_currentState == State::TrackingFace) &&
        (robot.GetMoveComponent().GetTrackToFace() != Face::UnknownFace) &&
        (diffVec.z() > oObject->GetSize().z()) ) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.HandleObservedObject.TrackingBlock", "Now tracking object %d", objectID.GetValue());
      _trackedObject = objectID;
      SetCurrState(State::TrackingBlock);
      robot.GetMoveComponent().DisableTrackToFace();
      robot.GetMoveComponent().EnableTrackToObject(_trackedObject, false);
      SetBlockLightState(robot, _trackedObject, BlockLightState::Visible);
      PlayAnimation(robot, "Demo_Look_Around_See_Something_A");
    }

    return RESULT_OK;
  }
  

  
  Result BehaviorBlockPlay::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg, double currentTime_sec)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;
    
    if(_trackedObject == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject", "About to delete tracked object, clearing.");
      _trackedObject.UnSet();
    }
    
    if(_objectToPickUp == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject", "About to delete object to pickup, clearing.");
      _objectToPickUp.UnSet();
    }

    if(_objectToPlaceOn == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject", "About to delete object to place on, clearing.");
      _objectToPlaceOn.UnSet();
    }
    
    
    return RESULT_OK;
  }
  
  
  Result BehaviorBlockPlay::HandleObservedFace(const Robot& robot, const ExternalInterface::RobotObservedFace& msg, double currentTime_sec)
  {
    // If faceID not already set or we're not currently tracking the update the faceID
    if (_faceID == Face::UnknownFace) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleObservedFace.UpdatingFaceToTrack", "id = %lld", msg.faceID);
      _faceID = static_cast<Face::ID_t>(msg.faceID);
    } else if (_faceID == robot.GetMoveComponent().GetTrackToFace() ) {
      // Currently tracking a face, keep track of last known position
      _lastKnownFacePose = robot.GetPose();
      _lastKnownFacePose.SetTranslation({msg.world_x, msg.world_y, msg.world_z});
      _hasValidLastKnownFacePose = true;
    }
    
    return RESULT_OK;
  }
  
  Result BehaviorBlockPlay::HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg)
  {
    if (_faceID == msg.faceID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedFace", "id = %lld", msg.faceID);
      _faceID = Face::UnknownFace;
    }
    
    return RESULT_OK;
  }
  
  
  void BehaviorBlockPlay::PlayAnimation(Robot& robot, const std::string& animName)
  {
    // Check if animation is already being played
    for (auto& animTagNamePair : _animActionTags) {
      if (animTagNamePair.second == animName) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.PlayAnimation.Ignoring", "%s already playing", animName.c_str());
        return;
      }
    }
    
    BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.PlayAnimation", "%s", animName.c_str());
    PlayAnimationAction* animAction = new PlayAnimationAction(animName.c_str());
    _animActionTags[animAction->GetTag()] = animName;
    robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, animAction);
  }
  


  
  
} // namespace Cozmo
} // namespace Anki
