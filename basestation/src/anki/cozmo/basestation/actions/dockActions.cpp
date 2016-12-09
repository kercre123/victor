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

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/animationTriggerResponsesContainer.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "util/console/consoleInterface.h"
#include "util/helpers/templateHelpers.h"


namespace{
const float kMaxNegativeXPlacementOffset = 1.f;
  
}


namespace Anki {
  
  namespace Cozmo {
  
    // Which docking method actions should use
    CONSOLE_VAR(u32, kDefaultDockingMethod,"DockingMethod(B:0 T:1 H:2)", (u8)DockingMethod::BLIND_DOCKING);
    CONSOLE_VAR(u32, kPickupDockingMethod, "DockingMethod(B:0 T:1 H:2)", (u8)DockingMethod::HYBRID_DOCKING);
    CONSOLE_VAR(u32, kRollDockingMethod,   "DockingMethod(B:0 T:1 H:2)", (u8)DockingMethod::BLIND_DOCKING);
    CONSOLE_VAR(u32, kStackDockingMethod,  "DockingMethod(B:0 T:1 H:2)", (u8)DockingMethod::BLIND_DOCKING);
    
    // Helper function for computing the distance-to-preActionPose threshold,
    // given how far preActionPose is from actionObject
    Point2f ComputePreActionPoseDistThreshold(const Pose3d& preActionPose,
                                              const Pose3d& actionObjectPose,
                                              const Radians& preActionPoseAngleTolerance)
    {
      if(preActionPoseAngleTolerance > 0.f) {
        // Compute distance threshold for preaction pose based on distance to the
        // object: the further away, the more slop we're allowed.
        Pose3d objectWrtPreActionPose;
        if(false == actionObjectPose.GetWithRespectTo(preActionPose, objectWrtPreActionPose)) {
          PRINT_NAMED_WARNING("ComputePreActionPoseDistThreshold.ObjectPoseOriginProblem",
                              "Could not get object pose w.r.t. preActionPose.");
          return -1.f;
        }
        
        const f32 objectDistance = objectWrtPreActionPose.GetTranslation().Length();
        const f32 thresh = objectDistance * std::sin(preActionPoseAngleTolerance.ToFloat());
        
        // We don't care so much about the distance to the object (x threshold) so scale it
        const Point2f preActionPoseDistThresh(thresh * PREACTION_POSE_X_THRESHOLD_SCALAR, thresh);
        
        PRINT_CH_INFO("Actions",
                      "ComputePreActionPoseDistThreshold.DistThresh",
                      "At a distance of %.1fmm, will use pre-dock pose distance threshold of (%.1fmm, %.1fmm)",
                      objectDistance, preActionPoseDistThresh.x(), preActionPoseDistThresh.y());
        
        return preActionPoseDistThresh;
      } else {
        return -1.f;
      }
    }

    #pragma mark ---- IDockAction ----

    IDockAction::IDockAction(Robot& robot,
                             ObjectID objectID,
                             const std::string name,
                             const RobotActionType type,
                             const bool useManualSpeed)
    : IAction(robot,
              name,
              type,
              ((u8)AnimTrackFlag::HEAD_TRACK |
               (u8)AnimTrackFlag::LIFT_TRACK |
               (useManualSpeed ? 0 : (u8)AnimTrackFlag::BODY_TRACK)))
    , _dockObjectID(objectID)
    , _useManualSpeed(useManualSpeed)
    , _dockingMethod((DockingMethod)kDefaultDockingMethod)
    {
      
    }
    
