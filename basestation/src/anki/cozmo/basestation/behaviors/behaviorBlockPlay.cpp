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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorBlockPlay.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/trackingActions.h"

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#define DEBUG_BLOCK_PLAY_BEHAVIOR 1

#define DO_2001_MUSIC_GAG 0

#define HANNS_SPEED 0

// TODO:(bn) when driving to the side or back of the cube to roll it, the position is really far off. Instead
// of relying on the action (or maybe add this logic within the action?) plan to a pre-dock pose which is much
// further away from the block, so it can adjust more naturally instead of having to back up and do weird
// stuff

// TODO:(bn) tune speed, so maybe we can move faster and make everything look better. Ideally, this whole
// behavior could have a single parameter for this

// TODO:(bn) need to apply motion profile to turn in place and face pose action???


namespace Anki {
namespace Cozmo {
  
  BehaviorBlockPlay::BehaviorBlockPlay(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::TrackingFace)
  , _lastHandlerResult(RESULT_OK)
  //, _inactionStartTime(0)
  , _faceID(Face::UnknownFace)
  {
    SetDefaultName("BlockPlay");

    // start with defaults
    _motionProfile = DEFAULT_PATH_MOTION_PROFILE;

#if HANNS_SPEED

    _motionProfile.speed_mmps = 120.0f;
    _motionProfile.accel_mmps2 = 200.0f;
    _motionProfile.decel_mmps2 = 120.0f;
    _motionProfile.pointTurnSpeed_rad_per_sec = 2.5f;
    _motionProfile.pointTurnAccel_rad_per_sec2 = 8.0f;
    _motionProfile.pointTurnDecel_rad_per_sec2 = 6.0f;
    _motionProfile.dockSpeed_mmps = 120.0f;
    _motionProfile.reverseSpeed_mmps = 50.0f;

#else
    _motionProfile.speed_mmps = 60.0f;
    _motionProfile.accel_mmps2 = 200.0f;
    _motionProfile.decel_mmps2 = 200.0f;
    _motionProfile.pointTurnSpeed_rad_per_sec = 2.0f;
    _motionProfile.pointTurnAccel_rad_per_sec2 = 100.0f;
    _motionProfile.pointTurnDecel_rad_per_sec2 = 100.0f;
    _motionProfile.dockSpeed_mmps = 80.0f; // slow it down a bit for reliability
    _motionProfile.reverseSpeed_mmps = 40.0f;
#endif    

    SubscribeToTags({{
      EngineToGameTag::RobotCompletedAction,
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedObject,
      EngineToGameTag::RobotObservedFace,
      EngineToGameTag::RobotDeletedFace,
      EngineToGameTag::ObjectMoved
    }});


  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorBlockPlay::IsRunnable(const Robot& robot, double currentTime_sec) const
  {
    const bool holdingBlock = robot.IsCarryingObject() && _objectToPickUp.IsSet();
    const bool alreadyRunning = _currentState != State::TrackingFace;
    Pose3d waste;
    const bool hasSeenFace = _faceID != Face::UnknownFace || robot.GetFaceWorld().GetLastObservedFace(waste) > 0;
    const bool hasObject = _trackedObject.IsSet();
    const bool isDoingSomething = _isActing || !_animActionTags.empty();
    const bool needsToUpdateLights = ! _objectsToTurnOffLights.empty();



    bool ret = alreadyRunning || holdingBlock || isDoingSomething || (hasSeenFace && hasObject) || needsToUpdateLights;

    // static bool wasTrue = false;
    // if(ret) {
    //   wasTrue = true;
    // }

    // if( wasTrue && ! ret ) {
    //   PRINT_NAMED_INFO("BehaviorBlockPlay.IsRunnable.NowFalse",
    //                    "iscarrying? %d, pickupSet? %d, currStateTracking? %d, faceIDKnown? %d, lastValid? %d, trackedSet? %d",
    //                    robot.IsCarryingObject() ? 1 : 0,
    //                    _objectToPickUp.IsSet()  ? 1 : 0,
    //                    alreadyRunning ? 1 : 0,
    //                    _faceID != Face::UnknownFace ? 1 : 0,
    //                    _hasValidLastKnownFacePose  ? 1 : 0,
    //                    hasObject ? 1 : 0);
    //   wasTrue = false;
    // }
      
    
    return ret;
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
      _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _timetoInspectBlock;
      SetCurrState(State::InspectingBlock);
    } else {
      SetCurrState(State::TrackingFace);
    }
  }

