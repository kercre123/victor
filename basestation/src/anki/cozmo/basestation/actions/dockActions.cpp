/**
 * File: dockActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "dockActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/helpers/templateHelpers.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/charger.h"

namespace Anki {
  
  namespace Cozmo {
    
    // Helper function for computing the distance-to-preActionPose threshold,
    // given how far robot is from actionObject
    f32 ComputePreActionPoseDistThreshold(const Pose3d& preActionPose,
                                          const ActionableObject* actionObject,
                                          const Radians& preActionPoseAngleTolerance)
    {
      assert(actionObject != nullptr);
      
      if(preActionPoseAngleTolerance > 0.f) {
        // Compute distance threshold to preaction pose based on distance to the
        // object: the further away, the more slop we're allowed.
        Pose3d objectWrtRobot;
        if(false == actionObject->GetPose().GetWithRespectTo(preActionPose, objectWrtRobot)) {
          PRINT_NAMED_ERROR("IDockAction.Init.ObjectPoseOriginProblem",
                            "Could not get object %d's pose w.r.t. _robot.",
                            actionObject->GetID().GetValue());
          return -1.f;
        }
        
        const f32 objectDistance = objectWrtRobot.GetTranslation().Length();
        const f32 preActionPoseDistThresh = objectDistance * std::sin(preActionPoseAngleTolerance.ToFloat());
        
        PRINT_NAMED_INFO("IDockAction.Init.DistThresh",
                         "At a distance of %.1fmm, will use pre-dock pose distance threshold of %.1fmm",
                         objectDistance, preActionPoseDistThresh);
        
        return preActionPoseDistThresh;
      } else {
        return -1.f;
      }
    }

    #pragma mark ---- IDockAction ----

    IDockAction::IDockAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IAction(robot)
    , _dockObjectID(objectID)
    , _useManualSpeed(useManualSpeed)
    {
      
    }
    
    IDockAction::~IDockAction()
    {
      // Make sure we back to looking for markers (and stop tracking) whenever
      // and however this action finishes
      _robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
      _robot.GetVisionComponent().EnableMode(VisionMode::Tracking, false);
      
      // Abort anything that shouldn't still be running
      if(_robot.IsTraversingPath()) {
        _robot.AbortDrivingToPose();
      }
      if(_robot.IsPickingOrPlacing()) {
        _robot.AbortDocking();
      }
      
      // Stop squinting
      _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_squintLayerTag, 250);
      
      Util::SafeDelete(_faceAndVerifyAction);
    }

    void IDockAction::SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2)
    {
      _dockSpeed_mmps = speed_mmps; _dockAccel_mmps2 = accel_mmps2;
    }

    void IDockAction::SetSpeed(f32 speed_mmps)
    {
      _dockSpeed_mmps = speed_mmps;
    }

    void IDockAction::SetAccel(f32 accel_mmps2)
    {
      _dockAccel_mmps2 = accel_mmps2;
    }

    void IDockAction::SetPlacementOffset(f32 offsetX_mm, f32 offsetY_mm, f32 offsetAngle_rad)
    {
      _placementOffsetX_mm = offsetX_mm;
      _placementOffsetY_mm = offsetY_mm;
      _placementOffsetAngle_rad = offsetAngle_rad;
    }

    void IDockAction::SetPlaceOnGround(bool placeOnGround)
    {
      _placeObjectOnGroundIfCarrying = placeOnGround;
    }

    void IDockAction::SetPreActionPoseAngleTolerance(Radians angleTolerance)
    {
      _preActionPoseAngleTolerance = angleTolerance;
    }

    void IDockAction::SetPostDockLiftMovingAnimation(const std::string& animName)
    {
      _liftMovingAnimation = animName;
    }

    ActionResult IDockAction::Init()
    {
      _waitToVerifyTime = -1.f;
      
      // Make sure the object we were docking with still exists in the world
      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      if(dockObject == nullptr) {
        PRINT_NAMED_ERROR("IDockAction.Init.ActionObjectNotFound",
                          "Action object with ID=%d no longer exists in the world.",
                          _dockObjectID.GetValue());
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return ActionResult::FAILURE_ABORT;
      }
      
      // select the object so it shows up properly in viz
      _robot.GetBlockWorld().SelectObject(_dockObjectID);
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<PreActionPose> preActionPoses;
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      _robot.GetBlockWorld().GetObstacles(obstacles);
      dockObject->GetCurrentPreActionPoses(preActionPoses, {GetPreActionType()},
                                           std::set<Vision::Marker::Code>(), obstacles, nullptr, _placementOffsetX_mm);
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_ERROR("IDockAction.Init.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.",
                          _dockObjectID.GetValue());
        _interactionResult = ObjectInteractionResult::NO_PREACTION_POSES;
        return ActionResult::FAILURE_ABORT;
      }
      
      const Point2f currentXY(_robot.GetPose().GetTranslation().x(),
                              _robot.GetPose().GetTranslation().y());
      
      //float closestDistSq = std::numeric_limits<float>::max();
      Point2f closestPoint(std::numeric_limits<float>::max());
      size_t closestIndex = preActionPoses.size();
      float closestDistSq = std::numeric_limits<float>::max();
      
      for(size_t index=0; index < preActionPoses.size(); ++index) {
        Pose3d preActionPose;
        if(preActionPoses[index].GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), preActionPose) == false) {
          PRINT_NAMED_WARNING("IDockAction.Init.PreActionPoseOriginProblem",
                              "Could not get pre-action pose w.r.t. robot parent.");
        }
        
        const Point2f preActionXY(preActionPose.GetTranslation().x(),
                                  preActionPose.GetTranslation().y());
        const Point2f dist = (currentXY - preActionXY);
        const float distSq = dist.LengthSq();
        
        PRINT_NAMED_DEBUG("IDockAction.Init.CheckPoint",
                          "considering point (%f, %f) dist = %f",
                          dist.x(), dist.y(),
                          dist.Length());
        
        if(distSq < closestDistSq) {
          closestPoint = dist.GetAbs();
          closestIndex  = index;
          closestDistSq = distSq;
        }
      }
      
      PRINT_NAMED_INFO("IDockAction.Init.ClosestPoint",
                       "Closest point (%f, %f) robot (%f, %f) dist = %f",
                       closestPoint.x(), closestPoint.y(),
                       currentXY.x(), currentXY.y(),
                       closestPoint.Length());
      
      //const f32 closestDist = sqrtf(closestDistSq);
      
      // by default, even if we aren't checking for pre-dock poses, we shouldn't be too far away, otherwise we
      // may be selecting a different marker / face to dock with
      f32 preActionPoseDistThresh = DEFAULT_PREDOCK_POSE_DISTANCE_MM * 1.1f;
      
      if (_doNearPredockPoseCheck) {
        preActionPoseDistThresh = ComputePreActionPoseDistThreshold(_robot.GetPose(), dockObject,
                                                                    _preActionPoseAngleTolerance);
      }
      
      if(preActionPoseDistThresh > 0.f && closestPoint > preActionPoseDistThresh) {
        PRINT_NAMED_INFO("IDockAction.Init.TooFarFromGoal",
                        "Robot is too far from pre-action pose (%.1fmm, %.1fmm).",
                        closestPoint.x(), closestPoint.y());
        _interactionResult = ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE;
        return ActionResult::FAILURE_RETRY;
      }
      
      if(SelectDockAction(dockObject) != RESULT_OK) {
        PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.DockActionSelectionFailure",
                          "");
        // NOTE: SelectDockAction should set _interactionResult on failure
        return ActionResult::FAILURE_ABORT;
      }
      
      // Specify post-dock lift motion callback to play sound
      using namespace RobotInterface;
      auto liftSoundLambda = [this](const AnkiEvent<RobotToEngine>& event)
      {
        if (!_liftMovingAnimation.empty()) {
          // Check that the animation only has sound keyframes
          const Animation* anim = _robot.GetCannedAnimation(_liftMovingAnimation);
          if (nullptr != anim) {
            auto & headTrack        = anim->GetTrack<HeadAngleKeyFrame>();
            auto & liftTrack        = anim->GetTrack<LiftHeightKeyFrame>();
            auto & bodyTrack        = anim->GetTrack<BodyMotionKeyFrame>();
            
            if (!headTrack.IsEmpty() || !liftTrack.IsEmpty() || !bodyTrack.IsEmpty()) {
              PRINT_NAMED_WARNING("IDockAction.MovingLiftPostDockHandler.AnimHasMotion",
                                  "Animation must contain only sound.");
              return;
            }
            
            // Check that the action matches the current action
            DockAction recvdAction = event.GetData().Get_movingLiftPostDock().action;
            if (_dockAction != recvdAction) {
              PRINT_NAMED_WARNING("IDockAction.MovingLiftPostDockHandler.ActionMismatch",
                                  "Expected %u, got %u. Ignoring.",
                                  (u32)_dockAction, (u32)recvdAction);
              return;
            }
            
            // Play the animation
            PRINT_NAMED_INFO("IDockAction.MovingLiftPostDockHandler",
                             "Playing animation %s ",
                             _liftMovingAnimation.c_str());
            _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, new PlayAnimationAction(_robot, _liftMovingAnimation, 1, false));
          } else {
            PRINT_NAMED_WARNING("IDockAction.MovingLiftPostDockHandler.InvalidAnimation",
                                "Could not find animation %s",
                                _liftMovingAnimation.c_str());
          }
        }
      };
      _liftMovingSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(), RobotToEngineTag::movingLiftPostDock, liftSoundLambda);
      
      
      if (_doNearPredockPoseCheck) {
        PRINT_NAMED_INFO("IDockAction.Init.BeginDocking",
                         "Robot is within (%.1fmm,%.1fmm) of the nearest pre-action pose, "
                         "proceeding with docking.", closestPoint.x(), closestPoint.y());
      } else {
        PRINT_NAMED_INFO("IDockAction.Init.BeginDocking",
                         "Proceeding with docking.");
      }
      
      // Set dock markers
      _dockMarker = preActionPoses[closestIndex].GetMarker();
      _dockMarker2 = GetDockMarker2(preActionPoses, closestIndex);

      // Set up a visual verification action to make sure we can still see the correct
      // marker of the selected object before proceeding
      // NOTE: This also disables tracking head to object if there was any
      _faceAndVerifyAction = new FaceObjectAction(_robot,
                                                  _dockObjectID,
                                                  _dockMarker->GetCode(),
                                                  0, true, false);

      // Disable the visual verification from issuing a completion signal
      _faceAndVerifyAction->ShouldEmitCompletionSignal(false);
      _faceAndVerifyAction->ShouldSuppressTrackLocking(true);
      
      // Go ahead and Update the FaceObjectAction once now, so we don't
      // waste a tick doing so in CheckIfDone (since this is the first thing
      // that will be done in CheckIfDone anyway)
      ActionResult faceObjectResult = _faceAndVerifyAction->Update();

      if(ActionResult::SUCCESS == faceObjectResult ||
         ActionResult::RUNNING == faceObjectResult)
      {
        return ActionResult::SUCCESS;
      } else {
        return faceObjectResult;
      }
    } // Init()

    ActionResult IDockAction::CheckIfDone()
    {
      ActionResult actionResult = ActionResult::RUNNING;
      
      // Wait for visual verification to complete successfully before telling
      // robot to dock and continuing to check for completion
      if(_faceAndVerifyAction != nullptr) {
        actionResult = _faceAndVerifyAction->Update();
        if(actionResult == ActionResult::RUNNING) {
          return actionResult;
        } else {
          if(actionResult == ActionResult::SUCCESS) {
            // Finished with visual verification:
            Util::SafeDelete(_faceAndVerifyAction);
            actionResult = ActionResult::RUNNING;
            
            PRINT_NAMED_INFO("IDockAction.DockWithObjectHelper.BeginDocking", "Docking with marker %d (%s) using action %s.",
                             _dockMarker->GetCode(), Vision::MarkerTypeStrings[_dockMarker->GetCode()], DockActionToString(_dockAction));
            if(_robot.DockWithObject(_dockObjectID,
                                      _dockSpeed_mmps,
                                      _dockAccel_mmps2,
                                      _dockMarker, _dockMarker2,
                                      _dockAction,
                                      _placementOffsetX_mm,
                                      _placementOffsetY_mm,
                                      _placementOffsetAngle_rad,
                                      _useManualSpeed) == RESULT_OK)
            {
              //NOTE: Any completion (success or failure) after this point should tell
              // the robot to stop tracking and go back to looking for markers!
              _wasPickingOrPlacing = false;
            } else {
              return ActionResult::FAILURE_ABORT;
            }
            
          } else {
            PRINT_NAMED_ERROR("IDockAction.CheckIfDone.VisualVerifyFailed",
                              "VisualVerification of object failed, stopping IDockAction.");
            _interactionResult = ObjectInteractionResult::VISUAL_VERIFICATION_FAILED;
            return actionResult;
          }
        }
      }
      
      if (!_wasPickingOrPlacing) {
        // We have to see the robot went into pick-place mode once before checking
        // to see that it has finished picking or placing below. I.e., we need to
        // know the robot got the DockWithObject command sent in Init().
        _wasPickingOrPlacing = _robot.IsPickingOrPlacing();
        
        if(_wasPickingOrPlacing) {
          // Apply continuous eye squint if we have just now started picking and placing
          AnimationStreamer::FaceTrack squintLayer;
          ProceduralFace squintFace;
          
          const f32 DockSquintScaleY = 0.35f;
          const f32 DockSquintScaleX = 1.05f;
          squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleY, DockSquintScaleY);
          squintFace.SetParameterBothEyes(ProceduralFace::Parameter::EyeScaleX, DockSquintScaleX);
          squintFace.SetParameterBothEyes(ProceduralFace::Parameter::UpperLidAngle, -10);
          
          squintLayer.AddKeyFrameToBack(ProceduralFaceKeyFrame()); // need start frame at t=0 to get interpolation
          squintLayer.AddKeyFrameToBack(ProceduralFaceKeyFrame(squintFace, 250));
          _squintLayerTag = _robot.GetAnimationStreamer().AddPersistentFaceLayer("DockSquint", std::move(squintLayer));
        }
      }
      else if (!_robot.IsPickingOrPlacing() && !_robot.GetMoveComponent().IsMoving())
      {
        const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        
        // While head is moving to verification angle, this shouldn't count towards the waitToVerifyTime
        if (_robot.GetMoveComponent().IsHeadMoving()) {
          _waitToVerifyTime = -1;
        }
        
        // Set the verification time if not already set
        if(_waitToVerifyTime < 0.f) {
          _waitToVerifyTime = currentTime + GetVerifyDelayInSeconds();
        }
        
        // Stopped executing docking path, and should have backed out by now,
        // and have head pointed at an angle to see where we just placed or
        // picked up from. So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        if(currentTime >= _waitToVerifyTime) {
          //PRINT_NAMED_INFO("IDockAction.CheckIfDone",
          //                 "Robot has stopped moving and picking/placing. Will attempt to verify success.");
          
          actionResult = Verify();
        }
      }
      
      if(ActionResult::SUCCESS == actionResult) {
        _interactionResult = ObjectInteractionResult::SUCCESS;
      }
      
      return actionResult;
    } // CheckIfDone()
    
#pragma mark ---- PopAWheelieAction ----
    
    PopAWheelieAction::PopAWheelieAction(Robot& robot,
                                         ObjectID objectID,
                                         const bool useManualSpeed)
    : IDockAction(robot, objectID, useManualSpeed)
    {
      
    }
    
    const std::string& PopAWheelieAction::GetName() const
    {
      static const std::string name("PopAWheelieAction");
      return name;
    }
    
    RobotActionType PopAWheelieAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_POP_A_WHEELIE:
          return RobotActionType::POP_A_WHEELIE;
          
        default:
          PRINT_NAMED_WARNING("PopAWheelieAction",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void PopAWheelieAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      switch(_dockAction)
      {
        case DockAction::DA_POP_A_WHEELIE:
        {
          if(_robot.IsCarryingObject()) {
            PRINT_NAMED_WARNING("PopAWheelieAction.EmitCompletionSignal",
                                "Expecting robot to think it's not carrying object for roll action.");
          } else {
            info.numObjects = 1;
            info.objectIDs.fill(-1);
            info.objectIDs[0] = _dockObjectID;
          }
          break;
        }
        default:
          PRINT_NAMED_WARNING("PopAWheelieAction.EmitCompletionSignal",
                              "Dock action not set before filling completion signal.");
      }
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result PopAWheelieAction::SelectDockAction(ActionableObject* object)
    {
      Pose3d objectPose;
      if(object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), objectPose) == false) {
        PRINT_NAMED_WARNING("PopAWheelieAction.SelectDockAction.PoseWrtFailed",
                            "Could not get pose of dock object w.r.t. robot's parent.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = objectPose.GetTranslation().z() - _robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_POP_A_WHEELIE;
      
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      // TODO: There might be ways to roll high blocks when not carrying object and low blocks when carrying an object.
      //       Do them later.
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        PRINT_STREAM_INFO("PopAWheelieAction.SelectDockAction", "Object is too high to pop-a-wheelie. Aborting.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      } else if (_robot.IsCarryingObject()) {
        PRINT_STREAM_INFO("PopAWheelieAction.SelectDockAction", "Can't pop-a-wheelie while carrying an object.");
        _interactionResult = ObjectInteractionResult::STILL_CARRYING;
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult PopAWheelieAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_POP_A_WHEELIE:
        {
          if(_robot.GetLastPickOrPlaceSucceeded()) {
            // Check that the robot is sufficiently pitched up
            if (_robot.GetPitchAngle() < 1.f) {
              PRINT_NAMED_INFO("PopAWheelieAction.Verify.PitchAngleTooSmall",
                               "Robot pitch angle expected to be higher (measured %f rad)",
                               _robot.GetPitchAngle());
              result = ActionResult::FAILURE_RETRY;
            } else {
              result = ActionResult::SUCCESS;
            }
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track.
            PRINT_NAMED_INFO("PopAWheelieAction.Verify.DockingFailed",
                             "Robot reported pop-a-wheelie failure. Assuming docking failed");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // DA_POP_A_WHEELIE
          
          
        default:
          PRINT_NAMED_WARNING("PopAWheelieAction.Verify.ReachedDefaultCase",
                              "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
#pragma mark ---- AlignWithObjectAction ----
    
    AlignWithObjectAction::AlignWithObjectAction(Robot& robot,
                                                 ObjectID objectID,
                                                 const f32 distanceFromMarker_mm,
                                                 const bool useManualSpeed)
    : IDockAction(robot, objectID, useManualSpeed)
    {
      SetPlacementOffset(distanceFromMarker_mm, 0, 0);
    }
    
    AlignWithObjectAction::~AlignWithObjectAction()
    {
      
    }
    
    const std::string& AlignWithObjectAction::GetName() const
    {
      static const std::string name("AlignWithObjectAction");
      return name;
    }
    
    
    void AlignWithObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs.fill(-1);
      info.objectIDs[0] = _dockObjectID;
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    
    Result AlignWithObjectAction::SelectDockAction(ActionableObject* object)
    {
      _dockAction = DockAction::DA_ALIGN;
      return RESULT_OK;
    } // SelectDockAction()
    
    
    ActionResult AlignWithObjectAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_ALIGN:
        {
          // What does it mean to verify this action other than to complete
          if (!_robot.IsPickingOrPlacing() && !_robot.IsTraversingPath()) {
            PRINT_STREAM_INFO("AlignWithObjectAction.Verify", "Align with object SUCCEEDED!");
            result = ActionResult::SUCCESS;
          }
          break;
        } // ALIGN
          
        default:
          PRINT_NAMED_ERROR("AlignWithObjectAction.Verify.ReachedDefaultCase",
                            "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
#pragma mark ---- PickupObjectAction ----
    
    PickupObjectAction::PickupObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IDockAction(robot, objectID, useManualSpeed)
    {
      SetPostDockLiftMovingAnimation("LiftEffortPickup");
    }
    
    const std::string& PickupObjectAction::GetName() const
    {
      static const std::string name("PickupObjectAction");
      return name;
    }
    
    RobotActionType PickupObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_HIGH:
          return RobotActionType::PICKUP_OBJECT_HIGH;
          
        case DockAction::DA_PICKUP_LOW:
          return RobotActionType::PICKUP_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("PickupObjectAction.GetType",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void PickupObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_HIGH:
        case DockAction::DA_PICKUP_LOW:
        {
          if(!_robot.IsCarryingObject()) {
            PRINT_NAMED_ERROR("PickupObjectAction.EmitCompletionSignal",
                              "Expecting robot to think it's carrying object for pickup action.");
          } else {
            const std::set<ObjectID> carriedObjects = _robot.GetCarryingObjects();
            info.numObjects = carriedObjects.size();
            info.objectIDs.fill(-1);
            info.objectIDs[0] = _dockObjectID;
            
            u8 objectCnt = 0;
            for (auto& objID : carriedObjects) {
              info.objectIDs[objectCnt++] = objID.GetValue();
            }
            
          }
          break;
        }
        default:
          PRINT_NAMED_ERROR("PickupObjectAction.EmitCompletionSignal",
                            "Dock action not set before filling completion signal.");
      }
      
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result PickupObjectAction::SelectDockAction(ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      // Make it w.r.t. robot's parent so we can compare heights fairly.
      if(object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), _dockObjectOrigPose) == false) {
        PRINT_NAMED_ERROR("PickupObjectAction.SelectDockAction.PoseWrtFailed",
                          "Could not get pose of dock object w.r.t. robot parent.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - _robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_PICKUP_LOW;
      
      if (_robot.IsCarryingObject()) {
        PRINT_NAMED_INFO("PickupObjectAction.SelectDockAction", "Already carrying object. Can't pickup object. Aborting.");
        _interactionResult = ObjectInteractionResult::STILL_CARRYING;
        return RESULT_FAIL;
      } else if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { // TODO: Stop using constant ROBOT_BOUNDING_Z for this
        _dockAction = DockAction::DA_PICKUP_HIGH;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult PickupObjectAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_LOW:
        case DockAction::DA_PICKUP_HIGH:
        {
          if(_robot.IsCarryingObject() == false) {
            PRINT_NAMED_ERROR("PickupObjectAction.Verify.RobotNotCarryingObject",
                              "Expecting robot to think it's carrying an object at this point.");
            _interactionResult = ObjectInteractionResult::NOT_CARRYING;
            result = ActionResult::FAILURE_RETRY;
            break;
          }
          
          BlockWorld& blockWorld = _robot.GetBlockWorld();
          
          // We should _not_ still see a object with the
          // same type as the one we were supposed to pick up in that
          // block's original position because we should now be carrying it.
          ObservableObject* carryObject = blockWorld.GetObjectByID(_robot.GetCarryingObject());
          if(carryObject == nullptr) {
            PRINT_NAMED_ERROR("PickupObjectAction.Verify.CarryObjectNoLongerExists",
                              "Object %d we were carrying no longer exists in the world.",
                              _robot.GetCarryingObject().GetValue());
            _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
            result = ActionResult::FAILURE_ABORT;
            break;
          }
          
          const BlockWorld::ObjectsMapByID_t& objectsWithType = blockWorld.GetExistingObjectsByType(carryObject->GetType());
          
          // Robot's pose parent could have changed due to delocalization.
          // Assume it's actual pose is relatively accurate w.r.t. that original
          // pose (when dockObjectOrigPose was stored) and update the parent so
          // that we can do IsSameAs checks below.
          _dockObjectOrigPose.SetParent(_robot.GetPose().GetParent());
          
          Vec3f Tdiff;
          Radians angleDiff;
          ObservableObject* objectInOriginalPose = nullptr;
          for(auto object : objectsWithType)
          {
            // TODO: is it safe to always have useAbsRotation=true here?
            Vec3f Tdiff;
            Radians angleDiff;
            if(object.second->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose, // dock obj orig pose is w.r.t. robot
                                                               carryObject->GetRotationAmbiguities(),
                                                               carryObject->GetSameDistanceTolerance()*0.5f,
                                                               carryObject->GetSameAngleTolerance(), true,
                                                               Tdiff, angleDiff))
            {
              PRINT_NAMED_INFO("PickupObjectAction.Verify.ObjectInOrigPose",
                               "Seeing object %d in original pose. (Tdiff = (%.1f,%.1f,%.1f), "
                               "AngleDiff=%.1fdeg",
                               object.first.GetValue(),
                               Tdiff.x(), Tdiff.y(), Tdiff.z(), angleDiff.getDegrees());
              objectInOriginalPose = object.second;
              break;
            }
          }
          
          if(objectInOriginalPose != nullptr)
          {
            // Must not actually be carrying the object I thought I was!
            // Put the object I thought I was carrying in the position of the
            // object I matched to it above, and then delete that object.
            // (This prevents a new object with different ID being created.)
            if(carryObject->GetID() != objectInOriginalPose->GetID()) {
              PRINT_NAMED_INFO("PickupObjectAction.Verify",
                               "Moving carried object to object seen in original pose "
                               "and deleting that object (ID=%d).",
                               objectInOriginalPose->GetID().GetValue());
              carryObject->SetPose(objectInOriginalPose->GetPose());
              blockWorld.DeleteObject(objectInOriginalPose->GetID());
            }
            _robot.UnSetCarryingObjects();
            
            PRINT_NAMED_INFO("PickupObjectAction.Verify",
                             "Object pick-up FAILED! (Still seeing object in same place.)");
            _interactionResult = ObjectInteractionResult::NOT_CARRYING;
            result = ActionResult::FAILURE_RETRY;
          } else {
            PRINT_NAMED_INFO("PickupObjectAction.Verify", "Object pick-up SUCCEEDED!");
            result = ActionResult::SUCCESS;
          }
          break;
        } // PICKUP
          
        default:
          PRINT_NAMED_ERROR("PickupObjectAction.Verify.ReachedDefaultCase",
                            "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)

      return result;
      
    } // Verify()
    
#pragma mark ---- PlaceObjectOnGroundAction ----
    
    PlaceObjectOnGroundAction::PlaceObjectOnGroundAction(Robot& robot)
    : IAction(robot)
    {
      
    }
    
    const std::string& PlaceObjectOnGroundAction::GetName() const
    {
      static const std::string name("PlaceObjectOnGroundAction");
      return name;
    }
    
    ActionResult PlaceObjectOnGroundAction::Init()
    {
      ActionResult result = ActionResult::RUNNING;
      
      // Robot must be carrying something to put something down!
      if(_robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d executing PlaceObjectOnGroundAction but not carrying object.", _robot.GetID());
        _interactionResult = ObjectInteractionResult::NOT_CARRYING;
        result = ActionResult::FAILURE_ABORT;
      } else {
        
        _carryingObjectID  = _robot.GetCarryingObject();
        _carryObjectMarker = _robot.GetCarryingMarker();
        
        if(_robot.PlaceObjectOnGround() == RESULT_OK)
        {
          result = ActionResult::SUCCESS;
        } else {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckPreconditions.SendPlaceObjectOnGroundFailed",
                            "Robot's SendPlaceObjectOnGround method reported failure.");
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
        }
        
        _faceAndVerifyAction = new FaceObjectAction(_robot, _carryingObjectID,
                                                    _carryObjectMarker->GetCode(), 0, true, false);
        
        _faceAndVerifyAction->ShouldEmitCompletionSignal(false);
        _faceAndVerifyAction->ShouldSuppressTrackLocking(true);
        
      } // if/else IsCarryingObject()
      
      // If we were moving, stop moving.
      _robot.GetMoveComponent().StopAllMotors();
      
      return result;
      
    } // CheckPreconditions()
    
    ActionResult PlaceObjectOnGroundAction::CheckIfDone()
    {
      ActionResult actionResult = ActionResult::RUNNING;
      
      // Wait for robot to report it is done picking/placing and that it's not
      // moving
      if (!_robot.IsPickingOrPlacing() && !_robot.GetMoveComponent().IsMoving())
      {
        // Stopped executing docking path, and should have placed carried block
        // and backed out by now, and have head pointed at an angle to see
        // where we just placed or picked up from.
        // So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        
        actionResult = _faceAndVerifyAction->Update();
        
        if(actionResult != ActionResult::RUNNING && actionResult != ActionResult::SUCCESS) {
          PRINT_NAMED_ERROR("PlaceObjectOnGroundAction.CheckIfDone",
                            "FaceAndVerify action reported failure, just deleting object %d.",
                            _carryingObjectID.GetValue());
          _robot.GetBlockWorld().ClearObject(_carryingObjectID);
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
        }
        
      } // if robot is not picking/placing or moving
      
      if(ActionResult::SUCCESS == actionResult) {
        _interactionResult = ObjectInteractionResult::SUCCESS;
      }
      
      return actionResult;
      
    } // CheckIfDone()
    
    void  PlaceObjectOnGroundAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      info.numObjects = 1;
      info.objectIDs[0] = _carryingObjectID;
      info.result = _interactionResult;
      
      completionUnion.Set_objectInteractionCompleted(std::move(info));
    }
    
#pragma mark ---- PlaceObjectOnGroundAtPoseAction ----
    
    PlaceObjectOnGroundAtPoseAction::PlaceObjectOnGroundAtPoseAction(Robot& robot,
                                                                     const Pose3d& placementPose,
                                                                     const PathMotionProfile motionProfile,
                                                                     const bool useExactRotation,
                                                                     const bool useManualSpeed)
    : CompoundActionSequential(robot, {
      new DriveToPlaceCarriedObjectAction(robot,
                                          placementPose,
                                          true,
                                          motionProfile,
                                          useExactRotation,
                                          useManualSpeed),
      new PlaceObjectOnGroundAction(robot)})
    {
      
    }
    
    void PlaceObjectOnGroundAtPoseAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      // Use the completion union of the constituent PlaceObjectOnGround action
      completionUnion = _completedActionInfoStack.back().first;
    }
    
#pragma mark ---- PlaceRelObjectAction ----
    
    PlaceRelObjectAction::PlaceRelObjectAction(Robot& robot,
                                               ObjectID objectID,
                                               const bool placeOnGround,
                                               const f32 placementOffsetX_mm,
                                               const bool useManualSpeed)
    : IDockAction(robot, objectID, useManualSpeed)
    {
      SetPlacementOffset(placementOffsetX_mm, 0, 0);
      SetPlaceOnGround(placeOnGround);
      SetPostDockLiftMovingAnimation(placeOnGround ? "LiftEffortPlaceLow" : "LiftEffortPlaceHigh");
    }
    
    const std::string& PlaceRelObjectAction::GetName() const
    {
      static const std::string name("PlaceRelObjectAction");
      return name;
    }
    
    RobotActionType PlaceRelObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_PLACE_HIGH:
          return RobotActionType::PLACE_OBJECT_HIGH;
          
        case DockAction::DA_PLACE_LOW:
          return RobotActionType::PLACE_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("PlaceRelObjectAction.GetType",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void PlaceRelObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      
      switch(_dockAction)
      {
        case DockAction::DA_PLACE_HIGH:
        case DockAction::DA_PLACE_LOW:
        {
          // TODO: Be able to fill in more objects in the stack
          ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_dockObjectID);
          if(object == nullptr) {
            PRINT_NAMED_ERROR("PlaceRelObjectAction.EmitCompletionSignal",
                              "Docking object %d not found in world after placing.",
                              _dockObjectID.GetValue());
          } else {
            auto objectStackIter = info.objectIDs.begin();
            info.objectIDs.fill(-1);
            info.numObjects = 0;
            while(object != nullptr &&
                  info.numObjects < info.objectIDs.size())
            {
              *objectStackIter = object->GetID().GetValue();
              ++objectStackIter;
              ++info.numObjects;
              object = _robot.GetBlockWorld().FindObjectOnTopOf(*object, 15.f);
            }
            
          }
          break;
        }
        default:
          PRINT_NAMED_ERROR("PlaceRelObjectAction.EmitCompletionSignal",
                            "Dock action not set before filling completion signal.");
      }
      
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result PlaceRelObjectAction::SelectDockAction(ActionableObject* object)
    {
      if (!_robot.IsCarryingObject()) {
        PRINT_STREAM_INFO("PlaceRelObjectAction.SelectDockAction", "Can't place if not carrying an object. Aborting.");
        _interactionResult = ObjectInteractionResult::NOT_CARRYING;
        return RESULT_FAIL;
      }
      
      _dockAction = _placeObjectOnGroundIfCarrying ? DockAction::DA_PLACE_LOW : DockAction::DA_PLACE_HIGH;
      
      // Need to record the object we are currently carrying because it
      // will get unset when the robot unattaches it during placement, and
      // we want to be able to verify that we're seeing what we just placed.
      _carryObjectID     = _robot.GetCarryingObject();
      _carryObjectMarker = _robot.GetCarryingMarker();
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult PlaceRelObjectAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_PLACE_LOW:
        case DockAction::DA_PLACE_HIGH:
        {
          if(_robot.GetLastPickOrPlaceSucceeded()) {
            
            if(_robot.IsCarryingObject() == true) {
              PRINT_NAMED_ERROR("PlaceRelObjectAction.Verify",
                                "Expecting robot to think it's NOT carrying an object at this point.");
              _interactionResult = ObjectInteractionResult::STILL_CARRYING;
              return ActionResult::FAILURE_ABORT;
            }
            
            // If the physical robot thinks it succeeded, move the lift out of the
            // way, and attempt to visually verify
            if(_placementVerifyAction == nullptr) {
              _placementVerifyAction = new FaceObjectAction(_robot, _carryObjectID, Radians(0), true, false);
              _placementVerifyAction->ShouldSuppressTrackLocking(true);
              _verifyComplete = false;
              
              // Disable completion signals since this is inside another action
              _placementVerifyAction->ShouldEmitCompletionSignal(false);
              
              // Go ahead do the first update of the FaceObjectAction to get the
              // init "out of the way" rather than wasting a tick here
              result = _placementVerifyAction->Update();
              if(ActionResult::SUCCESS != result && ActionResult::RUNNING != result) {
                return result;
              }
            }
            
            result = _placementVerifyAction->Update();
            
            if(result != ActionResult::RUNNING) {
              
              // Visual verification is done
              Util::SafeDelete(_placementVerifyAction);
              
              if(result != ActionResult::SUCCESS) {
                if(_dockAction == DockAction::DA_PLACE_LOW) {
                  PRINT_NAMED_ERROR("PlaceRelObjectAction.Verify",
                                    "Robot thinks it placed the object low, but verification of placement "
                                    "failed. Not sure where carry object %d is, so deleting it.",
                                    _carryObjectID.GetValue());
                  
                  _robot.GetBlockWorld().ClearObject(_carryObjectID);
                } else {
                  assert(_dockAction == DockAction::DA_PLACE_HIGH);
                  PRINT_NAMED_ERROR("PlaceRelObjectAction.Verify",
                                    "Robot thinks it placed the object high, but verification of placement "
                                    "failed. Assuming we are still carrying object %d.",
                                    _carryObjectID.GetValue());
                  
                  _robot.SetObjectAsAttachedToLift(_carryObjectID, _carryObjectMarker);
                }
                
              }
              else if(_dockAction == DockAction::DA_PLACE_HIGH && !_verifyComplete) {
                
                // If we are placing high and verification succeeded, lower the lift
                _verifyComplete = true;
                
                if(result == ActionResult::SUCCESS) {
                  // Visual verification succeeded, drop lift (otherwise, just
                  // leave it up, since we are assuming we are still carrying the object)
                  _placementVerifyAction = new MoveLiftToHeightAction(_robot,
                                                                      MoveLiftToHeightAction::Preset::LOW_DOCK);
                  _placementVerifyAction->ShouldSuppressTrackLocking(true);
                  
                  // Disable completion signals since this is inside another action
                  _placementVerifyAction->ShouldEmitCompletionSignal(false);
                  
                  result = ActionResult::RUNNING;
                }
                
              }
            } else {
              // Mostly for debugging when placement verification is taking too long
              PRINT_NAMED_INFO("PlaceRelObjectAction.Verify.Waiting", "");
            } // if(result != ActionResult::RUNNING)
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track, so we are probably still holding the block
            PRINT_NAMED_ERROR("PlaceRelObjectAction.Verify",
                              "Robot reported placement failure. Assuming docking failed "
                              "and robot is still holding same block.");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // PLACE
          
        default:
          PRINT_NAMED_ERROR("PlaceRelObjectAction.Verify.ReachedDefaultCase",
                            "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
#pragma mark ---- RollObjectAction ----
    
    RollObjectAction::RollObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IDockAction(robot, objectID, useManualSpeed)
    {
      _dockAction = DockAction::DA_ROLL_LOW;
      SetPostDockLiftMovingAnimation("LiftEffortRoll");
    }
    
    const std::string& RollObjectAction::GetName() const
    {
      static const std::string name("RollObjectAction");
      return name;
    }
    
    RobotActionType RollObjectAction::GetType() const
    {
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
          return RobotActionType::ROLL_OBJECT_LOW;
          
        default:
          PRINT_NAMED_WARNING("RollObjectAction.GetType",
                              "Dock action not set before determining action type.");
          return RobotActionType::PICK_AND_PLACE_INCOMPLETE;
      }
    }
    
    void RollObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
        {
          if(_robot.IsCarryingObject()) {
            PRINT_NAMED_WARNING("RollObjectAction.EmitCompletionSignal",
                                "Expecting robot to think it's not carrying object for roll action.");
          }
          else {
            info.numObjects = 1;
            info.objectIDs.fill(-1);
            info.objectIDs[0] = _dockObjectID;
          }
          break;
        }
        default:
          PRINT_NAMED_WARNING("RollObjectAction.EmitCompletionSignal",
                              "Dock action not set before filling completion signal.");
      }
      
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result RollObjectAction::SelectDockAction(ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      // Make it w.r.t. robot's parent so we don't have to worry about differing origins later.
      if(object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), _dockObjectOrigPose) == false) {
        PRINT_NAMED_WARNING("RollObjectAction.SelectDockAction.PoseWrtFailed",
                            "Could not get pose of dock object w.r.t. robot's parent.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - _robot.GetPose().GetTranslation().z();
      
      // Get the top marker as this will be what needs to be seen for verification
      Block* block = dynamic_cast<Block*>(object);
      if (block == nullptr) {
        PRINT_NAMED_WARNING("RollObjectAction.SelectDockAction.NonBlock", "Only blocks can be rolled");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      Pose3d junk;
      _expectedMarkerPostRoll = &(block->GetTopMarker(junk));
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      // TODO: There might be ways to roll high blocks when not carrying object and low blocks when carrying an object.
      //       Do them later.
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        PRINT_STREAM_INFO("RollObjectAction.SelectDockAction", "Object is too high to roll. Aborting.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      } else if (_robot.IsCarryingObject()) {
        PRINT_STREAM_INFO("RollObjectAction.SelectDockAction", "Can't roll while carrying an object.");
        _interactionResult = ObjectInteractionResult::STILL_CARRYING;
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult RollObjectAction::Verify()
    {
      ActionResult result = ActionResult::RUNNING;
      
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
        {
          if(_robot.GetLastPickOrPlaceSucceeded()) {
            
            if(_robot.IsCarryingObject() == true) {
              PRINT_NAMED_WARNING("RollObjectAction::Verify",
                                  "Expecting robot to think it's NOT carrying an object at this point.");
              _interactionResult = ObjectInteractionResult::STILL_CARRYING;
              result = ActionResult::FAILURE_ABORT;
              break;
            }
            
            // If the physical robot thinks it succeeded, verify that the expected marker is being seen
            if(_rollVerifyAction == nullptr) {
              _rollVerifyAction = new VisuallyVerifyObjectAction(_robot, _dockObjectID, _expectedMarkerPostRoll->GetCode());
              _rollVerifyAction->ShouldSuppressTrackLocking(true);
              
              // Disable completion signals since this is inside another action
              _rollVerifyAction->ShouldEmitCompletionSignal(false);
              
              // Do one update step immediately after creating the action to get Init done
              result = _rollVerifyAction->Update();
            }
            
            if(result == ActionResult::RUNNING) {
              result = _rollVerifyAction->Update();
            }
            
            if(result != ActionResult::RUNNING) {
              
              // Visual verification is done
              Util::SafeDelete(_rollVerifyAction);
              
              if(result != ActionResult::SUCCESS) {
                PRINT_NAMED_INFO("RollObjectAction.Verify",
                                 "Robot thinks it rolled the object, but verification failed. ");
                result = ActionResult::FAILURE_ABORT;
              }
            } else {
              // Mostly for debugging when verification takes too long
              PRINT_NAMED_INFO("RollObjectAction.Verify.Waiting", "");
            } // if(result != ActionResult::RUNNING)
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track.
            PRINT_NAMED_WARNING("RollObjectAction.Verify",
                                "Robot reported roll failure. Assuming docking failed");
            // retry, since the block is hopefully still there
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // ROLL_LOW
          
          
        default:
          PRINT_NAMED_WARNING("RollObjectAction.Verify.ReachedDefaultCase",
                              "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
#pragma mark ---- AscendOrDescendRampAction ----
    
    AscendOrDescendRampAction::AscendOrDescendRampAction(Robot& robot, ObjectID rampID, const bool useManualSpeed)
    : IDockAction(robot, rampID, useManualSpeed)
    {
      
    }
    
    const std::string& AscendOrDescendRampAction::GetName() const
    {
      static const std::string name("AscendOrDescendRampAction");
      return name;
    }
    
    Result AscendOrDescendRampAction::SelectDockAction(ActionableObject* object)
    {
      Ramp* ramp = dynamic_cast<Ramp*>(object);
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("AscendOrDescendRampAction.SelectDockAction.NotRampObject",
                          "Could not cast generic ActionableObject into Ramp object.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      // Choose ascent or descent
      const Ramp::TraversalDirection direction = ramp->WillAscendOrDescend(_robot.GetPose());
      switch(direction)
      {
        case Ramp::ASCENDING:
          _dockAction = DockAction::DA_RAMP_ASCEND;
          break;
          
        case Ramp::DESCENDING:
          _dockAction = DockAction::DA_RAMP_DESCEND;
          break;
          
        case Ramp::UNKNOWN:
        default:
          result = RESULT_FAIL;
      }
      
      // Tell robot which ramp it will be using, and in which direction
      _robot.SetRamp(_dockObjectID, direction);
      
      return result;
      
    } // SelectDockAction()
    
    
    ActionResult AscendOrDescendRampAction::Verify()
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                       "Robot has completed going up/down ramp.");
      
      return ActionResult::SUCCESS;
    } // Verify()
    
#pragma mark ---- MountChargerAction ----
    
    MountChargerAction::MountChargerAction(Robot& robot, ObjectID chargerID, const bool useManualSpeed)
    : IDockAction(robot, chargerID, useManualSpeed)
    {
      
    }
    
    const std::string& MountChargerAction::GetName() const
    {
      static const std::string name("MountChargerAction");
      return name;
    }
    
    Result MountChargerAction::SelectDockAction(ActionableObject* object)
    {
      Charger* charger = dynamic_cast<Charger*>(object);
      if(charger == nullptr) {
        PRINT_NAMED_ERROR("MountChargerAction.SelectDockAction.NotChargerObject",
                          "Could not cast generic ActionableObject into Charger object.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      _dockAction = DockAction::DA_MOUNT_CHARGER;
      
      // Tell robot which charger it will be using
      _robot.SetCharger(_dockObjectID);
      
      return result;
      
    } // SelectDockAction()
    
    
    ActionResult MountChargerAction::Verify()
    {
      // Verify that robot is on charger
      if (_robot.IsOnCharger()) {
        PRINT_NAMED_INFO("MountChargerAction.Verify.MountingChargerComplete",
                         "Robot has mounted charger.");
        return ActionResult::SUCCESS;
      }
      return ActionResult::FAILURE_ABORT;
    } // Verify()

#pragma mark ---- CrossBridgeAction ----
    
    CrossBridgeAction::CrossBridgeAction(Robot& robot, ObjectID bridgeID, const bool useManualSpeed)
    : IDockAction(robot, bridgeID, useManualSpeed)
    {
      
    }
    
    const std::string& CrossBridgeAction::GetName() const
    {
      static const std::string name("CrossBridgeAction");
      return name;
    }
    
    const Vision::KnownMarker* CrossBridgeAction::GetDockMarker2(const std::vector<PreActionPose> &preActionPoses, const size_t closestIndex)
    {
      // Use the unchosen pre-crossing pose marker (the one at the other end of
      // the bridge) as dockMarker2
      assert(preActionPoses.size() == 2);
      size_t indexForOtherEnd = 1 - closestIndex;
      assert(indexForOtherEnd == 0 || indexForOtherEnd == 1);
      return preActionPoses[indexForOtherEnd].GetMarker();
    }
    
    Result CrossBridgeAction::SelectDockAction(ActionableObject* object)
    {
      _dockAction = DockAction::DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult CrossBridgeAction::Verify()
    {
      // TODO: Need some kind of verificaiton here?
      PRINT_NAMED_INFO("CrossBridgeAction.Verify.BridgeCrossingComplete",
                       "Robot has completed crossing a bridge.");
      return ActionResult::SUCCESS;
    } // Verify()
  }
}