    IDockAction::~IDockAction()
    {      
      // the action automatically selects the block, deselect now to remove Viz
      _robot.GetBlockWorld().DeselectCurrentObject();
    
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
      
      _robot.UnsetDockObjectID();
      
      if(_lightsSet)
      {
        PRINT_CH_INFO("Actions", "IDockAction.UnsetInteracting", "%s[%d] Unsetting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _dockObjectID.GetValue());
        _robot.GetLightsComponent().UnSetInteractionObject(_dockObjectID);
      }
      
      // Stop squinting
      _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_squintLayerTag, 250);
      
      if(_faceAndVerifyAction != nullptr)
      {
        _faceAndVerifyAction->PrepForCompletion();
      }

      if(GetState() != ActionResult::FAILURE_NOT_STARTED) {
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior("dockAction",
                                                                     BehaviorType::AcknowledgeObject,
                                                                     true);
      }

      
      Util::SafeDelete(_faceAndVerifyAction);
      
      for (auto& b : _behaviorsToSuppress) {
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), b, true);
      }
    }

    void IDockAction::SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2, f32 decel_mmps2)
    {
      _dockSpeed_mmps = speed_mmps;
      _dockAccel_mmps2 = accel_mmps2;
      _dockDecel_mmps2 = decel_mmps2;
    }

    void IDockAction::SetSpeed(f32 speed_mmps)
    {
      _dockSpeed_mmps = speed_mmps;
    }

    void IDockAction::SetAccel(f32 accel_mmps2, f32 decel_mmps2)
    {
      _dockAccel_mmps2 = accel_mmps2;
      _dockDecel_mmps2 = decel_mmps2;
    }

    void IDockAction::SetPlacementOffset(f32 offsetX_mm, f32 offsetY_mm, f32 offsetAngle_rad)
    {
      if(FLT_LT(offsetX_mm, -kMaxNegativeXPlacementOffset)) {
        ASSERT_NAMED_EVENT(false, "IDockAction.SetPlacementOffset.InvalidOffset", "x offset %f cannot be negative (through block)", offsetX_mm);
        // for release set offset to 0 so that Cozmo doesn't look stupid plowing through a block
        offsetX_mm = 0;
      }
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

    void IDockAction::SetPostDockLiftMovingAnimation(AnimationTrigger animTrigger)
    {
      _liftMovingAnimation = animTrigger;
    }
    
    Result IDockAction::ComputePlacementApproachAngle(const Robot& robot,
                                                      const Pose3d& placementPose,
                                                      f32& approachAngle_rad)
    {
      if (!robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "ComputePlacementApproachAngle.NoCarriedObject", "");
        return RESULT_FAIL;
      }
    
      // Get carried object
      const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(robot.GetCarryingObject());
      if(nullptr == object)
      {
        PRINT_NAMED_WARNING("DriveToActions.ComputePlacementApproachAngle.NullObject",
                            "ObjectID=%d", robot.GetCarryingObject().GetValue());
        return RESULT_FAIL;
      }
      
      // Check that up axis of carried object and the desired placementPose are the same.
      // Otherwise, it's impossible for the robot to place it there!
      const AxisName targetUpAxis = placementPose.GetRotationMatrix().GetRotatedParentAxis<'Z'>();
      const AxisName currentUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
      if (currentUpAxis != targetUpAxis) {
        PRINT_NAMED_WARNING("ComputePlacementApproachAngle.MismatchedUpAxes",
                            "Carried up axis: %d , target up axis: %d",
                            currentUpAxis, targetUpAxis);
        return RESULT_FAIL;
      }
      
      // Get pose of carried object wrt robot
      Pose3d poseObjectWrtRobot;
      if (!object->GetPose().GetWithRespectTo(robot.GetPose(), poseObjectWrtRobot)) {
        PRINT_NAMED_WARNING("ComputePlacementApproachAngle.FailedToComputeObjectWrtRobotPose", "");
        return RESULT_FAIL;
      }
      
      // Get pose of robot if the carried object were aligned with the placementPose and the robot was still carrying it
      Pose3d poseRobotIfPlacingObject = poseObjectWrtRobot.GetInverse();
      poseRobotIfPlacingObject.PreComposeWith(placementPose);
      
      approachAngle_rad = poseRobotIfPlacingObject.GetRotationMatrix().GetAngleAroundParentAxis<'Z'>().ToFloat();
      
      return RESULT_OK;
    }
    
    void IDockAction::GetPreActionPoses(Robot& robot,
                                        const PreActionPoseInput& input,
                                        PreActionPoseOutput& output)
    {
      ActionableObject* dockObject                = input.object;
      PreActionPose::ActionType preActionPoseType = input.preActionPoseType;
      bool doNearPredockPoseCheck                 = input.doNearPreDockPoseCheck;
      Radians preActionPoseAngleTolerance         = input.preActionPoseAngleTolerance;
      f32 preDockPoseDistOffsetX_mm               = input.preDockPoseDistOffsetX_mm;
      
      std::vector<PreActionPose>& preActionPoses  = output.preActionPoses;
      size_t& closestIndex                        = output.closestIndex;
      Point2f& closestPoint                       = output.closestPoint;
    
      // Make sure the object we were docking with is not null
      if(dockObject == nullptr) {
        PRINT_NAMED_WARNING("IsCloseEnoughToPreActionPose.NullObject", "");
        output.actionResult = ActionResult::FAILURE_ABORT;
        output.interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return;
      }
      
      if(dockObject->GetID() == robot.GetCarryingObject())
      {
        PRINT_NAMED_WARNING("IsCloseEnoughToPreActionPose.CarryingSelectedObject",
                            "Robot is currently carrying action object with ID=%d",
                            dockObject->GetID().GetValue());
        output.actionResult = ActionResult::FAILURE_ABORT;
        output.interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return;
      }
      
      // select the object so it shows up properly in viz
      robot.GetBlockWorld().SelectObject(dockObject->GetID());
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<std::pair<Quad2f, ObjectID> > obstacles;
      robot.GetBlockWorld().GetObstacles(obstacles);
      
      PRINT_CH_DEBUG("Actions", "IsCloseEnoughToPreActionPose.GetCurrentPreActionPoses",
                     "Using preDockPoseOffset_mm %f and %s",
                     preDockPoseDistOffsetX_mm,
                     (doNearPredockPoseCheck ? "checking if near pose" : "NOT checking if near pose"));
      dockObject->GetCurrentPreActionPoses(preActionPoses,
                                           robot.GetPose(),
                                           {preActionPoseType},
                                           std::set<Vision::Marker::Code>(),
                                           obstacles,
                                           nullptr,
                                           preDockPoseDistOffsetX_mm);
      
      // If using approach angle remove any preAction poses that aren't close to the desired approach angle
      if(input.useApproachAngle)
      {
        for(auto iter = preActionPoses.begin(); iter != preActionPoses.end();)
        {
          Pose3d preActionPose;
          if(iter->GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), preActionPose) == false)
          {
            PRINT_NAMED_WARNING("IsCloseEnoughToPreActionPose.PreActionPoseOriginProblem",
                                "Could not get pre-action pose w.r.t. world origin.");
            continue;
          }
          
          Radians headingDiff = preActionPose.GetRotationAngle<'Z'>() - input.approachAngle_rad;
          // If the heading difference between our desired approach angle and the preAction pose's heading is
          // greater than 45 degrees this preAction pose will not be the one of the poses closest to approach angle
          if(FLT_GE(std::abs(headingDiff.ToFloat()), DEG_TO_RAD_F32(45)))
          {
            iter = preActionPoses.erase(iter);
          }
          else
          {
            ++iter;
          }
        }
      }
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_WARNING("IsCloseEnoughToPreActionPose.NoPreActionPoses",
                            "Action object with ID=%d returned no pre-action poses of the given type.",
                            dockObject->GetID().GetValue());
        output.actionResult = ActionResult::FAILURE_ABORT;
        output.interactionResult = ObjectInteractionResult::NO_PREACTION_POSES;
        return;
      }
      
      const Point2f currentXY(robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y());
      
      closestIndex = preActionPoses.size();
      float closestDistSq = std::numeric_limits<float>::max();
      
      for(size_t index = 0; index < preActionPoses.size(); ++index)
      {
        Pose3d preActionPose;
        if(preActionPoses[index].GetPose().GetWithRespectTo(*robot.GetPose().GetParent(), preActionPose) == false)
        {
          PRINT_NAMED_WARNING("IsCloseEnoughToPreActionPose.PreActionPoseOriginProblem",
                              "Could not get pre-action pose w.r.t. world origin.");
          continue;
        }
        
        const Point2f preActionXY(preActionPose.GetTranslation().x(),
                                  preActionPose.GetTranslation().y());
        const Point2f dist = (currentXY - preActionXY);
        const float distSq = dist.LengthSq();
        
        PRINT_CH_DEBUG("Actions", "IsCloseEnoughToPreActionPose.CheckPoint",
                       "considering point (%f, %f) dist = %f",
                       dist.x(), dist.y(),
                       dist.Length());
        
        if(distSq < closestDistSq)
        {
          closestPoint = dist.GetAbs();
          closestIndex  = index;
          closestDistSq = distSq;
        }
      }
      
      PRINT_CH_INFO("Actions", "IsCloseEnoughToPreActionPose.ClosestPoint",
                    "Closest point (%f, %f) robot (%f, %f) dist = %f",
                    preActionPoses[closestIndex].GetPose().GetTranslation().x(),
                    preActionPoses[closestIndex].GetPose().GetTranslation().y(),
                    currentXY.x(), currentXY.y(),
                    closestPoint.Length());
      
      output.distThresholdUsed = ComputePreActionPoseDistThreshold(preActionPoses[closestIndex].GetPose(),
                                                                   dockObject->GetPose(),
                                                                   preActionPoseAngleTolerance);
      
      output.robotAtClosestPreActionPose = false;
      
      if(output.distThresholdUsed > 0)
      {
        if(closestPoint.AnyGT(output.distThresholdUsed))
        {
          // If we are checking that we are close enough to the preDock pose and our closestPoint is
          // outside the distThreshold then fail saying we are too far away
          // Otherwise we will succeed but robotAtClosestPreActionPose will stay false
          if(doNearPredockPoseCheck)
          {
            PRINT_CH_INFO("Actions", "IsCloseEnoughToPreActionPose.TooFarFromGoal",
                          "Robot is too far from pre-action pose (%.1fmm, %.1fmm).",
                          closestPoint.x(), closestPoint.y());
            output.actionResult = ActionResult::FAILURE_RETRY;
            output.interactionResult = ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE;
            return;
          }
        }
        // Else closestPoint is within the distThreshold and if the angle of the closest preAction pose is within
        // preActionPoseAngleTolerance to the current angle of the robot then set robotAtClosestPreActionPose to true
        else
        {
          Pose3d p;
          preActionPoses[closestIndex].GetPose().GetWithRespectTo(robot.GetPose(), p);

          if(FLT_LT(std::abs(p.GetRotation().GetAngleAroundZaxis().ToFloat()), preActionPoseAngleTolerance.ToFloat()))
          {
            PRINT_CH_INFO("Actions",
                          "IsCloseEnoughToPreActionPose.AtClosestPreActionPose",
                          "Robot is close enough to closest preAction pose (%.1fmm, %.1fmm) with threshold (%.1fmm, %.1fmm)",
                          closestPoint.x(),
                          closestPoint.y(),
                          output.distThresholdUsed.x(),
                          output.distThresholdUsed.y());
            output.robotAtClosestPreActionPose = true;
          }
        }
      }
      
      output.actionResult = ActionResult::SUCCESS;
      output.interactionResult = ObjectInteractionResult::SUCCESS;
    }

    ActionResult IDockAction::Init()
    {
      _waitToVerifyTime = -1.f;

      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      
      if(dockObject == nullptr)
      {
        PRINT_NAMED_WARNING("IDockAction.NullDockObject", "Dock object is null returning failure");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return ActionResult::FAILURE_ABORT;
      }
      
      PreActionPoseOutput preActionPoseOutput;
      
      if(_doNearPredockPoseCheck)
      {
        PreActionPoseInput preActionPoseInput(dockObject,
                                              GetPreActionType(),
                                              _doNearPredockPoseCheck,
                                              _preDockPoseDistOffsetX_mm,
                                              _preActionPoseAngleTolerance.ToFloat(),
                                              false, 0);
        
        GetPreActionPoses(_robot, preActionPoseInput, preActionPoseOutput);
      
        _interactionResult = preActionPoseOutput.interactionResult;
        
        if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
        {
          return preActionPoseOutput.actionResult;
        }
      }
      
      if(SelectDockAction(dockObject) != RESULT_OK) {
        PRINT_NAMED_WARNING("IDockAction.Init.DockActionSelectionFailure",
                            "");
        // NOTE: SelectDockAction should set _interactionResult on failure
        return ActionResult::FAILURE_ABORT;
      }
      
      // Specify post-dock lift motion callback to play sound
      using namespace RobotInterface;
      auto liftSoundLambda = [this](const AnkiEvent<RobotToEngine>& event)
      {
        if (_liftMovingAnimation != AnimationTrigger::Count) {
          // Check that the animation only has sound keyframes
          bool has_response = _robot.GetContext()->GetDataLoader()->GetAnimationTriggerResponses()->HasResponse(_liftMovingAnimation);
          if (has_response) {
            
            // Check that the action matches the current action
            DockAction recvdAction = event.GetData().Get_movingLiftPostDock().action;
            if (_dockAction != recvdAction) {
              PRINT_NAMED_WARNING("IDockAction.MovingLiftPostDockHandler.ActionMismatch",
                                  "Expected %u, got %u. Ignoring.",
                                  (u32)_dockAction, (u32)recvdAction);
              return;
            }
            
            // Play the animation
            PRINT_CH_INFO("Actions", "IDockAction.MovingLiftPostDockHandler",
                          "Playing animation %s ",
                          EnumToString(_liftMovingAnimation));
            IActionRunner* animAction = new TriggerAnimationAction(_robot, _liftMovingAnimation, 1, false);
            animAction->ShouldEmitCompletionSignal(false);
            _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
          } else {
            PRINT_NAMED_WARNING("IDockAction.MovingLiftPostDockHandler.InvalidAnimation",
                                "Could not find animation %s",
                                EnumToString(_liftMovingAnimation));
          }
        }
      };
      _liftMovingSignalHandle = _robot.GetRobotMessageHandler()->Subscribe(_robot.GetID(), RobotToEngineTag::movingLiftPostDock, liftSoundLambda);
      
      
      if (_doNearPredockPoseCheck) {
        PRINT_CH_INFO("Actions", "IDockAction.Init.BeginDockingFromPreActionPose",
                      "Robot is within (%.1fmm,%.1fmm) of the nearest pre-action pose, "
                      "proceeding with docking.", preActionPoseOutput.closestPoint.x(), preActionPoseOutput.closestPoint.y());
        
        // Set dock markers
        _dockMarker = preActionPoseOutput.preActionPoses[preActionPoseOutput.closestIndex].GetMarker();
        _dockMarker2 = GetDockMarker2(preActionPoseOutput.preActionPoses, preActionPoseOutput.closestIndex);
        
      } else {
        std::vector<const Vision::KnownMarker*> markers;
        dockObject->GetObservedMarkers(markers);
        
        if(markers.empty())
        {
          PRINT_NAMED_ERROR("IDockAction.Init.NoMarkers",
                            "Using currently observed markers instead of preDock pose but no currently visible marker");
          _interactionResult = ObjectInteractionResult::VISUAL_VERIFICATION_FAILED;
          return ActionResult::FAILURE_ABORT;
        }
        else if(markers.size() == 1)
        {
          _dockMarker = markers.front();
        }
        else
        {
          f32 distToClosestMarker = std::numeric_limits<f32>::max();
          for(const Vision::KnownMarker* marker : markers)
          {
            Pose3d p;
            if(!marker->GetPose().GetWithRespectTo(_robot.GetPose(), p))
            {
              PRINT_CH_INFO("Actions", "IDockAction.Init.GetMarkerWRTRobot",
                            "Failed to get marker %s's pose wrt to robot",
                            marker->GetCodeName());
              continue;
            }
            
            if(p.GetTranslation().LengthSq() < distToClosestMarker*distToClosestMarker)
            {
              distToClosestMarker = p.GetTranslation().Length();
              _dockMarker = marker;
            }
          }
        }
        PRINT_CH_INFO("Actions", "IDockAction.Init.BeginDockingToMarker",
                      "Proceeding with docking to marker %s", _dockMarker->GetCodeName());
      }
      
      if(_dockMarker == nullptr)
      {
        PRINT_NAMED_WARNING("IDockAction.Init.NullDockMarker", "Dock marker is null returning failure");
        return ActionResult::FAILURE_ABORT;
      }

      SetupTurnAndVerifyAction(dockObject);
      
      for (auto& b : _behaviorsToSuppress) {
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), b, false);
      }
      
      if(!_lightsSet)
      {
        PRINT_CH_INFO("Actions", "IDockAction.SetInteracting", "%s[%d] Setting interacting object to %d",
                      GetName().c_str(), GetTag(),
                      _dockObjectID.GetValue());
        _robot.GetLightsComponent().SetInteractionObject(_dockObjectID);
        _lightsSet = true;
      }
      
      // If this is a reset clear the _squintLayerTag
      if(_squintLayerTag != AnimationStreamer::NotAnimatingTag){
        _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_squintLayerTag, 250);
        _squintLayerTag = AnimationStreamer::NotAnimatingTag;
      }
      
      // Allow actions the opportunity to check or set any properties they need to
      // this allows actions that are part of driveTo or wrappers a chance to check data
      // when they know they're at the pre-dock pose
      ActionResult internalActionResult = InitInternal();
      if(internalActionResult != ActionResult::SUCCESS){
        return internalActionResult;
      }
      
      // Go ahead and Update the FaceObjectAction once now, so we don't
      // waste a tick doing so in CheckIfDone (since this is the first thing
      // that will be done in CheckIfDone anyway)
      ActionResult faceObjectResult = _faceAndVerifyAction->Update();

      if(ActionResult::SUCCESS == faceObjectResult ||
         ActionResult::RUNNING == faceObjectResult)
      {

        // disable the reactionary behavior for objects, since we are about to interact with one
        // TODO:(bn) should some dock actions not do this? E.g. align with offset that doesn't touch the cube?
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior("dockAction",
                                                                     BehaviorType::AcknowledgeObject,
                                                                     false);
        
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
            
            PRINT_CH_INFO("Actions", "IDockAction.DockWithObjectHelper.BeginDocking",
                          "Docking with marker %d (%s) using action %s.",
                          _dockMarker->GetCode(), Vision::MarkerTypeStrings[_dockMarker->GetCode()], DockActionToString(_dockAction));
            if(_robot.DockWithObject(_dockObjectID,
                                     _dockSpeed_mmps,
                                     _dockAccel_mmps2,
                                     _dockDecel_mmps2,
                                     _dockMarker, _dockMarker2,
                                     _dockAction,
                                     _placementOffsetX_mm,
                                     _placementOffsetY_mm,
                                     _placementOffsetAngle_rad,
                                     _useManualSpeed,
                                     _numDockingRetries,
                                     _dockingMethod) == RESULT_OK)
            {
              //NOTE: Any completion (success or failure) after this point should tell
              // the robot to stop tracking and go back to looking for markers!
              _wasPickingOrPlacing = false;
            } else {
              return ActionResult::FAILURE_ABORT;
            }
            
          } else {
            PRINT_NAMED_WARNING("IDockAction.CheckIfDone.VisualVerifyFailed",
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
          //PRINT_CH_INFO("Actions", "IDockAction.CheckIfDone",
          //              "Robot has stopped moving and picking/placing. Will attempt to verify success.");
          
          actionResult = Verify();
        }
      }
      
      if(ActionResult::SUCCESS == actionResult) {
        _interactionResult = ObjectInteractionResult::SUCCESS;
      }
      
      return actionResult;
    } // CheckIfDone()
    
    
    void IDockAction::SetupTurnAndVerifyAction(const ObservableObject* dockObject)
    {
      _faceAndVerifyAction = new CompoundActionSequential(_robot,{});
      
      _faceAndVerifyAction->ShouldEmitCompletionSignal(false);
      _faceAndVerifyAction->ShouldSuppressTrackLocking(true);
    
      // If we are checking to see if there is an object on top of our dockObject then
      // add a VisuallyVerifyNoObjectAtPoseAction to the _faceAndVerifyAction
      if(_checkForObjectOnTopOf)
      {
        Pose3d pose = dockObject->GetPose().GetWithRespectToOrigin();
        const Point3f rotatedSize = dockObject->GetPose().GetRotation() * dockObject->GetSize();
        pose.SetTranslation({pose.GetTranslation().x(),
                             pose.GetTranslation().y(),
                             pose.GetTranslation().z() + rotatedSize.z()});
        
        VisuallyVerifyNoObjectAtPoseAction* verifyNoObjectOnTopOfAction = new VisuallyVerifyNoObjectAtPoseAction(_robot,
                                                                                                                 pose,
                                                                                                                 rotatedSize * 0.5f);
        verifyNoObjectOnTopOfAction->AddIgnoreID(dockObject->GetID());
        
        // Disable the visual verification from issuing a completion signal
        verifyNoObjectOnTopOfAction->ShouldEmitCompletionSignal(false);
        verifyNoObjectOnTopOfAction->ShouldSuppressTrackLocking(true);
        
        _faceAndVerifyAction->AddAction(verifyNoObjectOnTopOfAction);
      }
      
      // Set up a visual verification action to make sure we can still see the correct
      // marker of the selected object before proceeding
      // NOTE: This also disables tracking head to object if there was any
      IAction* turnTowardsDockObjectAction = new TurnTowardsObjectAction(_robot,
                                                                         _dockObjectID,
                                                                         (_visuallyVerifyObjectOnly ? Vision::Marker::ANY_CODE : _dockMarker->GetCode()),
                                                                         0, true, false);
      
      // Disable the turn towards action from issuing a completion signal
      turnTowardsDockObjectAction->ShouldEmitCompletionSignal(false);
      turnTowardsDockObjectAction->ShouldSuppressTrackLocking(true);
      
      _faceAndVerifyAction->AddAction(turnTowardsDockObjectAction);
    }
    