  namespace {
  void AddToCompoundAction( CompoundActionParallel*& compound, IActionRunner* newAction ) {
    if(compound == nullptr) {
      compound = new CompoundActionParallel;
    }
    compound->AddAction(newAction);
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

    UpdateStateName();

    if( ! _objectsToTurnOffLights.empty() ) {
      for( auto it = _objectsToTurnOffLights.begin();
           it != _objectsToTurnOffLights.end();
           it = _objectsToTurnOffLights.erase(it) ) {
        SetBlockLightState(robot, *it, BlockLightState::None);
      }
      
      // check if that was the only reason we were running, in which case bail out now
      
      if( ! IsRunnable(robot, currentTime_sec) ) {
        return Status::Complete;
      }
    }

    
    if( robot.IsCarryingObject() ) {
      LiftShouldBeLocked(robot);
      if( _currentState == State::TrackingFace || _currentState == State::WaitingForBlock ) {
        HeadShouldBeLocked(robot);
      }
      else {
        HeadShouldBeUnlocked(robot);
      }
    }
    else {
      LiftShouldBeUnlocked(robot);
      HeadShouldBeUnlocked(robot);
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

    // wait until after _isActing check for body lock so we can unlock it specifically for certain actions / animations
    if( _currentState == State::TrackingFace ) {
      BodyShouldBeUnlocked(robot);
    }
    else {
      BodyShouldBeLocked(robot);
    }

    switch(_currentState)
    {
      case State::TrackingFace:
      {

        // Behavior is different depending on whether we are carrying a block (it's almost like a different
        // state really....)

        if( ! robot.IsCarryingObject() )  {
          if( _faceID != Face::UnknownFace ) {
            TrackFaceAction* action = new TrackFaceAction(_faceID);
            // NOTE: don't use StartActing for this, because it is a continuous action
            robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackFace.Enabled",
                                   "EnableTrackToFace %lld", _faceID);
          }
          else if( robot.GetMoveComponent().GetTrackToFace() != _faceID ) {
            PRINT_NAMED_INFO("BehaviorBlockPlay.TrackingWrongFace",
                             "Disabling face tracking because we aren't tracking the correct face (or it was deleted)");
            robot.GetActionList().Cancel(Robot::DriveAndManipulateSlot, RobotActionType::TRACK_FACE);
          }
          else {
            if (_noFacesStartTime <= 0) {
              _noFacesStartTime = currentTime_sec;
            }

            const float abortAfterNoFacesFor_s = 3.0f;
            
            // abort if we haven't seen a face in a while (this should allow us to run, e.g.,  the FindFaces behavior)
            if (currentTime_sec - _noFacesStartTime > abortAfterNoFacesFor_s ) {
              PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.NoFacesSeen", "Aborting behavior");
              return Status::Complete;
            }
          }
        }

        break;
      }
      case State::TrackingBlock:
      {

        if (!_trackedObject.IsSet()) {
          if( robot.GetMoveComponent().GetTrackToObject().IsSet() ) {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackingBlockUnset",
                                   "disabling object tracking because object was deleted / unset");
            robot.GetActionList().Cancel(Robot::DriveAndManipulateSlot, RobotActionType::TRACK_OBJECT);
          }
          _faceID = Face::UnknownFace;
          TurnTowardsAFace(robot);
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

          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.TrackingBlock.Error",
                              "Should not be in TrackingBlock state while carrying block! bailing");
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
          break;
        }

        if( currentTime_sec > _trackedObjectStoppedMovingTime + 0.5f &&
            currentTime_sec < _trackedObjectStoppedMovingTime + 5.0f) {
          // if the block recently stopped moving, look down
          
          if( _lastObjectObservedTime + _lostBlockTimeToLookDown < currentTime_sec ) {
            const float targetAngle = -7.5f;
            if( robot.GetHeadAngle() > targetAngle + DEG_TO_RAD(3.0f) ) {

              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.LookDownForBlock",
                                     "haven't seen block since %f (t=%f), looking down",
                                     _lastObjectObservedTime,
                                     currentTime_sec);

              _oldHeadAngle_rads = robot.GetHeadAngle();
              
              // look down to see if we see the cube there
              MoveHeadToAngleAction* lookDownAction = new MoveHeadToAngleAction(targetAngle);
              lookDownAction->SetMoveEyes(true, true); // hold eyes down until next head movement
              MoveLiftToHeightAction* moveLiftAction =
                new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
              
              StartActing(robot, new CompoundActionSequential({lookDownAction, moveLiftAction}));

              // resume tracking afterwards
              TrackObjectAction* trackingAction = new TrackObjectAction(_trackedObject);
              robot.GetActionList().QueueActionAtEnd(Robot::DriveAndManipulateSlot, trackingAction);
              break;
            }
          }
        }
        
        // Check that block is on the ground and not moving
        if( currentTime_sec > _trackedObjectStoppedMovingTime + 0.75f &&
            diffVec.z() < 0.75 * obj->GetSize().z()) {
          
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.BlockOnGround", "State: TrackingBlock");
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StopTrackingBlock",
                                 "disabling block tracking in order to inspect block");
          SetCurrState(State::InspectingBlock);
          robot.GetActionList().Cancel(Robot::DriveAndManipulateSlot, RobotActionType::TRACK_OBJECT);
          PlayAnimation(robot, "ID_react2block_02", false);

          // hold a bit before making a decision about the block
          _holdUntilTime = currentTime_sec + _timetoInspectBlock;
        }
        
