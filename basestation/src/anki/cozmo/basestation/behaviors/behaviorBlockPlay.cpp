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

#define DEBUG_BLOCK_PLAY_BEHAVIOR 1

// TODO:(bn) when driving to the side or back of the cube to roll it, the position is really far off. Instead
// of relying on the action (or maybe add this logic within the action?) plan to a pre-dock pose which is much
// further away from the block, so it can adjust more naturally instead of having to back up and do weird
// stuff

// TODO:(bn) tune speed, so maybe we can move faster and make everything look better. Ideally, this whole
// behavior could have a single parameter for this

// TODO:(bn) remove face tracking as first step, and integrate this behavior with the follow faces behavior
// TODO:(bn) make sure we can go back and forth at the various staged between these behaviors

// TODO:(bn) need to apply motion profile to turn in place and face pose action???


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

    // start with defaults
    _motionProfile = DEFAULT_PATH_MOTION_PROFILE;
    
    _motionProfile.speed_mmps = 60.0f;
    _motionProfile.accel_mmps2 = 200.0f;
    _motionProfile.decel_mmps2 = 200.0f;
    _motionProfile.pointTurnSpeed_rad_per_sec = 2.0f;
    _motionProfile.pointTurnAccel_rad_per_sec2 = 100.0f;
    _motionProfile.pointTurnDecel_rad_per_sec2 = 100.0f;
    _motionProfile.dockSpeed_mmps = 80.0f; // slow it down a bit for reliability
    
    SubscribeToTags({{
      EngineToGameTag::RobotCompletedAction,
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedObject,
      EngineToGameTag::BlockPlaced,
      EngineToGameTag::RobotObservedFace,
      EngineToGameTag::RobotDeletedFace
    }});


  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorBlockPlay::IsRunnable(const Robot& robot, double currentTime_sec) const
  {
    return _trackedObject.IsSet() || _objectToPickUp.IsSet() || _objectToPlaceOn.IsSet() ||
      (_currentState == State::TrackingFace && _faceID != Face::UnknownFace && robot.IsCarryingObject());
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

    // hack to track object motion
    if( _trackedObject.IsSet() ) {
      ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
      if (nullptr != obj) {
        if( obj->IsMoving() ) {
          _trackedObjectStoppedMovingTime = -1.0f;
        }
        else if( _trackedObjectStoppedMovingTime < 0 ) {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.BlockStoppedMoving",
                                 "tracked block stopped moving at t=%f",
                                 currentTime_sec);
          _trackedObjectStoppedMovingTime = currentTime_sec;
        }
      }
    }

    

    if( _holdUntilTime > 0.0f) {
      if( currentTime_sec < _holdUntilTime ) {
        return Status::Running;
      }
      else {
        _holdUntilTime = -1.0f;
      }
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
          
          // if we are carrying an object, make sure the lift is out of the way. Don't execute the action yet
          // because we may want to do it in parallel
          MoveLiftToHeightAction* moveLiftAction = nullptr;

          const float lowCarry = kLowCarryHeightMM; //  + 10;
          
          if( robot.IsCarryingObject() && robot.GetLiftHeight() > lowCarry + 5 && !robot.IsLiftMoving() ) {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace",
                                   "block in lift is blocking view, moving from %f to %f",
                                   robot.GetLiftHeight(),
                                   lowCarry);

            moveLiftAction = new MoveLiftToHeightAction(lowCarry);
            // move slowly
            moveLiftAction->SetDuration(1.0f);
          }

          if (_faceID != Face::UnknownFace) {
            
            // If we have a valid faceID, track it.
            robot.GetMoveComponent().EnableTrackToFace(_faceID, false);
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace.Enabled",
                                   "EnableTrackToFace %lld", _faceID);
            
          } else if (_hasValidLastKnownFacePose) {
            
            // Otherwise, if had previously seen a valid face, turn to face its last known pose
            robot.GetActionList().Cancel(_lastActionTag);
            IActionRunner* lookAtFaceAction = new FacePoseAction(_lastKnownFacePose, DEG_TO_RAD(5), PI_F);

            // if we also need to lower the lift, do it in parallel (this is the most common case)
            if( moveLiftAction != nullptr) {
              StartActing(robot, new CompoundActionParallel({moveLiftAction, lookAtFaceAction}));
              moveLiftAction = nullptr;
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace.FindingOldFace",
                                     "Moving to last face pose AND lowering lift");
            }
            else {
              // otherwise just execute the look action
              StartActing(robot, lookAtFaceAction);
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace.FindingOldFace",
                                     "Moving to last face pose");
            }
            
            _hasValidLastKnownFacePose = false;
          } else {

            // There's no face visible.
            // Wait a few seconds for a face to appear, otherwise quit.
            if (_noFacesStartTime <= 0) {
              _noFacesStartTime = currentTime_sec;
            }

            // HACK: don't abort if we are carrying an object, because we might get stuck
            if (currentTime_sec - _noFacesStartTime > 2.0f && !robot.IsCarryingObject()) {
              PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.NoFacesSeen", "Aborting behavior");
              return Status::Complete;
            }
          }

          // TEMP: what I really need is a "search for faces" beahvior, instead of look around. It would
          // always be runnable. Then when we "abort" this behavior, we could go to that one

          if( moveLiftAction != nullptr) {
            // execute the lower lift action before we start tracking faces
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace.LowerLift",
                                   "executing lift lowering action");

            StartActing(robot, moveLiftAction);
            break;
          }
        }
        else if( robot.GetMoveComponent().GetTrackToFace() != _faceID ) {
          PRINT_NAMED_INFO("BehaviorBlockPlay.TrackingWrongFace",
                           "Disabling face tracking because we aren't tracking the correct face (or it was deleted)");
          robot.GetMoveComponent().DisableTrackToFace();
        }

        break;
      }
      case State::TrackingBlock:
      {

        if (!_trackedObject.IsSet()) {
          if( robot.GetMoveComponent().GetTrackToObject().IsSet() ) {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackingBlockUnset",
                                   "disabling object tracking because object was deleted / unset");
            robot.GetMoveComponent().DisableTrackToObject();
          }
          _faceID = Face::UnknownFace;
          SetCurrState(State::TrackingFace);
          break;
        }

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
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.RaisingBlockToSeeOtherBlock",
                             "trying to see block %d",
                             _trackedObject.GetValue());
            StartActing(robot,
                        new CompoundActionParallel({
                            new MoveLiftToHeightAction( LIFT_HEIGHT_CARRY ),
                            new MoveHeadToAngleAction( 0 ) }) );
            // robot.GetMoveComponent().MoveLiftToHeight(LIFT_HEIGHT_CARRY, 2, 5);
            // robot.GetMoveComponent().MoveHeadToAngle(0, 2, 5);
          }
        }
        
        // Check that block is on the ground and not moving
        if( currentTime_sec > _trackedObjectStoppedMovingTime + 0.75f &&
            diffVec.z() < 0.75 * obj->GetSize().z()) {
          
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.BlockOnGround", "State: TrackingBlock");
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StopTrackingBlock",
                                 "disabling block tracking in order to inspect block");
          SetCurrState(State::InspectingBlock);
          robot.GetMoveComponent().DisableTrackToObject();
          PlayAnimation(robot, "ID_react2block_02", false);

          // hold a bit before making a decision about the block
          const float inspectTime = 0.3f;
          _holdUntilTime = currentTime_sec + inspectTime;
        }
        
        break;
      }
      case State::InspectingBlock:
      {
        ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
        if (nullptr == obj) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: InspectingBlock");
          _faceID = Face::UnknownFace;
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
        } else {
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.RollingBlock", "UpAxis %c", AxisToChar(upAxis));
          SetCurrState(State::RollingBlock);
        }
        
        break;
      }
      case State::RollingBlock:
      {
        // Verify that object still exists
        if (!_trackedObject.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: RollingBlock");
          _faceID = Face::UnknownFace;
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

        if( preActionPoses.empty() ) {
          // no valid actions, just wait a bit, hope we get them next time
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.NoPredockPoses",
                           "No valid pre-dock poses for object %d, waiting",
                           _trackedObject.GetValue());
          break;
        }
        
        // Execute roll
        robot.GetActionList().Cancel(_lastActionTag);
        IActionRunner* rollAction = nullptr;
        if (preActionPoses.size() > 1) {
          // Block must be upside down so choose any roll action
          rollAction = new DriveToRollObjectAction(_trackedObject, _motionProfile);
        } else {
          // Block is sideways so pick the approach angle from the 'docking' preActionPose
          // (The bottom of the cube is the only dockable side when the cube is sideways and also
          // the side from which it should be rolled to make the cube upright.)
          Vec3f approachVec = ComputeVectorBetween(obj->GetPose(), preActionPoses[0].GetPose());
          
          f32 approachAngle_rad = atan2f(approachVec.y(), approachVec.x());
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.ApproachForRoll", "%f rad", approachAngle_rad);
          rollAction = new DriveToRollObjectAction(_trackedObject,
                                                   _motionProfile,
                                                   true,
                                                   approachAngle_rad);
        }
        StartActing(robot, rollAction);
        break;
      }
      case State::PickingUpBlock:
      {
        // Verify that object still exists
        if (!_objectToPickUp.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: PickingUpBlock");
          _faceID = Face::UnknownFace;
          SetCurrState(State::TrackingFace);
          break;
        }
        
        robot.GetActionList().Cancel(_lastActionTag);
        StartActing(robot, new DriveToPickupObjectAction(_objectToPickUp, _motionProfile));
        break;
      }
      case State::PlacingBlock:
      {
        // Verify that object still exists
        if (!_objectToPlaceOn.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: PlacingBlock");
          _faceID = Face::UnknownFace;
          SetCurrState(State::TrackingFace);
          break;
        }
        
        robot.GetActionList().Cancel(_lastActionTag);
        StartActing(robot, new DriveToPlaceOnObjectAction(robot, _objectToPlaceOn, _motionProfile));
        break;
      }
      case State::SearchingForMissingBlock:
      {
        // TODO:(bn) add a "huh, where's the block?" animation here

        // first face the block object (where we think it is, if we know), then look a bit to the left and
        // right

        const float lookAmountRads = DEG_TO_RAD(20);
        const float waitTime = 0.6f;
        
        CompoundActionSequential* searchAction = new CompoundActionSequential();
        searchAction->SetDelayBetweenActions(waitTime);

        if( _trackedObject.IsSet() ) {
          ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
          if( obj != nullptr &&
              obj->IsExistenceConfirmed() ) {

            searchAction->AddAction( new FacePoseAction(obj->GetPose(), DEG_TO_RAD(5), 0.5 * PI_F) );
          }
        }

        TurnInPlaceAction* turnAction = new TurnInPlaceAction( -lookAmountRads, false );
        turnAction->SetTolerance(DEG_TO_RAD(2));
        searchAction->AddAction( turnAction );

        turnAction = new TurnInPlaceAction( 2 * lookAmountRads, false );
        turnAction->SetTolerance(DEG_TO_RAD(2));
        searchAction->AddAction( turnAction );

        searchAction->AddAction( new WaitAction(0.2f) );

        StartActing(robot, searchAction);

        // mark the time the search started, so if we see something after this time we count it.  Add a bit of
        // a fudge factor so we don't count things that we are seeing right now
        _searchStartTime = currentTime_sec + _objectObservedTimeThreshold;

        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                               "BehaviorBlockPlay.StartSearchingForObject",
                               "t must be >= %f",
                               _searchStartTime);

        break;
      }
      
      case State::Complete:
      {
        // TODO:(bn) fade lights out??
        SetBlockLightState(robot, _objectToPlaceOn, BlockLightState::None);
        SetBlockLightState(robot, _objectToPickUp, BlockLightState::None);
        return Status::Complete;
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
        _lastHandlerResult = HandleDeletedObject(event.GetData().Get_RobotDeletedObject(), event.GetCurrentTime());
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
        _lastHandlerResult = HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction(),
                                                  event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult = HandleObservedObjectWhileRunning(robot,
                                                              event.GetData().Get_RobotObservedObject(),
                                                              event.GetCurrentTime());
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
      case EngineToGameTag::RobotDeletedObject:
      case EngineToGameTag::BlockPlaced:
      case EngineToGameTag::RobotObservedFace:
      case EngineToGameTag::RobotDeletedFace:
        // Handled by AlwaysHandle() / HandleWhileRunning
        break;

      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult = HandleObservedObjectWhileNotRunning(robot,
                                                                 event.GetData().Get_RobotObservedObject(),
                                                                 event.GetCurrentTime());
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
        _stateName += "FACETRACK";
        break;
      case State::TrackingBlock:
        _stateName += "BLOCKTRACK";
        break;
      case State::InspectingBlock:
        _stateName += "INSPECTING";
        break;
      case State::RollingBlock:
        _stateName += "ROLLING";
        break;
      case State::PickingUpBlock:
        _stateName += "PICKING";
        break;
      case State::PlacingBlock:
        _stateName += "PLACING";
        break;
      case State::SearchingForMissingBlock:
        _stateName += "SEARCHING";
        break;
      case State::Complete:
        _stateName += "COMPLETE";
        break;
      default:
        PRINT_NAMED_WARNING("BehaviorBlockPlay.SetCurrState.InvalidState", "");
    }

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.SetState", "set state to '%s'", _stateName.c_str());
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
  
  
  Result BehaviorBlockPlay::HandleActionCompleted(Robot& robot,
                                                  const ExternalInterface::RobotCompletedAction &msg,
                                                  double currentTime_sec)
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
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.AnimCompleted",
                               "%s (result %d)",
                               msg.completionInfo.animName.c_str(),
                               msg.result);
        
        // Erase this animation action and resume pickOrPlace if there are no more animations pending
        _animActionTags.erase(msg.idTag);
        
      } else {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                               "BehaviorBlockPlay.HandleActionCompleted.UnknownAnimCompleted",
                               "");
      }
    }
    
    // Delete anim action tag, in case we somehow missed it.
    _animActionTags.erase(msg.idTag);


    
    if (msg.idTag != _lastActionTag) {
      // PRINT_NAMED_INFO("BehaviorBlockPlay.HandleActionCompleted.ExternalAction", "Ignoring");
      return lastResult;
    }
    
    switch(_currentState)
    {
      case State::TrackingFace:
      case State::TrackingBlock:
      case State::InspectingBlock:
      case State::Complete:
        _isActing = false;
        break;
      case State::RollingBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::ROLL_OBJECT_LOW:
            {
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.RollSuccessful",
                                     "");
              
              // We're done rolling the block
              SetCurrState(State::InspectingBlock);

              PlayAnimation(robot, "ID_rollBlock_succeed");

              // hold a bit before making a decision about the block
              const float inspectTime = 0.3f;
              _holdUntilTime = currentTime_sec + inspectTime;

              _isActing = false;
              break;
            }
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
            {
              
              PRINT_NAMED_ERROR("BehaviorBlockPlay.Rolling.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");

              // We failed to pick up or place the last block, try again
              SetCurrState(State::RollingBlock);
              _isActing = false;
              break;
            }
              
            default:
              // Simply ignore other action completions?
              break;
              
          } // switch(actionType)
        } else if( msg.result == ActionResult::FAILURE_RETRY ) {
          
          // We failed to pick up or place the last block, try again
          PlayAnimation(robot, "Demo_OCD_PickUp_Fail");

          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                 "BehaviorBlockPlay.HandleActionCompleted.RollFailure",
                                 "failed roll with FAILURE_RETRY,trying again");
          _isActing = false;

        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                 "BehaviorBlockPlay.HandleActionCompleted.RollFailure",
                                 "failed roll, searching for cube");
          _missingBlockFoundState = State::RollingBlock;
          SetCurrState(State::SearchingForMissingBlock);
          _isActing = false;
        }
        break;
      }
      case State::PickingUpBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PICKUP_OBJECT_LOW:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.PickupSuccessful",
                                     "");

              PlayAnimation(robot, "ID_pickUpBlock_succeed", false);
              
              // We're done picking up the block.
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
        } else if( msg.result == ActionResult::FAILURE_RETRY ) {

          // // This isn't foolproof, but use lift height to check if this failure occurred during pickup
          // // verification.
          // if (robot.GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK) {
          //   PlayAnimation(robot, "Demo_OCD_PickUp_Fail");
          // } // else {
          //   // don't make sad emote if we just drove to the wrong position
          //   _isActing = false;
          //   // }

          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                 "BehaviorBlockPlay.HandleActionCompleted.PickupFailure",
                                 "failed pickup, trying again");
          PlayAnimation(robot, "Demo_OCD_PickUp_Fail");
          _isActing = false;

        } else {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                   "BehaviorBlockPlay.HandleActionCompleted.PickupFailure",
                                   "failed pickup, searching");
            _missingBlockFoundState = State::PickingUpBlock;
            SetCurrState(State::SearchingForMissingBlock);
            _isActing = false;
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
              
              PlayAnimation(robot, "ID_reactTo2ndBlock_success");
              SetCurrState(State::Complete);
              _isActing = false;
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

      case State::SearchingForMissingBlock:
      {
        // check if we've seen the block during the "search". If so, go back to the correct state, otherwise
        // reset
        _isActing = false;

        if( msg.result == ActionResult::FAILURE_RETRY ) {
          PRINT_NAMED_INFO("BehaviorBlockPlay.SearchForMissingBlock.retry",
                           "search action failed with retry code, trying again");
          SetCurrState(State::SearchingForMissingBlock);

        }
        else {
        
          if( _lastObjectObservedTime < _searchStartTime ) {
            PRINT_NAMED_INFO("BehaviorBlockPlay.SearchForMissingBlock.NotFound",
                             "resetting (last seen %f, search started at %f)",
                             _lastObjectObservedTime,
                             _searchStartTime);
            _faceID = Face::UnknownFace;
            SetCurrState(State::TrackingFace);
          }
          else {
            PRINT_NAMED_INFO("BehaviorBlockPlay.SearchForMissingBlock.Found",
                             "Found block, restoring state.(last seen %f, search started at %f)",
                             _lastObjectObservedTime,
                             _searchStartTime);
            SetCurrState(_missingBlockFoundState);
          }
        }
        
        break;
      }
      default:
        break;
    } // switch(_currentState)


    // hack: re-enable idle animations
    if( ! _isActing ) {
      robot.SetIdleAnimation(AnimationStreamer::LiveAnimation);
    }

    return lastResult;
  } // HandleActionCompleted()

  bool BehaviorBlockPlay::HandleObservedObjectHelper(const Robot& robot,
                                                     const ExternalInterface::RobotObservedObject& msg,
                                                     double currentTime_sec)
  {
    ObjectID objectID;
    objectID = msg.objectID;

    // Make sure this is actually a block
    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
    if (nullptr == oObject) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandeObservedObject.InvalidObject",
                          "How'd this happen? (ObjectID %d)", objectID.GetValue());
      return false;
    }
    
    if(&oObject->GetPose().FindOrigin() != robot.GetWorldOrigin()) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandleObservedObject.OriginMismatch",
                          "Ignoring object %d because it does not share an origin "
                          "with the robot.", oObject->GetID().GetValue());
      return false;
    }
    
    if(oObject->IsActive() && oObject->GetIdentityState() != ActiveIdentityState::Identified) {
      PRINT_NAMED_WARNING("BehaviorBlockPlay.HandleObservedObject.UnidentifiedActiveObject",
                          "How'd this happen? (ObjectID %d, idState=%s)",
                          objectID.GetValue(), EnumToString(oObject->GetIdentityState()));
      return false;
    }
    
    // Only care about light cubes
    if (oObject->GetFamily() != ObjectFamily::LightCube) {
      return false;
    }

    return true;
  }
  

  Result BehaviorBlockPlay::HandleObservedObjectWhileNotRunning(const Robot& robot,
                                                                const ExternalInterface::RobotObservedObject &msg,
                                                                double currentTime_sec)
  {
    ObjectID objectID;
    objectID = msg.objectID;

    if( ! HandleObservedObjectHelper(robot, msg, currentTime_sec ) ) {
      return RESULT_OK;
    }

    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);

    // Get height of the object.
    // Only track the block if it's above a certain height.
    Vec3f diffVec = ComputeVectorBetween(oObject->GetPose(), robot.GetPose());
    
    // If this is observed while tracking face, then switch to tracking this object
    if ((_currentState == State::TrackingFace) &&
        (robot.GetMoveComponent().GetTrackToFace() != Face::UnknownFace) &&
        (diffVec.z() > oObject->GetSize().z()) ) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.HandleObservedObjectWhileNotRunning.TrackingBlock",
                       "Now tracking object %d",
                       objectID.GetValue());

      _trackedObject = objectID;
    }

    if( objectID == _trackedObject && msg.markersVisible ) {
      _lastObjectObservedTime = currentTime_sec;
    }

    return RESULT_OK;
  }



  Result BehaviorBlockPlay::HandleObservedObjectWhileRunning(Robot& robot,
                                                             const ExternalInterface::RobotObservedObject &msg,
                                                             double currentTime_sec)
  {
    ObjectID objectID;
    objectID = msg.objectID;

    if( ! HandleObservedObjectHelper(robot, msg, currentTime_sec ) ) {
      return RESULT_OK;
    }

    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
    
    // Get height of the object.
    // Only track the block if it's above a certain height.
    Vec3f diffVec = ComputeVectorBetween(oObject->GetPose(), robot.GetPose());

    // If this is observed while tracking face, then switch to tracking this object
    if ((_currentState == State::TrackingFace) &&
        (robot.GetMoveComponent().GetTrackToFace() != Face::UnknownFace) &&
        (diffVec.z() > oObject->GetSize().z()) ) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.HandleObservedObjectWhileRunning.TrackingBlock",
                       "Now tracking object %d",
                       objectID.GetValue());

      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StopTrackingFace",
                             "disabling face tracking (in favor of block tracking)");

      _trackedObject = objectID;

      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StartTrackingObject",
                             "Switching from face to tracking object %d", _trackedObject.GetValue());

      _noFacesStartTime = -1.0; // reset face timeout
      SetCurrState(State::TrackingBlock);
      robot.GetMoveComponent().DisableTrackToFace();
      robot.GetMoveComponent().EnableTrackToObject(_trackedObject, false);
      SetBlockLightState(robot, _trackedObject, BlockLightState::Visible);

      if( robot.IsCarryingObject() ) {
        PlayAnimation(robot, "ID_reactTo2ndBlock_01", false);
      }
      else {
        PlayAnimation(robot, "ID_react2block_01", false);
      }
    }
    
    // BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleObject",
    //                        "object id %d (tracking %d): diffvec_z: %f, z_size: %f t=%f (markersVisible? %d",
    //                        objectID.GetValue(),
    //                        _trackedObject.GetValue(),
    //                        diffVec.z(),
    //                        oObject->GetSize().z(),
    //                        currentTime_sec,
    //                        msg.markersVisible ? 1 : 0);

    if( objectID == _trackedObject && msg.markersVisible ) {
      _lastObjectObservedTime = currentTime_sec;
    }

    return RESULT_OK;
  }  

  
  Result BehaviorBlockPlay::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg, double currentTime_sec)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;
    
    if(_trackedObject == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject",
                             "About to delete tracked object, clearing.");      
      _trackedObject.UnSet();
    }
    
    if(_objectToPickUp == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject",
                             "About to delete object to pickup, clearing.");
      _objectToPickUp.UnSet();
    }

    if(_objectToPlaceOn == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleDeletedObject",
                             "About to delete object to place on, clearing.");
      _objectToPlaceOn.UnSet();
    }
    
    
    return RESULT_OK;
  }
  
  
  Result BehaviorBlockPlay::HandleObservedFace(const Robot& robot,
                                               const ExternalInterface::RobotObservedFace& msg,
                                               double currentTime_sec)
  {
    // If faceID not already set or we're not currently tracking the update the faceID
    if (_faceID == Face::UnknownFace) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleObservedFace.UpdatingFaceToTrack",
                             "id = %lld",
                             msg.faceID);
      _faceID = static_cast<Face::ID_t>(msg.faceID);
      _noFacesStartTime = -1.0;
    } else if (_faceID == robot.GetMoveComponent().GetTrackToFace() ) {
      // Currently tracking a face, keep track of last known position
      _lastKnownFacePose = robot.GetPose();
      _lastKnownFacePose.SetTranslation({msg.world_x, msg.world_y, msg.world_z});
      _hasValidLastKnownFacePose = true;
      _noFacesStartTime = -1.0;
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
  

  void BehaviorBlockPlay::StartActing(Robot& robot, IActionRunner* action)
  {
    // HACK! disable idle animation while acting
    robot.SetIdleAnimation("NONE");

    _lastActionTag = action->GetTag();
    robot.GetActionList().QueueActionAtEnd(Robot::DriveAndManipulateSlot, action);
    _isActing = true;
  }


  void BehaviorBlockPlay::PlayAnimation(Robot& robot, const std::string& animName, bool sequential)
  {
    // Check if animation is already being played
    for (auto& animTagNamePair : _animActionTags) {
      if (animTagNamePair.second == animName) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                               "BehaviorBlockPlay.PlayAnimation.Ignoring",
                               "%s already playing",
                               animName.c_str());
        return;
      }
    }
    
    BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                           "BehaviorBlockPlay.PlayAnimation",
                           "%s %s",
                           animName.c_str(),
                           sequential ? "sequentially" : "in parallel");
    
    PlayAnimationAction* animAction = new PlayAnimationAction(animName.c_str());

    if( sequential ) {
      _animActionTags[animAction->GetTag()] = animName;
      robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, animAction);
    }
    else {
      robot.GetActionList().QueueActionNow(Robot::FaceAnimationSlot, animAction);
    }
  }
  


  
  
} // namespace Cozmo
} // namespace Anki
