/**
 * File: flipBlockAction.cpp
 *
 * Author: Al Chaussee
 * Date:   5/18/16
 *
 * Description: Action which flips blocks
 *              By default, when driving to the flipping preAction pose, we will drive to one of the two poses that is closest
 *              to Cozmo and farthest from the last known face. In order to maximize the chances of the person being able to see
 *              Cozmo's face and reactions while he is flipping the block
 *                - Should there not be a last known face, of the two closest preAction poses the left most will be chosen
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "flipBlockAction.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

namespace {
// Utility function to check if the robot is within a threshold of a preAction pose
bool WithinPreActionThreshold(const Robot& robot, std::vector<Pose3d>& possiblePoses, f32 threshold)
{
  float distanceBetweenRobotAndPose;
  for(auto pose : possiblePoses){
    ComputeDistanceSQBetween(robot.GetPose(), pose, distanceBetweenRobotAndPose);
    if(threshold >= 0 && threshold > distanceBetweenRobotAndPose){
      return true;
    }
  }
  
  return false;
}
  
}

static constexpr f32 kPreDockPoseAngleTolerance = DEG_TO_RAD(5.f);

DriveAndFlipBlockAction::DriveAndFlipBlockAction(Robot& robot,
                                                 const ObjectID objectID,
                                                 const bool useApproachAngle,
                                                 const f32 approachAngle_rad,
                                                 const bool useManualSpeed,
                                                 Radians maxTurnTowardsFaceAngle_rad,
                                                 const bool sayName,
                                                 const float minAlignThreshold_mm)
: IDriveToInteractWithObject(robot,
                             objectID,
                             PreActionPose::FLIPPING,
                             0,
                             useApproachAngle,
                             approachAngle_rad,
                             useManualSpeed,
                             maxTurnTowardsFaceAngle_rad,
                             sayName)
, _flipBlockAction(new FlipBlockAction(robot, objectID))
{
  SetName("DriveToAndFlipBlock");
  SetProxyTag(_flipBlockAction->GetTag());
  GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, &robot](ActionableObject* object, std::vector<Pose3d>& possiblePoses, bool& alreadyInPosition)
  {
    
    // Check to see if the robot is close enough to the preActionPose to prevent tiny re-alignments
    if(!alreadyInPosition && _minAlignThreshold_mm >= 0){
      bool withinThreshold = WithinPreActionThreshold(robot, possiblePoses, _minAlignThreshold_mm);
      alreadyInPosition = withinThreshold;
      _flipBlockAction->SetShouldCheckPreActionPose(!withinThreshold);
    }
    
    return GetPossiblePoses(robot, object, possiblePoses, alreadyInPosition, false);
  });

  AddAction(_flipBlockAction);
  SetProxyTag(_flipBlockAction->GetTag()); // Use flip action's completion info
}

void DriveAndFlipBlockAction::ShouldDriveToClosestPreActionPose(bool tf)
{
  GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, tf](ActionableObject* object, std::vector<Pose3d>& possiblePoses, bool& alreadyInPosition)
  {
    
    // Check to see if the robot is close enough to the preActionPose to prevent tiny re-alignments
    if(!alreadyInPosition && _minAlignThreshold_mm >= 0){
      bool withinThreshold = WithinPreActionThreshold(_robot, possiblePoses, _minAlignThreshold_mm);
      alreadyInPosition = withinThreshold;
      _flipBlockAction->SetShouldCheckPreActionPose(!withinThreshold);
    }
    
    
    return GetPossiblePoses(_robot, object, possiblePoses, alreadyInPosition, tf);
  });
}

ActionResult DriveAndFlipBlockAction::GetPossiblePoses(Robot& robot,
                                                       ActionableObject* object,
                                                       std::vector<Pose3d>& possiblePoses,
                                                       bool& alreadyInPosition,
                                                       const bool shouldDriveToClosestPose)
{
  PRINT_NAMED_INFO("DriveAndFlipBlockAction.GetPossiblePoses", "Getting possible preActionPoses");
  const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                           PreActionPose::FLIPPING,
                                                           false,
                                                           0,
                                                           kPreDockPoseAngleTolerance,
                                                           false, 0);
  
  IDockAction::PreActionPoseOutput preActionPoseOutput;
  
  IDockAction::GetPreActionPoses(robot, preActionPoseInput, preActionPoseOutput);
  
  if(preActionPoseOutput.actionResult == ActionResult::FAILURE_ABORT)
  {
    PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction.Constructor", "Failed to find closest preAction pose");
    return ActionResult::FAILURE_ABORT;
  }
  
  Pose3d facePose;
  TimeStamp_t faceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);
  
  if(preActionPoseOutput.preActionPoses.empty())
  {
    PRINT_NAMED_WARNING("DriveToFlipBlockPoseAction.Constructor", "No preAction poses");
    return ActionResult::FAILURE_ABORT;
  }
  
  if(shouldDriveToClosestPose)
  {
    PRINT_NAMED_INFO("DriveAndFlipBlockAction.GetPossiblePoses", "Selecting closest preAction pose");
    possiblePoses.push_back(preActionPoseOutput.preActionPoses[preActionPoseOutput.closestIndex].GetPose());
    return ActionResult::SUCCESS;
  }
  
  Pose3d firstClosestPose;
  Pose3d secondClosestPose;
  f32 firstClosestDist = std::numeric_limits<float>::max();
  f32 secondClosestDist = firstClosestDist;
  
  for(auto iter = preActionPoseOutput.preActionPoses.begin(); iter != preActionPoseOutput.preActionPoses.end(); ++iter)
  {
    Pose3d poseWrtRobot;
    if(!iter->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot))
    {
      continue;
    }
    
    f32 dist = poseWrtRobot.GetTranslation().Length();
    
    
    if(dist < firstClosestDist)
    {
      secondClosestDist = firstClosestDist;
      secondClosestPose = firstClosestPose;
      
      firstClosestDist = dist;
      firstClosestPose = poseWrtRobot;
    }
    else if(dist < secondClosestDist)
    {
      secondClosestDist = dist;
      secondClosestPose = poseWrtRobot;
    }
  }
  
  Pose3d poseToDriveTo;
  
  // There is only one preaction pose so it will be the first closest
  if(preActionPoseOutput.preActionPoses.size() == 1)
  {
    poseToDriveTo = firstClosestPose;
  }
  else
  {
    Pose3d firstClosestPoseWRTFace = Pose3d(firstClosestPose);
    Pose3d secondClosestPoseWRTFace = Pose3d(secondClosestPose);
    const bool hasKnownFaces = faceTime != 0;
    const bool noFacesInFrame = hasKnownFaces && ((!firstClosestPose.GetWithRespectTo(facePose, firstClosestPoseWRTFace)
                                                    || !secondClosestPose.GetWithRespectTo(facePose, secondClosestPoseWRTFace)));
    
    if(!hasKnownFaces || noFacesInFrame)
    {
      // No last known face so pick the preaction pose that is left most relative to Cozmo
      if(firstClosestPose.GetTranslation().y() >= secondClosestPose.GetTranslation().y())
      {
        poseToDriveTo = firstClosestPose;
      }
      else
      {
        poseToDriveTo = secondClosestPose;
      }
    }else{
      // Otherwise pick one of the two preaction poses closest to the robot and farthest from the last known face

      if(firstClosestPoseWRTFace.GetTranslation().Length() > secondClosestPoseWRTFace.GetTranslation().Length())
      {
        poseToDriveTo = firstClosestPose;
      }
      else
      {
        poseToDriveTo = secondClosestPose;
      }
    }
  }

  possiblePoses.push_back(poseToDriveTo);
  return ActionResult::SUCCESS;
}


DriveToFlipBlockPoseAction::DriveToFlipBlockPoseAction(Robot& robot, ObjectID objectID)
: DriveToObjectAction(robot, objectID, PreActionPose::FLIPPING)
{
  SetName("DriveToFlipBlockPose");
  SetType(RobotActionType::DRIVE_TO_FLIP_BLOCK_POSE);
  SetGetPossiblePosesFunc([this, &robot](ActionableObject* object, std::vector<Pose3d>& possiblePoses, bool& alreadyInPosition)
  {
    return DriveAndFlipBlockAction::GetPossiblePoses(robot, object, possiblePoses, alreadyInPosition, false);
  });
}

void DriveToFlipBlockPoseAction::ShouldDriveToClosestPreActionPose(bool tf)
{
  SetGetPossiblePosesFunc([this, tf](ActionableObject* object, std::vector<Pose3d>& possiblePoses, bool& alreadyInPosition)
  {
    return DriveAndFlipBlockAction::GetPossiblePoses(_robot, object, possiblePoses, alreadyInPosition, tf);
  });
}


FlipBlockAction::FlipBlockAction(Robot& robot, ObjectID objectID)
: IAction(robot,
          "FlipBlock", 
          RobotActionType::FLIP_BLOCK,
          ((u8)AnimTrackFlag::LIFT_TRACK | (u8)AnimTrackFlag::BODY_TRACK))
, _objectID(objectID)
, _compoundAction(robot)
, _shouldCheckPreActionPose(true)
{
}

FlipBlockAction::~FlipBlockAction()
{
  _compoundAction.PrepForCompletion();
  if(_flipTag != -1)
  {
    _robot.GetActionList().Cancel(_flipTag);
  }
  
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, true);
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, true);
}

void FlipBlockAction::SetShouldCheckPreActionPose(bool shouldCheck)
{
  _shouldCheckPreActionPose = shouldCheck;
}
  
ActionResult FlipBlockAction::Init()
{
  ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("FlipBlockAction.Init.NullObject", "ObjectID=%d", _objectID.GetValue());
    return ActionResult::FAILURE_ABORT;
  }
  
  if(object->IsPoseStateUnknown())
  {
    PRINT_NAMED_WARNING("FlipBlockAction.Init.UnknownPose", "Object %d pose state is unknown", _objectID.GetValue());
    return ActionResult::FAILURE_ABORT;
  }
  
  const IDockAction::PreActionPoseInput preActionPoseInput(object,
                                                           PreActionPose::FLIPPING,
                                                           _shouldCheckPreActionPose,
                                                           0,
                                                           kPreDockPoseAngleTolerance,
                                                           false, 0);
  
  IDockAction::PreActionPoseOutput preActionPoseOutput;
  
  IDockAction::GetPreActionPoses(_robot, preActionPoseInput, preActionPoseOutput);
  
  if(preActionPoseOutput.actionResult != ActionResult::SUCCESS)
  {
    return preActionPoseOutput.actionResult;
  }
  
  Pose3d p;
  object->GetPose().GetWithRespectTo(_robot.GetPose(), p);

  // Need to suppress track locking so the two lift actions don't fail because the other locked the lift track
  // A little dangerous as animations playing in parallel to this action could move lift
  _compoundAction.ShouldSuppressTrackLocking(true);
  
  // Ensure that the robot doesn't react to slowing down when it hits the blocks
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, false);
  _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToCubeMoved, false);
  
  // Drive through the block
  DriveStraightAction* drive = new DriveStraightAction(_robot, p.GetTranslation().Length() + kDrivingDist_mm, kDrivingSpeed_mmps);
  
  // Need to set the initial lift height to fit lift base into block corner edge
  MoveLiftToHeightAction* initialLift = new MoveLiftToHeightAction(_robot, kInitialLiftHeight_mm);
  
  _compoundAction.AddAction(initialLift);
  _compoundAction.AddAction(drive);
  _compoundAction.Update();
  return ActionResult::SUCCESS;
}

ActionResult FlipBlockAction::CheckIfDone()
{
  ActionResult result = _compoundAction.Update();
  if(result != ActionResult::RUNNING)
  {
    // By clearing the bottom block the entire stack will get cleared
    _robot.GetBlockWorld().ClearObject(_objectID);
    return result;
  }
  
  ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("FlipBlockAction.CheckIfDone.NullObject", "ObjectID=%d", _objectID.GetValue());
    return ActionResult::FAILURE_ABORT;
  }
  
  Pose3d p;
  object->GetPose().GetWithRespectTo(_robot.GetPose(), p);
  if(p.GetTranslation().Length() < kDistToObjectToFlip_mm && _flipTag == -1)
  {
    IAction* action = new MoveLiftToHeightAction(_robot, MoveLiftToHeightAction::Preset::CARRY);
    action->ShouldEmitCompletionSignal(false);
    
    // FlipBlockAction is already locking all tracks so this lift action doesn't need to lock
    action->ShouldSuppressTrackLocking(true);
    _flipTag = action->GetTag();
    _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, action);
  }

  return ActionResult::RUNNING;
}
  
}
}