        break;
      }
      case State::InspectingBlock:
      {
        ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_trackedObject);
        if (nullptr == obj) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: InspectingBlock");
          _faceID = Face::UnknownFace;
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
          break;
        }
        
        if (robot.IsCarryingObject()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.InspecingBlock.Error",
                              "Holding block while inspecting, this shouldn't happen! bailing");
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
          break;
        }            

        // Check if it's sideways
        AxisName upAxis = obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
        if (upAxis == AxisName::Z_POS) {
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.PickingUpBlock", "");
          SetBlockLightState(robot, _trackedObject, BlockLightState::Upright);
          SetCurrState(State::PickingUpBlock);
          _objectToPickUp = _trackedObject;
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
          TurnTowardsAFace(robot);
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
        DriveToRollObjectAction* rollAction = nullptr;
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
        SetDriveToObjectSounds(rollAction);
        StartActing(robot, rollAction);
        break;
      }
      case State::PickingUpBlock:
      {
        // Verify that object still exists
        if (!_objectToPickUp.IsSet()) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.UnexpectedNullptr", "State: PickingUpBlock");
          _faceID = Face::UnknownFace;
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
          break;
        }
        
        // Check if object is very well aligned for pickup already.
        // If so, do pickup action without driving to predock pose.
        ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_objectToPickUp);
        if (nullptr == obj) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.UpdateInternal.PickupBlockNotFound", "ObjID %d", _objectToPickUp.GetValue());
          _faceID = Face::UnknownFace;
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
        } else {
          Radians blockOrientation = obj->GetPose().GetRotationAngle<'Z'>();
          Radians robotOrientation = robot.GetPose().GetRotationAngle<'Z'>();
          Vec3f robotToBlockVector = ComputeVectorBetween(obj->GetPose(), robot.GetPose());
          Radians robotToBlockOrientation = atan2f(robotToBlockVector.y(), robotToBlockVector.x());
          
          // Is block orientation reasonably close to robot orientation?
          bool robotAndBlockOrientationMatch = std::fabsf( std::fmodf((blockOrientation - robotOrientation).ToFloat(), PIDIV2_F)) < _robotObjectOrientationDiffThreshForDirectPickup;
          
          // Is the block more or less directly in front of the robot?
          bool blockIsInFrontOfRobot = std::fabsf((robotToBlockOrientation - robotOrientation).ToFloat()) < _angleToObjectThreshForDirectPickup;
          
          if (robotAndBlockOrientationMatch && blockIsInFrontOfRobot) {
            // Alignment is good. Go straight to pickup!
            PickupObjectAction* pickupAction = new PickupObjectAction(_objectToPickUp);
            pickupAction->SetDoNearPredockPoseCheck(false);
            StartActing(robot, pickupAction);
            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.Pickup.Direct",
                             "orientation diff = %f (need < %f), angle diff = %f (need < %f)",
                             std::fabsf( std::fmodf((blockOrientation - robotOrientation).ToFloat(), PIDIV2_F)),
                             _robotObjectOrientationDiffThreshForDirectPickup,
                             std::fabsf((robotToBlockOrientation - robotOrientation).ToFloat()),
                             _angleToObjectThreshForDirectPickup);
                             
          } else {
            DriveToPickupObjectAction* pickupAction = new DriveToPickupObjectAction(_objectToPickUp, _motionProfile);
            SetDriveToObjectSounds(pickupAction);
            StartActing(robot, pickupAction);

            PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.Pickup.Drive",
                             "orientation diff = %f (need < %f), angle diff = %f (need < %f)",
                             std::fabsf( std::fmodf((blockOrientation - robotOrientation).ToFloat(), PIDIV2_F)),
                             _robotObjectOrientationDiffThreshForDirectPickup,
                             std::fabsf((robotToBlockOrientation - robotOrientation).ToFloat()),
                             _angleToObjectThreshForDirectPickup);
          }
        }
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

        PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.PlacingBlock.Place",
                         "Executing place object action");
        
        DriveToPlaceOnObjectAction* placeAction = new DriveToPlaceOnObjectAction(robot, _objectToPlaceOn, _motionProfile);
        SetDriveToObjectSounds(placeAction);
        StartActing(robot, placeAction);
        break;
      }
      case State::WaitingForBlock:
      {
        // we should be holding a block, waiting for the second one. If not, bail out of this state
        if( ! robot.IsCarryingObject() ) {
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.WaitingForBlock.NotCarrying",
                           "robot should have been carrying block, but isn't. bailing");
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
          break;
        }

        // lift should be high, and face should be low
        IActionRunner* actionToRun = nullptr;
        if( robot.GetLiftHeight() < LIFT_HEIGHT_CARRY - 3.0f ) {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.UpdateInternal.WaitingForBlock.Lift",
                                 "raising lift to look down for block");

          actionToRun = new MoveLiftToHeightAction( LIFT_HEIGHT_CARRY );
        }
        if( ! NEAR( robot.GetHeadAngle(), _waitForBlockHeadAngle_rads, DEG_TO_RAD(4.0f) ) ) {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.UpdateInternal.WaitingForBlock.Head",
                                 "lowering head to see block");
          MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction( _waitForBlockHeadAngle_rads );
          if( actionToRun != nullptr ) {
            actionToRun = new CompoundActionParallel({actionToRun, headAction});
          }
          else {
            actionToRun = headAction;
          }
        }

        if( actionToRun != nullptr ) {
          StartActing(robot, actionToRun);
          _waitForBlockStartTime = -1.0f;
        }
        else if( _waitForBlockStartTime < 0.0f ) {
          _waitForBlockStartTime = currentTime_sec;
        }
        else if( currentTime_sec > _waitForBlockStartTime + _maxTimeToWaitForBlock ) {
          PRINT_NAMED_INFO("BehaviorBlockPlay.UpdateInternal.WaitingForBlock.WaitingTooLong",
                           "We have waited %f seconds to see the block, but didn't, so giving up",
                           currentTime_sec - _waitForBlockStartTime);
          // TODO:(bn) sound here??
          _waitForBlockStartTime = -1.0f;
          TurnTowardsAFace(robot);
          SetCurrState(State::TrackingFace);
        }
        
        break;
      }
      case State::SearchingForMissingBlock:
      {
        // first face the block object (where we think it is, if we know), then look a bit to the left and
        // right

        const float skipSearchIfSeenWithin_s = 0.5f;
        if( _lastObjectObservedTime > currentTime_sec - skipSearchIfSeenWithin_s ) {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.UpdateInternal.SearchingForMissingBlock.SkipSearch",
                                 "in searching state, but saw the object at %f (t=%f), so not searching",
                                 _lastObjectObservedTime,
                                 currentTime_sec);
          SetCurrState(_missingBlockFoundState);
          if( _missingBlockFoundState == State::InspectingBlock ) {
            _holdUntilTime = currentTime_sec + _timetoInspectBlock;
          }
        }
        else {
        
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
        }

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

  void BehaviorBlockPlay::TurnTowardsAFace(Robot& robot)
  {
    if( ! robot.IsCarryingObject() ) {
      Pose3d lastFacePose;
      if( robot.GetFaceWorld().GetLastObservedFace(lastFacePose) > 0 ) {
        IActionRunner* lookAtFaceAction = new FacePoseAction(lastFacePose, DEG_TO_RAD(5), PI_F);
        StartActing(robot, lookAtFaceAction);
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TurnTowardsAFace.FindingOldFace",
                               "Moving to last face pose");
        lastFacePose.Print();
      }
      else if( robot.GetHeadAngle() < DEG_TO_RAD(15.0f) && ! robot.GetMoveComponent().IsHeadMoving()) {
        PRINT_NAMED_INFO("BehaviorBlockPlay.TurnTowardsAFace.NeverSawFace",
                         "never saw any faces, so just moving head up instead");
        StartActing(robot, new MoveHeadToAngleAction( DEG_TO_RAD(20.0f) ));
      }
    }
    else {
      // robot is holding a block
      // we may need to do multiple actions in parallel, so set up a compound action just in case
      CompoundActionParallel* actionToRun = nullptr;

      const float lowCarry = kLowCarryHeightMM; //  + 10;
          
      if( robot.GetLiftHeight() > lowCarry + 5 && !robot.GetMoveComponent().IsLiftMoving() ) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TurnTowardsAFace.MoveLift",
                               "block in lift is blocking view, moving from %f to %f",
                               robot.GetLiftHeight(),
                               lowCarry);

        MoveLiftToHeightAction* moveLiftAction = new MoveLiftToHeightAction(lowCarry);
        // move slowly
        moveLiftAction->SetDuration(1.0f);
        AddToCompoundAction(actionToRun, moveLiftAction);
      }

      // figure out which body angle we should face to look at a person. For the demo, we assume a table
      // with a reasonable head height, so we'll just pick the head angle manually to avoid bugs where it
      // stares into it's own cube
      const float fixedHeadAngle_rads = DEG_TO_RAD(25.0f);
      Radians targetBodyAngle = 0;
      bool haveTargetAngle = false;
          
      if( _faceID != Face::UnknownFace ) {
        auto face = robot.GetFaceWorld().GetFace(_faceID);
        if( face != nullptr ) {
          Pose3d faceWrtRobot;
          if( face->GetHeadPose().GetWithRespectTo(robot.GetPose(), faceWrtRobot) ) {
            targetBodyAngle = std::atan2( faceWrtRobot.GetTranslation().y(),
                                          faceWrtRobot.GetTranslation().x() );
            haveTargetAngle = true;
                
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TurnTowardsAFace.Block.HaveFaceId",
                                   "turning towards face id %lld at angle %fdeg",
                                   _faceID,
                                   DEG_TO_RAD( targetBodyAngle.ToFloat() ));
          }
        }
      }

      if( ! haveTargetAngle ) {
        // just use the last face from face world
        Pose3d lastFacePose;
        if( robot.GetFaceWorld().GetLastObservedFace(lastFacePose) > 0 ) {
          Pose3d faceWrtRobot;
          if( lastFacePose.GetWithRespectTo(robot.GetPose(), faceWrtRobot) ) {
            targetBodyAngle = std::atan2( faceWrtRobot.GetTranslation().y(),
                                          faceWrtRobot.GetTranslation().x() );
            haveTargetAngle = true;

            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TurnTowardsAFace.Block.UseLastFace",
                                   "turning towards the last face in Faceworld, with angle %fdeg",
                                   DEG_TO_RAD( targetBodyAngle.ToFloat() ));

            lastFacePose.Print();
            faceWrtRobot.Print();
          }
        }
      }

          
      const float turnInPlaceTol_rads = DEG_TO_RAD(3.0f);
          
      if( haveTargetAngle ) {
        if( !robot.GetMoveComponent().IsMoving() &&
            (robot.GetPose().GetRotationAngle<'Z'>() - targetBodyAngle).getAbsoluteVal().ToFloat() > turnInPlaceTol_rads ) {
          AddToCompoundAction(actionToRun, new TurnInPlaceAction(targetBodyAngle, false) );
        }
      }
      else {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TurnTowardsAFace.Block.NoTurnInPlace",
                               "couldn't find an angle to turn to");
      }

      if( !robot.GetMoveComponent().IsHeadMoving() &&
          (Radians(fixedHeadAngle_rads) - Radians(robot.GetHeadAngle())).getAbsoluteVal().ToFloat() > turnInPlaceTol_rads ) {
        AddToCompoundAction(actionToRun, new MoveHeadToAngleAction(fixedHeadAngle_rads) );
      }

      if( actionToRun != nullptr ) {
        StartActing(robot, actionToRun);
      }
    }
  }

  Result BehaviorBlockPlay::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
  {
    _interrupted = true;

    if(_lastActionTag != static_cast<u32>(ActionConstants::INVALID_TAG)) {
      // Make sure we don't stay in tracking when we leave this action
      // TODO: this will cancel any action we were doing. Cancel all tracking actions?
      robot.GetActionList().Cancel(_lastActionTag);
    }
    
    HeadShouldBeUnlocked(robot);
    LiftShouldBeUnlocked(robot);
    BodyShouldBeUnlocked(robot);
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

      case EngineToGameTag::ObjectMoved:
        _lastHandlerResult = HandleObjectMoved(robot, event.GetData().Get_ObjectMoved());
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
      case EngineToGameTag::ObjectMoved:
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
  
  void BehaviorBlockPlay::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
      case EngineToGameTag::RobotDeletedObject:
      case EngineToGameTag::RobotObservedFace:
      case EngineToGameTag::RobotDeletedFace:
      case EngineToGameTag::ObjectMoved:
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

    UpdateStateName();

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.SetState",
                           "set state to '%s'", GetStateName().c_str());
  }

  void BehaviorBlockPlay::UpdateStateName()
  {
    std::string name;
    switch(_currentState)
    {
      case State::TrackingFace:
        name = "FACETRACK";
        break;
      case State::TrackingBlock:
        name = "BLOCKTRACK";
        break;
      case State::InspectingBlock:
        name = "INSPECTING";
        break;
      case State::RollingBlock:
        name = "ROLLING";
        break;
      case State::PickingUpBlock:
        name = "PICKING";
        break;
      case State::PlacingBlock:
        name = "PLACING";
        break;
      case State::WaitingForBlock:
        name = "WAIT4BLOCK";
        break;
      case State::SearchingForMissingBlock:
        name = "SEARCHING";
        break;
      default:
        PRINT_NAMED_WARNING("BehaviorBlockPlay.SetCurrState.InvalidState", "");
        break;
    }

    if( _isActing ) {
      name += '*';
    }
    else {
      name += ' ';
    }

    if( ! _animActionTags.empty() ) {
      name += std::to_string(_animActionTags.size());
    }

    SetStateName(name);
  }


  // TODO: Get color and on/off settings from config
  void BehaviorBlockPlay::SetBlockLightState(Robot& robot, const ObjectID& objID, BlockLightState state)
  {
    std::string name = "<INVALID>";
    
    switch(state) {
      case BlockLightState::None: {
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL,
                              ::Anki::NamedColors::BLACK, ::Anki::NamedColors::BLACK,
                              10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        name = "None";
        break;
      }
      case BlockLightState::Visible: {
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL,
                              ::Anki::NamedColors::CYAN, ::Anki::NamedColors::BLACK,
                              10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        name = "Visible";
        break;
      }
      case BlockLightState::Upright: {
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL,
                              ::Anki::NamedColors::BLUE, ::Anki::NamedColors::BLUE,
                              200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        name = "Upright";
        break;
      }
      case BlockLightState::Complete: {
        robot.SetObjectLights(objID, WhichCubeLEDs::ALL,
                              ::Anki::NamedColors::GREEN, ::Anki::NamedColors::GREEN,
                              200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        name = "Complete";
        break;
      }
      default:
        break;
    }

    PRINT_NAMED_INFO("BehaviorBlockPlay.SetBlockLightState",
                     "setting block %d to '%s'",
                     objID.GetValue(),
                     name.c_str());
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
                               "%s (result: %s)",
                               msg.completionInfo.Get_animationCompleted().animationName.c_str(),
                               EnumToString(msg.result));
        
        // Erase this animation action and resume pickOrPlace if there are no more animations pending
        _animActionTags.erase(msg.idTag);

        if( msg.result != ActionResult::SUCCESS ) {
          PRINT_NAMED_WARNING("BehaviorBlockPlay.HandleCompletedAction.Fail",
                              "animation '%s' did not complete",
                              msg.completionInfo.Get_animationCompleted().animationName.c_str());
        }
      } else {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                               "BehaviorBlockPlay.HandleActionCompleted.UnknownAnimCompleted",
                               "");
      }
    }
    
    // Delete anim action tag, in case we somehow missed it.
    _animActionTags.erase(msg.idTag);

    // TODO:(bn) just use callback for this?
    if( _isDrivingForward && _driveForwardActionTag == msg.idTag )
    {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.DoneDrivingForward",
                             "finished drive forward action, no longer driving forward, result=%s",
                             ActionResultToString(msg.result));
      _isDrivingForward = false;
      return lastResult;
    }
    
    if (msg.idTag != _lastActionTag) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.HandleActionCompleted.OtherAction",
                             "finished action id=%d, but only care about id %d",
                             msg.idTag,
                             _lastActionTag);
      return lastResult;
    }

    if( _actionResultCallback ) {
      if( _actionResultCallback(msg.result) ) {
        return lastResult;
      }
    }
    
    switch(_currentState)
    {
      case State::TrackingFace:
      case State::TrackingBlock:
      case State::InspectingBlock:
      case State::WaitingForBlock:
        _isActing = false;
        break;
      case State::RollingBlock:
      {
        ++_attemptCounter;
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
              _holdUntilTime = currentTime_sec + _timetoInspectBlock;

              _isActing = false;
              _attemptCounter = 0;
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
          switch(msg.completionInfo.Get_objectInteractionCompleted().result)
          {
            case ObjectInteractionResult::INCOMPLETE:
            case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
            {
              PlayAnimation(robot, "ID_react2block_align_fail");
              break;
            }
            
            default: {
              PlayAnimation(robot, "ID_rollBlock_fail_01");
              break;
            }
          }


          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                 "BehaviorBlockPlay.HandleActionCompleted.RollFailure",
                                 "failed roll with FAILURE_RETRY and %s,trying again",
                                 EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
          // go back to inspecting so we know if we need to do a roll or a pickup
          SetCurrState(State::InspectingBlock);
          _holdUntilTime = currentTime_sec + _timetoInspectBlock;
          _isActing = false;
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                 "BehaviorBlockPlay.HandleActionCompleted.RollFailure",
                                 "failed roll with %s, searching for cube",
                                 EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
          _missingBlockFoundState = State::InspectingBlock;
          SetCurrState(State::SearchingForMissingBlock);
          _isActing = false;
        }
        break;
      }
      case State::PickingUpBlock:
      {
        ++_attemptCounter;
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
              _attemptCounter = 0;
              TurnTowardsAFace(robot);
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              
              PRINT_NAMED_ERROR("BehaviorBlockPlay.PickingUp.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");
              
              // We failed to pick up or place the last block, try again, and check if we need to roll or not
              SetCurrState(State::InspectingBlock);
              _holdUntilTime = currentTime_sec + _timetoInspectBlock;
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
                                 "failed pickup with %s, trying again",
                                 EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));

          switch(msg.completionInfo.Get_objectInteractionCompleted().result)
          {
            case ObjectInteractionResult::INCOMPLETE:
            case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE: {
              PlayAnimation(robot, "ID_react2block_align_fail");
              break;
            }

            default: {
              PlayAnimation(robot, "ID_rollBlock_fail_01");  // TEMP:  // TODO:(bn) different one here?
            }
          }

          SetCurrState(State::InspectingBlock);
          _holdUntilTime = currentTime_sec + _timetoInspectBlock;
          _isActing = false;

        } else {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                   "BehaviorBlockPlay.HandleActionCompleted.PickupFailure",
                                   "failed pickup with %s, searching",
                                   EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
            _missingBlockFoundState = State::InspectingBlock;
            SetCurrState(State::SearchingForMissingBlock);
            _isActing = false;
        }
        
        break;
      } // case PickingUpBlock
        
      case State::PlacingBlock:
      {
        ++_attemptCounter;
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PLACE_OBJECT_HIGH:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.PlacementSuccessful",
                                     "Placed object %d on object %d",
                                     _objectToPickUp.GetValue(),
                                     _objectToPlaceOn.GetValue());
              
              SetBlockLightState(robot, _objectToPlaceOn, BlockLightState::Complete);
              SetBlockLightState(robot, _objectToPickUp, BlockLightState::Complete);
              
              _trackedObject.UnSet();

              BodyShouldBeUnlocked(robot);

              // play the happy animation, then look at the user, then ignore the cubes until they move (wait
              // to ignore the cubes in case we shake / bump them during the other actions)
                            
              // wait for happy to finish before we mark the blocks, so the animation doesn't trigger their motion
              StartActing(robot,
                          new PlayAnimationAction("ID_reactTo2ndBlock_success"),
                          [this,&robot](ActionResult ret){
                            IgnoreObject(robot, _objectToPlaceOn);
                            _objectToPlaceOn.UnSet();
              
                            IgnoreObject(robot, _objectToPickUp);
                            _objectToPickUp.UnSet();

                            TurnTowardsAFace(robot);
                            SetCurrState(State::TrackingFace);

                            return true;
                          });

              _attemptCounter = 0;
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

          switch(msg.completionInfo.Get_objectInteractionCompleted().result)
          {
            case ObjectInteractionResult::INCOMPLETE:
            case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
            {
              // TODO:(bn) "soft fail" sound here?
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.PlacementFailure",
                                     "pre-dock fail, trying again");
              PlayAnimation(robot, "ID_react2block_align_fail");
              SetCurrState(State::PlacingBlock);
              _isActing = false;
              break;
            }

            case ObjectInteractionResult::VISUAL_VERIFICATION_FAILED:
            {
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.PlacementFailure",
                                     "failed to visually verify before placement docking, searching for cube");
              PlayAnimation(robot, "ID_react2block_align_fail");

              // search for the block, but hold a bit first in case we see it
              _missingBlockFoundState = State::PlacingBlock;
              _holdUntilTime = currentTime_sec + 0.75f;
              SetCurrState(State::SearchingForMissingBlock);
              _isActing = false;
              break;
            }
              
            default:
            {
              BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                                     "BehaviorBlockPlay.HandleActionCompleted.PlacementFailure",
                                     "dock fail with %s, backing up to try again",
                                     EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
              
              _objectToPlaceOn.UnSet();
              _objectToPickUp.UnSet();
              _trackedObject.UnSet();
              
              const float failureBackupDist = 70.0f;
              const float failureBackupSpeed = 80.0f;
              
              // back up and drop the block, then re-init to start over
              StartActing(robot,
                          new CompoundActionSequential({
                new PlayAnimationAction("ID_rollBlock_fail_01"),
                new DriveStraightAction(-failureBackupDist, -failureBackupSpeed),
                new PlaceObjectOnGroundAction()}),
                          [this,&robot](ActionResult ret){
                            _isActing = false;
                            InitState(robot);
                            return true;
                          });
              break;
            }
          } // switch(objectInteractionResult)
          
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
            TurnTowardsAFace(robot);
            SetCurrState(State::TrackingFace);
          }
          else {
            PRINT_NAMED_INFO("BehaviorBlockPlay.SearchForMissingBlock.Found",
                             "Found block, restoring state.(last seen %f, search started at %f)",
                             _lastObjectObservedTime,
                             _searchStartTime);
            SetCurrState(_missingBlockFoundState);
            if( _missingBlockFoundState == State::InspectingBlock ) {
              _holdUntilTime = currentTime_sec + _timetoInspectBlock;
            }
          }
        }
        
        break;
      }
      default:
        break;
    } // switch(_currentState)

    return lastResult;
  } // HandleActionCompleted()

  void BehaviorBlockPlay::IgnoreObject(Robot& robot, ObjectID objectID)
  {
    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
    if (nullptr != oObject) {
      if( oObject->IsActive() && oObject->GetIdentityState() == ActiveIdentityState::Identified ) {
        // NOTE: I actually need to know if I'm "connected" to this object. This could create big problems
        // otherwise, because we'll never get a "moved" message. Potential work-around: keep track of which
        // objects we *ever* got a moved message from, and don't add them here. This code will work correctly
        // as long as blocks stay powered and connected, otherwise we won't be able to re-play the demo
        _objectsToIgnore.insert(objectID);
        PRINT_NAMED_INFO("BehaviorBlockPlay.IgnoreObject", "ignoring block %d",
                         objectID.GetValue());
      }
    }
  }

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

    if( _objectsToIgnore.find(objectID) != _objectsToIgnore.end() ) {
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

    // const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);

    // // Get height of the object.
    // // Only track the block if it's above a certain height.
    // Vec3f diffVec = ComputeVectorBetween(oObject->GetPose(), robot.GetPose());
    
    // If this is observed while tracking face, then switch to tracking this object
    if (_currentState == State::TrackingFace &&
        msg.markersVisible// &&
        // // (robot.GetMoveComponent().GetTrackToFace() != Face::UnknownFace) &&
        // (diffVec.z() > oObject->GetSize().z())
      ) {
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
    if (_currentState == State::TrackingFace &&
        msg.markersVisible // &&
        // (robot.GetMoveComponent().GetTrackToFace() != Face::UnknownFace) &&
        // (diffVec.z() > oObject->GetSize().z())
      ) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.HandleObservedObjectWhileRunning.TrackingBlock",
                       "Now tracking object %d",
                       objectID.GetValue());

      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StopTrackingFace",
                             "disabling face tracking (in favor of block tracking)");

      _trackedObject = objectID;

      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StartTrackingObject",
                             "Switching from face to tracking object %d", _trackedObject.GetValue());

      _noFacesStartTime = -1.0; // reset face timeout
      SetBlockLightState(robot, _trackedObject, BlockLightState::Visible);
     
      if( robot.IsCarryingObject() ) {
        // look at the block, then react to it
        StartActing(robot,
                    new FacePoseAction(oObject->GetPose(), DEG_TO_RAD(5), PI_F),
                    [this,&robot](ActionResult ret){
                      PlayAnimation(robot, "ID_reactTo2ndBlock_01");
                      return false;
                    });
        SetCurrState(State::WaitingForBlock);
      }
      else {
#if DO_2001_MUSIC_GAG
        // play the animation, followed by the music
        PlayAnimationAction* animAction = new PlayAnimationAction("ID_react2block_01");
        DeviceAudioAction* startMusicAction = new DeviceAudioAction(Audio::MusicGroupStates::CUBE_INTERACTIONFIRST);
        WaitAction* musicWaitAction = new WaitAction(8.0f);
        DeviceAudioAction* stopMusicAction = new DeviceAudioAction(Audio::MusicGroupStates::SILENCE);

        CompoundActionSequential* combinedAction = new CompoundActionSequential({animAction,
              startMusicAction,
              musicWaitAction,
              stopMusicAction});

        _animActionTags[combinedAction->GetTag()] = "animPlusMusic";
        robot.GetActionList().QueueActionNow(Robot::FaceAnimationSlot, combinedAction);
#else
        PlayAnimation(robot, "ID_react2block_01", false);
#endif
        
        SetCurrState(State::TrackingBlock);
        TrackObjectAction* action = new TrackObjectAction(_trackedObject);
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action); // will cancel face tracking 
      }
    }

    else if( _currentState == State::WaitingForBlock &&
             ! _isActing && 
             _objectToPlaceOn != objectID && 
             msg.markersVisible &&
             robot.IsCarryingObject() &&
             diffVec.z() < 0.75 * oObject->GetSize().z()) {
      _objectToPlaceOn = objectID;
      SetBlockLightState(robot, _trackedObject, BlockLightState::Upright);
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.StartPlacing",
                             "Found block %d, placing on it",
                             _objectToPlaceOn.GetValue());
      SetCurrState(State::PlacingBlock);        
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

      if( _currentState == State::TrackingBlock && !robot.IsCarryingObject() && _animActionTags.empty()) {
        TrackBlockWithLift(robot, oObject->GetPose());
      }
    }

    return RESULT_OK;
  }

  void BehaviorBlockPlay::LiftShouldBeLocked(Robot& robot)
  {
    if( ! _lockedLift ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockLift",
                             "LOCKED");

      robot.GetMoveComponent().LockAnimTracks(static_cast<u8>(AnimTrackFlag::LIFT_TRACK));
      _lockedLift = true;
    }
  }

  void BehaviorBlockPlay::LiftShouldBeUnlocked(Robot& robot)
  {
    if( _lockedLift ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockLift",
                             "UNLOCKED");

      robot.GetMoveComponent().UnlockAnimTracks(static_cast<u8>(AnimTrackFlag::LIFT_TRACK));
      _lockedLift = false;
    }
  }

  void BehaviorBlockPlay::HeadShouldBeLocked(Robot& robot)
  {
    if( ! _lockedHead ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockHead",
                             "LOCKED");

      robot.GetMoveComponent().LockAnimTracks(static_cast<u8>(AnimTrackFlag::HEAD_TRACK));
      _lockedHead = true;
    }
  }

  void BehaviorBlockPlay::HeadShouldBeUnlocked(Robot& robot)
  {
    if( _lockedHead ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockHead",
                             "UNLOCKED");

      robot.GetMoveComponent().UnlockAnimTracks(static_cast<u8>(AnimTrackFlag::HEAD_TRACK));
      _lockedHead = false;
    }
  }

  void BehaviorBlockPlay::BodyShouldBeLocked(Robot& robot)
  {
    if( ! _lockedBody ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockBody",
                             "LOCKED");

      robot.GetMoveComponent().LockAnimTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK));
      _lockedBody = true;
    }
  }

  void BehaviorBlockPlay::BodyShouldBeUnlocked(Robot& robot)
  {
    if( _lockedBody ) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.AnimLockBody",
                             "UNLOCKED");

      robot.GetMoveComponent().UnlockAnimTracks(static_cast<u8>(AnimTrackFlag::BODY_TRACK));
      _lockedBody = false;
    }
  }


  void BehaviorBlockPlay::TrackBlockWithLift(Robot& robot, const Pose3d& objectPose)
  {
    // get the pose of the block WRT the lift joint
    Pose3d liftToObjectPose;
    if( objectPose.GetWithRespectTo(robot.GetLiftBasePose(), liftToObjectPose) ) {

      const float sideAngle = atan2f( liftToObjectPose.GetTranslation().y(), liftToObjectPose.GetTranslation().x() );

      float targetHeight = LIFT_HEIGHT_LOWDOCK;

      if( robot.GetHeadAngle() > _minHeadAngleforLiftUp_rads &
          liftToObjectPose.GetTranslation().x() <= _maxObjectDistToMoveLift) {
        if( std::abs(sideAngle) < _minTrackingAngleToMove_rads ) {
          targetHeight = _highLiftHeight;
        }
        else {
          // within range, but bad angle, so leave the lift where it is and let the tracking controller turn us
          targetHeight = robot.GetLiftHeight();
        }
      }

      if( ! NEAR( robot.GetLiftHeight(), targetHeight, 5.0f ) ) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.Move",
                               "new target height: %fmm",
                               targetHeight);
        
        // queue this action in the animation slot to avoid stomping on tracking
        MoveLiftToHeightAction* liftAction = new MoveLiftToHeightAction(targetHeight);
        robot.GetActionList().QueueActionAtEnd(Robot::FaceAnimationSlot, liftAction);
      }

      
      // check if we should move forward
      if( !_isDrivingForward &&
          liftToObjectPose.GetTranslation().x() >= _minObjectDistanceToMove &&
          std::abs(sideAngle) < _minTrackingAngleToMove_rads ) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.DriveForward",
                               "x dist = %f, side angle = %fdeg, driving forward",
                               liftToObjectPose.GetTranslation().x(),
                               RAD_TO_DEG(sideAngle));

        DriveStraightAction* driveAction = new DriveStraightAction(_distToDriveForwardWhileTracking,
                                                                   _speedToDriveForwardWhileTracking);
        // drive actions go in the main slot
        robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, driveAction);
        _driveForwardActionTag = driveAction->GetTag();

        // resume tracking after driving forward
        TrackObjectAction* trackingAction = new TrackObjectAction(_trackedObject);
        robot.GetActionList().QueueActionAtEnd(Robot::DriveAndManipulateSlot, trackingAction);
              
        _isDrivingForward = true;
      }
      else if ( !_isDrivingForward ) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.NoDrive",
                               "not driving forward because dist = %f, side angle = %fdeg",
                               liftToObjectPose.GetTranslation().x(),
                               RAD_TO_DEG(sideAngle));
      }
    }
    else {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.NoPose",
                             "couldn't get object pose WRT lift base.");
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.NoPose.Object",
                             "object graph: %s",
                             objectPose.GetNamedPathToOrigin(false).c_str());
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR, "BehaviorBlockPlay.TrackBlockWithLift.NoPose.LiftBase",
                             "lift base graph: %s",
                             robot.GetLiftBasePose().GetNamedPathToOrigin(false).c_str());
    }
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
    }

    _noFacesStartTime = -1.0;
    
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

  Result BehaviorBlockPlay::HandleObjectMoved(const Robot& robot, const ObjectMoved &msg)
  {
    ObjectID objectID;
    objectID = msg.objectID;

    auto it = _objectsToIgnore.find(objectID);
    if( it != _objectsToIgnore.end() ) {
      PRINT_NAMED_INFO("BehaviorBlockPlay.IgnoreObject", "re-considering object %d because it moved",
                       objectID.GetValue());
      _objectsToIgnore.erase(it);

      // flag this so we turn off lights as soon as we can
      _objectsToTurnOffLights.push_back(objectID);
    }
    
    return RESULT_OK;
  }  

  void BehaviorBlockPlay::StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback)
  {
    _lastActionTag = action->GetTag();
    _actionResultCallback = callback;
    robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, action);
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
    
    PlayAnimationAction* animAction = new PlayAnimationAction(animName.c_str());

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BLOCK_PLAY_BEHAVIOR,
                           "BehaviorBlockPlay.PlayAnimation",
                           "[%d] %s %s",
                           animAction->GetTag(),
                           animName.c_str(),
                           sequential ? "sequentially" : "in parallel");

    if( sequential ) {
      _animActionTags[animAction->GetTag()] = animName;
      robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, animAction);
    }
    else {
      robot.GetActionList().QueueActionAtEnd(Robot::FaceAnimationSlot, animAction);
    }
  }
  
  void BehaviorBlockPlay::SetDriveToObjectSounds(IDriveToInteractWithObject* action)
  {
    // Set sounds based on whether this is the first try or not
    if(_attemptCounter == 0) {
      action->SetSounds("ID_AlignToObject_Content_Start",
                        "ID_AlignToObject_Content_Drive",
                        "ID_AlignToObject_Content_Stop");
    } else {
      action->SetSounds("ID_AlignToObject_Frustrated_Start",
                        "ID_AlignToObject_Frustrated_Drive",
                        "ID_AlignToObject_Frustrated_Stop");
    }
    // TODO: More granularity in frustration level? (Hopefully don't need it; few failures!)
  }

  
} // namespace Cozmo
} // namespace Anki