#pragma mark ---- PopAWheelieAction ----
    
    PopAWheelieAction::PopAWheelieAction(Robot& robot,
                                         ObjectID objectID,
                                         const bool useManualSpeed)
    : IDockAction(robot,
                  objectID,
                  "PopAWheelie",
                  RobotActionType::POP_A_WHEELIE,
                  useManualSpeed)
    {
      
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
        {
          if(GetState() != ActionResult::FAILURE_NOT_STARTED)
          {
            PRINT_NAMED_WARNING("PopAWheelieAction.EmitCompletionSignal",
                                "Dock action not set before filling completion signal.");
          }
        }
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
        PRINT_CH_INFO("Actions", "PopAWheelieAction.SelectDockAction", "Object is too high to pop-a-wheelie. Aborting.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      } else if (_robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "PopAWheelieAction.SelectDockAction", "Can't pop-a-wheelie while carrying an object.");
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
              PRINT_CH_INFO("Actions", "PopAWheelieAction.Verify.PitchAngleTooSmall",
                            "Robot pitch angle expected to be higher (measured %f rad)",
                            _robot.GetPitchAngle().ToDouble());
              result = ActionResult::FAILURE_RETRY;
            } else {
              result = ActionResult::SUCCESS;
            }
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track.
            PRINT_CH_INFO("Actions", "PopAWheelieAction.Verify.DockingFailed",
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
    
    
#pragma mark ---- FacePlantAction ----
    
    FacePlantAction::FacePlantAction(Robot& robot,
                                     ObjectID objectID,
                                     const bool useManualSpeed)
    : IDockAction(robot, objectID, "FacePlant", RobotActionType::FACE_PLANT, useManualSpeed)
    {
      SetShouldCheckForObjectOnTopOf(false);
      SetShouldSuppressReactionaryBehavior(BehaviorType::ReactToCubeMoved);
      SetShouldSuppressReactionaryBehavior(BehaviorType::ReactToUnexpectedMovement);
      SetShouldSuppressReactionaryBehavior(BehaviorType::ReactToCliff);
    }
    
    void FacePlantAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      switch(_dockAction)
      {
        case DockAction::DA_FACE_PLANT:
        {
          if(_robot.IsCarryingObject()) {
            PRINT_NAMED_WARNING("FacePlantAction.EmitCompletionSignal",
                                "Expecting robot to think it's not carrying object for FacePlant action.");
          } else {
            info.numObjects = 1;
            info.objectIDs.fill(-1);
            info.objectIDs[0] = _dockObjectID;
          }
          break;
        }
        default:
          PRINT_NAMED_WARNING("FacePlantAction.EmitCompletionSignal",
                              "Dock action not set before filling completion signal.");
      }
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result FacePlantAction::SelectDockAction(ActionableObject* object)
    {
      Pose3d objectPose;
      if(object->GetPose().GetWithRespectTo(*_robot.GetPose().GetParent(), objectPose) == false) {
        PRINT_NAMED_WARNING("FacePlantAction.SelectDockAction.PoseWrtFailed",
                            "Could not get pose of dock object w.r.t. robot's parent.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      const f32 dockObjectHeightWrtRobot = objectPose.GetTranslation().z() - _robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_FACE_PLANT;
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        PRINT_CH_INFO("Actions", "FacePlantAction.SelectDockAction.ObjectTooHigh", "");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      // Check that this block is at the bottom of a 3-block stack
      // TODO: This logic only works because there can only ever be one stack with three blocks,
      //       but it should be made more generic.
      auto blockStackPtr = _robot.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetTallestStack();
      if (auto blockStack = blockStackPtr.lock()) {
        if (blockStack->GetStackHeight() < 3) {
          PRINT_CH_INFO("Actions", "FacePlantAction.SelectDockAction.ObjectStackNotBigEnough", "");
          _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
          return RESULT_FAIL;
        }
        
        if (blockStack->GetBottomBlockID() != object->GetID() ) {
          PRINT_CH_INFO("Actions", "FacePlantAction.SelectDockAction.ObjectNotAtBottomOfStack", "");
          _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
          return RESULT_FAIL;
        }
      } else {
        PRINT_CH_INFO("Actions", "FacePlantAction.SelectDockAction.NoStackFound", "");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
    
      if (_robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "FacePlantAction.SelectDockAction", "Can't face plant while carrying an object.");
        _interactionResult = ObjectInteractionResult::STILL_CARRYING;
        return RESULT_FAIL;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult FacePlantAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      switch(_dockAction)
      {
        case DockAction::DA_FACE_PLANT:
        {
          if(_robot.GetLastPickOrPlaceSucceeded()) {
            // Check that the robot is sufficiently pitched down
            if (_robot.GetPitchAngle() > kMaxSuccessfulPitchAngle_rad) {
              PRINT_CH_INFO("Actions", "FacePlantAction.Verify.PitchAngleTooSmall",
                            "Robot pitch angle expected to be lower (measured %f deg)",
                            _robot.GetPitchAngle().getDegrees() );
              result = ActionResult::FAILURE_RETRY;
            } else {
              result = ActionResult::SUCCESS;
            }
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track.
            PRINT_CH_INFO("Actions", "FacePlantAction.Verify.DockingFailed",
                          "Robot reported face plant failure. Assuming docking failed");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // DA_FACE_PLANT
          
          
        default:
          PRINT_NAMED_WARNING("FacePlantAction.Verify.ReachedDefaultCase",
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
                                                 const AlignmentType alignmentType,
                                                 const bool useManualSpeed)
    : IDockAction(robot,
                  objectID,
                  "AlignWithObject",
                  RobotActionType::ALIGN_WITH_OBJECT,
                  useManualSpeed)
    {
      SetShouldCheckForObjectOnTopOf(false);
      
      f32 distance = 0;
      switch(alignmentType)
      {
        case(AlignmentType::LIFT_FINGER):
        {
          distance = ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM;
          break;
        }
        case(AlignmentType::LIFT_PLATE):
        {
          distance = ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM - LIFT_FRONT_WRT_WRIST_JOINT;
          break;
        }
        case(AlignmentType::BODY):
        {
          distance = WHEEL_RAD_TO_MM;
          break;
        }
        case(AlignmentType::CUSTOM):
        {
          distance = distanceFromMarker_mm;
          break;
        }
      }
      SetPlacementOffset(distance, 0, 0);
    }
    
    AlignWithObjectAction::~AlignWithObjectAction()
    {
      
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
          if (!_robot.IsPickingOrPlacing() && !_robot.IsTraversingPath() && _robot.GetLastPickOrPlaceSucceeded()) {
            PRINT_CH_INFO("Actions", "AlignWithObjectAction.Verify", "Align with object SUCCEEDED!");
            result = ActionResult::SUCCESS;
          }
          break;
        } // ALIGN
          
        default:
          PRINT_NAMED_WARNING("AlignWithObjectAction.Verify.ReachedDefaultCase",
                              "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
#pragma mark ---- PickupObjectAction ----
    
    PickupObjectAction::PickupObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IDockAction(robot,
                  objectID,
                  "PickupObject",
                  RobotActionType::PICK_AND_PLACE_INCOMPLETE,
                  useManualSpeed)
    {
      _dockingMethod = (DockingMethod)kPickupDockingMethod;
      SetPostDockLiftMovingAnimation(AnimationTrigger::SoundOnlyLiftEffortPickup);
    }
    
    PickupObjectAction::~PickupObjectAction()
    {
      if(_verifyAction != nullptr)
      {
        _verifyAction->PrepForCompletion();
        Util::SafeDelete(_verifyAction);
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
            PRINT_CH_INFO("Actions", "PickupObjectAction.GetCompletionUnion.NotCarrying", "");
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
        {
          // Not setting dock action is only an issue if the action has started
          if(GetState() != ActionResult::FAILURE_NOT_STARTED)
          {
            PRINT_NAMED_WARNING("PickupObjectAction.EmitCompletionSignal",
                                "Dock action not set before filling completion signal");
          }
        }
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
        PRINT_NAMED_WARNING("PickupObjectAction.SelectDockAction.PoseWrtFailed",
                            "Could not get pose of dock object w.r.t. robot parent.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeightWrtRobot = _dockObjectOrigPose.GetTranslation().z() - _robot.GetPose().GetTranslation().z();
      _dockAction = DockAction::DA_PICKUP_LOW;
      SetType(RobotActionType::PICKUP_OBJECT_LOW);
      
      if (_robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "PickupObjectAction.SelectDockAction", "Already carrying object. Can't pickup object. Aborting.");
        _interactionResult = ObjectInteractionResult::STILL_CARRYING;
        return RESULT_FAIL;
      } else if (dockObjectHeightWrtRobot > 0.5f*ROBOT_BOUNDING_Z) { // TODO: Stop using constant ROBOT_BOUNDING_Z for this
        _dockAction = DockAction::DA_PICKUP_HIGH;
        SetType(RobotActionType::PICKUP_OBJECT_HIGH);
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    ActionResult PickupObjectAction::Verify()
    {
      ActionResult result = ActionResult::FAILURE_ABORT;
      
      if(_verifyAction == nullptr)
      {
        _verifyAction = new TurnTowardsPoseAction(_robot, _dockObjectOrigPose, 0);
        _verifyAction->ShouldEmitCompletionSignal(false);
        _verifyAction->ShouldSuppressTrackLocking(true);
        _verifyActionDone = false;
      }
      
      if(!_verifyActionDone)
      {
        ActionResult res = _verifyAction->Update();
        if(res != ActionResult::RUNNING)
        {
          _verifyActionDone = true;
        }
        else
        {
          return ActionResult::RUNNING;
        }
      }
      
      switch(_dockAction)
      {
        case DockAction::DA_PICKUP_LOW:
        case DockAction::DA_PICKUP_HIGH:
        {
          if(_robot.IsCarryingObject() == false) {
            PRINT_NAMED_WARNING("PickupObjectAction.Verify.RobotNotCarryingObject",
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
            PRINT_NAMED_WARNING("PickupObjectAction.Verify.CarryObjectNoLongerExists",
                                "Object %d we were carrying no longer exists in the world.",
                                _robot.GetCarryingObject().GetValue());
            _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
            result = ActionResult::FAILURE_ABORT;
            break;
          }
          
          BlockWorldFilter filter;
          filter.SetAllowedTypes({carryObject->GetType()});
          std::vector<ObservableObject*> objectsWithType;
          blockWorld.FindMatchingObjects(filter, objectsWithType);
          
          // Robot's pose parent could have changed due to delocalization.
          // Assume it's actual pose is relatively accurate w.r.t. that original
          // pose (when dockObjectOrigPose was stored) and update the parent so
          // that we can do IsSameAs checks below.
          _dockObjectOrigPose.SetParent(_robot.GetPose().GetParent());
          
          Vec3f Tdiff;
          Radians angleDiff;
          ObservableObject* objectInOriginalPose = nullptr;
          for(const auto& object : objectsWithType)
          {
            // TODO: is it safe to always have useAbsRotation=true here?
            Vec3f Tdiff;
            Radians angleDiff;
            if(object->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose, // dock obj orig pose is w.r.t. robot
                                                        carryObject->GetRotationAmbiguities(),
                                                        carryObject->GetSameDistanceTolerance()*0.5f,
                                                        carryObject->GetSameAngleTolerance(), true,
                                                        Tdiff, angleDiff))
            {
              PRINT_CH_INFO("Actions", "PickupObjectAction.Verify.ObjectInOrigPose",
                            "Seeing object %d in original pose. (Tdiff = (%.1f,%.1f,%.1f), "
                            "AngleDiff=%.1fdeg), carrying object %d",
                            object->GetID().GetValue(),
                            Tdiff.x(), Tdiff.y(), Tdiff.z(), angleDiff.getDegrees(),
                            carryObject->GetID().GetValue());
              
              objectInOriginalPose = object;
              break;
            }
          }
          
          if(objectInOriginalPose != nullptr)
          {
            // Must not actually be carrying the object I thought I was!
            // Put the object I thought I was carrying in the position of the
            // object I matched to it above, and then delete that object.
            // (This prevents a new object with different ID being created.)
            if(carryObject->GetID() != objectInOriginalPose->GetID())
            {
              PRINT_CH_INFO("Actions", "PickupObjectAction.Verify.SeeingDifferentObjectInOrigPose",
                            "Moving carried object (%s ID=%d) to object seen in original pose "
                            "and deleting that object (%s ID=%d).",
                            EnumToString(carryObject->GetType()),
                            carryObject->GetID().GetValue(),
                            EnumToString(objectInOriginalPose->GetType()),
                            objectInOriginalPose->GetID().GetValue());
              
              _robot.GetObjectPoseConfirmer().CopyWithNewPose(carryObject, objectInOriginalPose->GetPose(), objectInOriginalPose);
              
              blockWorld.DeleteObject(objectInOriginalPose->GetID());
            }
            _robot.UnSetCarryingObjects();
            
            PRINT_CH_INFO("Actions", "PickupObjectAction.Verify.SeeingCarriedObjectInOrigPose",
                          "Object pick-up FAILED! (Still seeing object in same place.)");
            _interactionResult = ObjectInteractionResult::NOT_CARRYING;
            result = ActionResult::FAILURE_RETRY;
          } else {
            PRINT_CH_INFO("Actions", "PickupObjectAction.Verify.Success", "Object pick-up SUCCEEDED!");
            _interactionResult = ObjectInteractionResult::SUCCESS;
            result = ActionResult::SUCCESS;
          }
          break;
        } // PICKUP
          
        default:
          PRINT_NAMED_WARNING("PickupObjectAction.Verify.ReachedDefaultCase",
                              "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)

      return result;
      
    } // Verify()
    
#pragma mark ---- PlaceObjectOnGroundAction ----
    
    PlaceObjectOnGroundAction::PlaceObjectOnGroundAction(Robot& robot)
    : IAction(robot,
              "PlaceObjectOnGround",
              RobotActionType::PLACE_OBJECT_LOW,
              ((u8)AnimTrackFlag::LIFT_TRACK |
               (u8)AnimTrackFlag::BODY_TRACK |
               (u8)AnimTrackFlag::HEAD_TRACK))
    {
      
    }
    
    PlaceObjectOnGroundAction::~PlaceObjectOnGroundAction()
    {
      // COZMO-3434 manually request to enable AckObject
      if(GetState() != ActionResult::FAILURE_NOT_STARTED) {
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior("PlaceObjectOnGroundAction",
                                                                     BehaviorType::AcknowledgeObject,
                                                                     true);
      }
    
      if(_faceAndVerifyAction != nullptr)
      {
        _faceAndVerifyAction->PrepForCompletion();
      }
      Util::SafeDelete(_faceAndVerifyAction);
    }
    
    ActionResult PlaceObjectOnGroundAction::Init()
    {
      ActionResult result = ActionResult::RUNNING;
      
      _startedPlacing = false;
      
      // Robot must be carrying something to put something down!
      if(_robot.IsCarryingObject() == false) {
        PRINT_NAMED_WARNING("PlaceObjectOnGroundAction.CheckPreconditions.NotCarryingObject",
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
          PRINT_NAMED_WARNING("PlaceObjectOnGroundAction.CheckPreconditions.SendPlaceObjectOnGroundFailed",
                              "Robot's SendPlaceObjectOnGround method reported failure.");
          _interactionResult = ObjectInteractionResult::UNKNOWN_PROBLEM;
          result = ActionResult::FAILURE_ABORT;
        }
        
        _faceAndVerifyAction = new TurnTowardsObjectAction(_robot,
                                                           _carryingObjectID,
                                                           _carryObjectMarker->GetCode(),
                                                           0, true, false);
        
        _faceAndVerifyAction->ShouldEmitCompletionSignal(false);
        _faceAndVerifyAction->ShouldSuppressTrackLocking(true);
        
      } // if/else IsCarryingObject()
      
      // If we were moving, stop moving.
      _robot.GetMoveComponent().StopAllMotors();
      
      _startedPlacing = false;
      
      // COZMO-3434 manually request to disable AckObject
      if(ActionResult::SUCCESS == result ||
         ActionResult::RUNNING == result)
      {
        // disable the reactionary behavior for objects, since we are placing one
        _robot.GetBehaviorManager().RequestEnableReactionaryBehavior("PlaceObjectOnGroundAction",
                                                                     BehaviorType::AcknowledgeObject,
                                                                     false);
      }
      
      return result;
      
    } // CheckPreconditions()
    
    ActionResult PlaceObjectOnGroundAction::CheckIfDone()
    {
      ActionResult actionResult = ActionResult::RUNNING;
      
      // Wait for robot to report it is done picking/placing and that it's not
      // moving
      
      if(_robot.IsPickingOrPlacing())
      {
        _startedPlacing = true;
      }
      
      if (_startedPlacing && !_robot.IsPickingOrPlacing() && !_robot.GetMoveComponent().IsMoving())
      {
        // Stopped executing docking path, and should have placed carried block
        // and backed out by now, and have head pointed at an angle to see
        // where we just placed or picked up from.
        // So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        
        actionResult = _faceAndVerifyAction->Update();
        
        if(actionResult != ActionResult::RUNNING && actionResult != ActionResult::SUCCESS) {
          PRINT_NAMED_WARNING("PlaceObjectOnGroundAction.CheckIfDone",
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
                                                                     const bool useExactRotation,
                                                                     const bool useManualSpeed,
                                                                     const bool checkFreeDestination,
                                                                     const float destinationObjectPadding_mm)
    : CompoundActionSequential(robot)
    {
      _driveAction = new DriveToPlaceCarriedObjectAction(robot,
                                                         placementPose,
                                                         true,
                                                         useExactRotation,
                                                         useManualSpeed,
                                                         checkFreeDestination,
                                                         destinationObjectPadding_mm);
      AddAction(_driveAction);
      
      PlaceObjectOnGroundAction* action = new PlaceObjectOnGroundAction(robot);
      AddAction(action);
      SetProxyTag(action->GetTag());
    }
    
    void PlaceObjectOnGroundAtPoseAction::SetMotionProfile(const PathMotionProfile& motionProfile)
    {
      if(nullptr != _driveAction) {
        _driveAction->SetMotionProfile(motionProfile);
      } else {
        PRINT_NAMED_WARNING("PlaceObjectOnGroundAtPoseAction.SetMotionProfile.NullDriveAction", "");
      }
    }
    
#pragma mark ---- PlaceRelObjectAction ----
    
    PlaceRelObjectAction::PlaceRelObjectAction(Robot& robot,
                                               ObjectID objectID,
                                               const bool placeOnGround,
                                               const f32 placementOffsetX_mm,
                                               const f32 placementOffsetY_mm,
                                               const bool useManualSpeed,
                                               const bool relativeCurrentMarker)
    : IDockAction(robot,
                  objectID,
                  "PlaceRelObject",
                  RobotActionType::PICK_AND_PLACE_INCOMPLETE,
                  useManualSpeed)
    , _relOffsetX_mm(placementOffsetX_mm)
    , _relOffsetY_mm(placementOffsetY_mm)
    , _relativeCurrentMarker(relativeCurrentMarker)
    {
      SetPlaceOnGround(placeOnGround);
      SetPostDockLiftMovingAnimation(placeOnGround ?
                                     AnimationTrigger::SoundOnlyLiftEffortPlaceLow :
                                     AnimationTrigger::SoundOnlyLiftEffortPlaceHigh);
      
      // Cozmo is carrying an object and wont be able to see on top of the object
      SetShouldCheckForObjectOnTopOf(false);
      
      // SetPlacementOffset set in InitInternal
    }

    ActionResult  PlaceRelObjectAction::InitInternal()
    {
      ActionResult result = ActionResult::SUCCESS;
      
      if(!_relativeCurrentMarker){
        result = TransformPlacementOffsetsRelativeObject();
      }
      
      // If attempting to place the block off to the side of the target, do it even blinder
      // so that Cozmo doesn't fail when he inevitably looses sight of the tracker
      if(!NEAR_ZERO(_relOffsetY_mm)){
        SetDockingMethod(DockingMethod::EVEN_BLINDER_DOCKING);
      }
      
      SetPlacementOffset(_relOffsetX_mm, _relOffsetY_mm, _placementOffsetAngle_rad);

      return result;
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
            PRINT_NAMED_WARNING("PlaceRelObjectAction.EmitCompletionSignal.NullObject",
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
        {
          // Not setting dock action is only an issue if the action has started
          if(GetState() != ActionResult::FAILURE_NOT_STARTED)
          {
            PRINT_NAMED_WARNING("PlaceRelObjectAction.EmitCompletionSignal",
                                "Dock action not set before filling completion signal.");
          }
        }
      }
      
      completionUnion.Set_objectInteractionCompleted(std::move( info ));
      IDockAction::GetCompletionUnion(completionUnion);
    }
    
    Result PlaceRelObjectAction::SelectDockAction(ActionableObject* object)
    {
      if (!_robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "PlaceRelObjectAction.SelectDockAction", "Can't place if not carrying an object. Aborting.");
        _interactionResult = ObjectInteractionResult::NOT_CARRYING;
        return RESULT_FAIL;
      }
      
      if(!_placeObjectOnGroundIfCarrying && !_robot.CanStackOnTopOfObject(*object))
      {
        PRINT_NAMED_WARNING("PlaceRelObjectAction.SelectDockAction", "Can't stack on object");
        return RESULT_FAIL;
      }
      
      _dockAction = _placeObjectOnGroundIfCarrying ? DockAction::DA_PLACE_LOW : DockAction::DA_PLACE_HIGH;
      
      if(_dockAction == DockAction::DA_PLACE_HIGH) {
        SetType(RobotActionType::PLACE_OBJECT_HIGH);
        _dockingMethod = (DockingMethod)kStackDockingMethod;
      }
      else
      {
        SetType(RobotActionType::PLACE_OBJECT_LOW);
      }
      
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
              PRINT_NAMED_WARNING("PlaceRelObjectAction.Verify",
                                  "Expecting robot to think it's NOT carrying an object at this point.");
              _interactionResult = ObjectInteractionResult::STILL_CARRYING;
              return ActionResult::FAILURE_ABORT;
            }
            
            // If the physical robot thinks it succeeded, move the lift out of the
            // way, and attempt to visually verify
            if(_placementVerifyAction == nullptr) {
              _placementVerifyAction = new TurnTowardsObjectAction(_robot, _carryObjectID, Radians(0), true, false);
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
              
              if(result != ActionResult::SUCCESS)
              {
                PRINT_NAMED_WARNING("PlaceRelObjectAction.Verify",
                                    "Robot thinks it placed the object %s, but verification of placement "
                                    "failed. Not sure where carry object %d is, so clearing it.",
                                    _dockAction == DockAction::DA_PLACE_LOW ? "low" : "high",
                                    _carryObjectID.GetValue());
                
                _robot.GetBlockWorld().ClearObject(_carryObjectID);  
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
              PRINT_CH_INFO("Actions", "PlaceRelObjectAction.Verify.Waiting", "");
            } // if(result != ActionResult::RUNNING)
            
          } else {
            // If the robot thinks it failed last pick-and-place, it is because it
            // failed to dock/track, so we are probably still holding the block
            PRINT_NAMED_WARNING("PlaceRelObjectAction.Verify",
                                "Robot reported placement failure. Assuming docking failed "
                                "and robot is still holding same block.");
            result = ActionResult::FAILURE_RETRY;
          }
          
          break;
        } // PLACE
          
        default:
          PRINT_NAMED_WARNING("PlaceRelObjectAction.Verify.ReachedDefaultCase",
                              "Don't know how to verify unexpected dockAction %s.", DockActionToString(_dockAction));
          result = ActionResult::FAILURE_ABORT;
          break;
          
      } // switch(_dockAction)
      
      return result;
      
    } // Verify()
    
    ActionResult PlaceRelObjectAction::TransformPlacementOffsetsRelativeObject()
    {
      ObservableObject* dockObject = _robot.GetBlockWorld().GetObjectByID(_dockObjectID);
      if(dockObject == nullptr){
        return ActionResult::FAILURE_ABORT;
      }
      
      Pose3d dockObjectWRTRobot;
      dockObject->GetZRotatedPointAboveObjectCenter()
                .GetWithRespectTo(_robot.GetPose(), dockObjectWRTRobot);
      const float robotObjRelRotation_rad =
                    dockObjectWRTRobot.GetRotation().GetAngleAroundZaxis().ToFloat();
      
      // consts for comparing relative robot/block alignment
      const float kRotationTolerence_rad = DEG_TO_RAD_F32(15);
      const float kInAlignment_rad = DEG_TO_RAD_F32(0);
      const float kClockwise_rad = DEG_TO_RAD_F32(-90);
      const float kCounterClockwise_rad = -kClockwise_rad;
      const float kOppposite_rad = DEG_TO_RAD_F32(180);
      const float kOppposite_rad_neg = -kOppposite_rad;

      //values to set placement offset with
      f32 xAbsolutePlacementOffset_mm;
      f32 yAbsolutePlacementOffset_mm;

      if(Util::IsNear(robotObjRelRotation_rad, kInAlignment_rad, kRotationTolerence_rad)){
        xAbsolutePlacementOffset_mm = -_relOffsetX_mm;
        yAbsolutePlacementOffset_mm = _relOffsetY_mm;
      }else if(Util::IsNear(robotObjRelRotation_rad, kCounterClockwise_rad, kRotationTolerence_rad)){
        xAbsolutePlacementOffset_mm = _relOffsetY_mm;
        yAbsolutePlacementOffset_mm = _relOffsetX_mm;
      }else if(Util::IsNear(robotObjRelRotation_rad, kClockwise_rad, kRotationTolerence_rad)){
        xAbsolutePlacementOffset_mm = -_relOffsetY_mm;
        yAbsolutePlacementOffset_mm = -_relOffsetX_mm;
      }else if( Util::IsNear(robotObjRelRotation_rad, kOppposite_rad, kRotationTolerence_rad)
               ||  Util::IsNear(robotObjRelRotation_rad, kOppposite_rad_neg, kRotationTolerence_rad)){
        xAbsolutePlacementOffset_mm = _relOffsetX_mm;
        yAbsolutePlacementOffset_mm = -_relOffsetY_mm;
      }else{
        PRINT_NAMED_WARNING("PlaceRelObjectAction.CalculatePlacementOffset.InvalidOrientation",
                          "Robot and block are not within alignment threshold - rotation:%f threshold:%f", RAD_TO_DEG(robotObjRelRotation_rad), kRotationTolerence_rad);
        return ActionResult::FAILURE_RETRY;
      }
      
      if(FLT_LT(xAbsolutePlacementOffset_mm, -kMaxNegativeXPlacementOffset)){
        PRINT_NAMED_ERROR("PlaceRelObjectAction.TransformPlacementOffsetsRelativeObject",
                          "Attempted to set negative xOffset. xOffset:%f, yOffset:%f", xAbsolutePlacementOffset_mm, yAbsolutePlacementOffset_mm);
        return ActionResult::FAILURE_ABORT;
      }
      
      _relOffsetX_mm = xAbsolutePlacementOffset_mm;
      _relOffsetY_mm = yAbsolutePlacementOffset_mm;

      return ActionResult::SUCCESS;
    }

    
#pragma mark ---- RollObjectAction ----
    
    RollObjectAction::RollObjectAction(Robot& robot, ObjectID objectID, const bool useManualSpeed)
    : IDockAction(robot,
                  objectID,
                  "RollObject",
                  RobotActionType::ROLL_OBJECT_LOW,
                  useManualSpeed)
    {
      _dockingMethod = (DockingMethod)kRollDockingMethod;
      _dockAction = DockAction::DA_ROLL_LOW;
      SetPostDockLiftMovingAnimation(AnimationTrigger::SoundOnlyLiftEffortPlaceRoll);
    }
    
    void RollObjectAction::EnableDeepRoll(bool enable)
    {
      _dockAction = enable ? DockAction::DA_DEEP_ROLL_LOW : DockAction::DA_ROLL_LOW;
      SetName(enable ? "DeepRollObject" : "RollObject");
    }
    
    void RollObjectAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      ObjectInteractionCompleted info;
      switch(_dockAction)
      {
        case DockAction::DA_ROLL_LOW:
        case DockAction::DA_DEEP_ROLL_LOW:
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
        {
          // Not setting dock action is only an issue if the action has started
          if(GetState() != ActionResult::FAILURE_NOT_STARTED)
          {
            PRINT_NAMED_WARNING("RollObjectAction.EmitCompletionSignal",
                                "Dock action not set before filling completion signal.");
          }
        }
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
        PRINT_CH_INFO("Actions", "RollObjectAction.SelectDockAction", "Object is too high to roll. Aborting.");
        _interactionResult = ObjectInteractionResult::INVALID_OBJECT;
        return RESULT_FAIL;
      } else if (_robot.IsCarryingObject()) {
        PRINT_CH_INFO("Actions", "RollObjectAction.SelectDockAction", "Can't roll while carrying an object.");
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
        case DockAction::DA_DEEP_ROLL_LOW:
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
              // Since rolling is the only action that moves the block and then immediatly needs to visually verify
              // The head needs to look down more to account for the fact the block pose moved towards us and then we can
              // do the verification
              _rollVerifyAction = new CompoundActionSequential(_robot,
                                                               {new MoveHeadToAngleAction(_robot, kAngleToLookDown),
                                                                new VisuallyVerifyObjectAction(_robot, _dockObjectID, _expectedMarkerPostRoll->GetCode())});
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
                PRINT_CH_INFO("Actions", "RollObjectAction.Verify",
                              "Robot thinks it rolled the object, but verification failed. ");
                
                // Automatically set to deep roll in case the action is retried
                EnableDeepRoll(true);
                
                result = ActionResult::FAILURE_RETRY;
              }
            } else {
              // Mostly for debugging when verification takes too long
              PRINT_CH_INFO("Actions", "RollObjectAction.Verify.Waiting", "");
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
    : IDockAction(robot,
                  rampID,
                  "AscendOrDescendRamp",
                  RobotActionType::ASCEND_OR_DESCEND_RAMP,
                  useManualSpeed)
    {
      
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
      PRINT_CH_INFO("Actions", "AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                    "Robot has completed going up/down ramp.");
      
      return ActionResult::SUCCESS;
    } // Verify()
    
#pragma mark ---- MountChargerAction ----
    
    MountChargerAction::MountChargerAction(Robot& robot, ObjectID chargerID, const bool useManualSpeed)
    : IDockAction(robot,
                  chargerID,
                  "MountCharger",
                  RobotActionType::MOUNT_CHARGER,
                  useManualSpeed)
    {
      // TODO: Charger marker pose still oscillates so just do your best from where you are
      //       rather than oscillating between jumpy predock poses.
      SetDoNearPredockPoseCheck(false);
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
        PRINT_CH_INFO("Actions", "MountChargerAction.Verify.MountingChargerComplete",
                      "Robot has mounted charger.");
        return ActionResult::SUCCESS;
      }
      return ActionResult::FAILURE_ABORT;
    } // Verify()

#pragma mark ---- CrossBridgeAction ----
    
    CrossBridgeAction::CrossBridgeAction(Robot& robot, ObjectID bridgeID, const bool useManualSpeed)
    : IDockAction(robot,
                  bridgeID,
                  "CrossBridge",
                  RobotActionType::CROSS_BRIDGE,
                  useManualSpeed)
    {
      
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
      PRINT_CH_INFO("Actions", "CrossBridgeAction.Verify.BridgeCrossingComplete",
                    "Robot has completed crossing a bridge.");
      return ActionResult::SUCCESS;
    } // Verify()
  }
}

